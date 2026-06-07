// SoldierAIController.cpp — Detour Crowd + 3 senses + fading memory + suppression
#include "AI/SoldierAIController.h"
#include "Characters/SoldierCharacter.h"
#include "CoverSystem/CoverSystemSubsystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Components/StateTreeAIComponent.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "SquadAITuning.h"
#include "Performance/SharedThreatMemory.h"
#include "Performance/AISignificanceManager.h"

ASoldierAIController::ASoldierAIController(const FObjectInitializer& OI)
	: Super(OI.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	SetPerceptionComponent(*PerceptionComp);

	StateTreeComp = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f; // 10 Hz for memory + suppression decay
}

void ASoldierAIController::BeginPlay()
{
	Super::BeginPlay();

	const USquadAITuning* T = USquadAITuning::Get();
	MemoryDecayRate = T->MemoryDecayPerSecond;
	SuppressionDecayPerSec = T->SuppressionDecayPerSecond;
}

void ASoldierAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Match team from character
	if (ASoldierCharacter* Soldier = Cast<ASoldierCharacter>(InPawn))
	{
		SetGenericTeamId(Soldier->GetGenericTeamId());
	}

	SetupPerception();

	if (PerceptionComp)
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &ASoldierAIController::OnPerceptionUpdated);
	}
}

void ASoldierAIController::OnUnPossess()
{
	if (PerceptionComp)
	{
		PerceptionComp->OnTargetPerceptionUpdated.RemoveDynamic(this, &ASoldierAIController::OnPerceptionUpdated);
	}
	Super::OnUnPossess();
}

void ASoldierAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float Now = GetWorld()->GetTimeSeconds();

	// ---- REACTIVE TICK: check if we should force full fidelity ----
	if (bForceFullTick && Now < ForceFullTickUntil)
	{
		// Stay at full fidelity until the timer expires
	}
	else
	{
		bForceFullTick = false;
	}

	// ---- PROXIMITY ALARM: cheap 30Hz distance check ----
	// Even Ambient/Dormant AI do this — promotes bucket on player proximity
	if (GetPawn())
	{
		APawn* Player = GetWorld()->GetFirstPlayerController() ?
			GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;

		if (Player)
		{
			const float DistSq = FVector::DistSquared(GetPawn()->GetActorLocation(), Player->GetActorLocation());
			if (DistSq < 2000.f * 2000.f && CurrentLODBucket >= 3) // 3 = Low bucket
			{
				// Player is close — force full tick for 1 second
				bForceFullTick = true;
				ForceFullTickUntil = Now + 1.f;
			}
		}
	}

	// ---- SUPPRESSION DECAY ----
	// Decay slower when in cover (suggestion 2.2 from improvements doc)
	ASoldierCharacter* Soldier = GetSoldierCharacter();
	const bool bInCover = Soldier ? Soldier->bIsInCover : false;
	const float CoverDecayMult = bInCover ? 0.5f : 1.f;
	Suppression = FMath::Max(0.f, Suppression - SuppressionDecayPerSec * DeltaTime * CoverDecayMult);

	// ---- MEMORY DECAY ----
	UpdateMemory(DeltaTime);

	// ---- SHARED THREAT REPORTING ----
	// When we see a threat, broadcast to squad-wide memory
	if (Memory.Num() > 0 && GetPawn())
	{
		USharedThreatMemorySubsystem* SharedMem = GetWorld()->GetSubsystem<USharedThreatMemorySubsystem>();
		if (SharedMem)
		{
			for (const FPerceivedTarget& T : Memory)
			{
				if (T.bCurrentlyVisible && T.Actor.IsValid())
				{
					SharedMem->ReportThreat(
						T.Actor.Get(),
						T.LastKnownLocation,
						FVector::ZeroVector,
						T.Confidence,
						GetGenericTeamId().GetId()
					);
				}
			}
		}
	}
}

// =====================================================================
//  PERCEPTION SETUP — 3 senses, team-filtered
// =====================================================================
void ASoldierAIController::SetupPerception()
{
	if (!PerceptionComp) return;

	const USquadAITuning* T = USquadAITuning::Get();

	// Sight
	SightConfig = NewObject<UAISenseConfig_Sight>(this);
	SightConfig->SightRadius = T->SightRadius;
	SightConfig->LoseSightRadius = T->LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = T->PeripheralVisionDegrees;
	SightConfig->SetMaxAge(20.f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 500.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	PerceptionComp->ConfigureSense(*SightConfig);

	// Hearing — detects ANY team (friendly gunfire = "where's the fight?")
	HearingConfig = NewObject<UAISenseConfig_Hearing>(this);
	HearingConfig->HearingRange = T->HearingRange;
	HearingConfig->SetMaxAge(8.f);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	PerceptionComp->ConfigureSense(*HearingConfig);

	// Damage — fires when hit, even from behind
	DamageConfig = NewObject<UAISenseConfig_Damage>(this);
	DamageConfig->SetMaxAge(5.f);
	PerceptionComp->ConfigureSense(*DamageConfig);

	PerceptionComp->SetDominantSense(UAISense_Sight::StaticClass());
}

// =====================================================================
//  PERCEPTION CALLBACK
// =====================================================================
void ASoldierAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;

	ASoldierCharacter* Other = Cast<ASoldierCharacter>(Actor);
	if (!Other || !Other->IsAlive()) return;

	// Only track hostiles for target memory
	const ETeamAttitude::Type Attitude = FGenericTeamId::GetAttitude(GetGenericTeamId(), Other->GetGenericTeamId());
	if (Attitude != ETeamAttitude::Hostile) return;

	if (Stimulus.WasSuccessfullySensed())
	{
		// Update or add to memory
		FPerceivedTarget* Existing = Memory.FindByPredicate([Actor](const FPerceivedTarget& T) {
			return T.Actor.Get() == Actor;
		});

		if (Existing)
		{
			Existing->LastKnownLocation = Actor->GetActorLocation();
			Existing->LastSeenTime = GetWorld()->GetTimeSeconds();
			Existing->Confidence = 1.f;
			Existing->bCurrentlyVisible = Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>();
		}
		else
		{
			FPerceivedTarget NewTarget;
			NewTarget.Actor = Actor;
			NewTarget.LastKnownLocation = Actor->GetActorLocation();
			NewTarget.LastSeenTime = GetWorld()->GetTimeSeconds();
			NewTarget.Confidence = 1.f;
			NewTarget.bCurrentlyVisible = Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>();
			Memory.Add(NewTarget);
		}
	}
	else
	{
		// Lost perception — mark not visible, confidence will decay
		FPerceivedTarget* Existing = Memory.FindByPredicate([Actor](const FPerceivedTarget& T) {
			return T.Actor.Get() == Actor;
		});
		if (Existing)
		{
			Existing->bCurrentlyVisible = false;
		}
	}
}

// =====================================================================
//  MEMORY DECAY — confidence fades, old entries removed
// =====================================================================
void ASoldierAIController::UpdateMemory(float DeltaTime)
{
	for (int32 i = Memory.Num() - 1; i >= 0; --i)
	{
		FPerceivedTarget& T = Memory[i];

		// Remove invalid
		if (!T.Actor.IsValid())
		{
			Memory.RemoveAtSwap(i, EAllowShrinking::No);
			continue;
		}

		// Remove dead
		if (ASoldierCharacter* S = Cast<ASoldierCharacter>(T.Actor.Get()))
		{
			if (!S->IsAlive())
			{
				Memory.RemoveAtSwap(i, EAllowShrinking::No);
				continue;
			}
		}

		// Decay confidence when not visible
		if (!T.bCurrentlyVisible)
		{
			T.Confidence -= MemoryDecayRate * DeltaTime;
			if (T.Confidence <= 0.f)
			{
				Memory.RemoveAtSwap(i, EAllowShrinking::No);
				continue;
			}
		}
		else
		{
			// Keep confidence at 1 while visible, update location
			T.Confidence = 1.f;
			T.LastKnownLocation = T.Actor->GetActorLocation();
		}
	}
}

// =====================================================================
//  THREAT SCORING
// =====================================================================
AActor* ASoldierAIController::GetPrimaryThreat(float& OutConfidence) const
{
	AActor* Best = nullptr;
	float BestScore = -FLT_MAX;
	const FVector SelfLoc = GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;

	for (const FPerceivedTarget& T : Memory)
	{
		AActor* A = T.Actor.Get();
		if (!A) continue;

		const float Dist2 = FVector::DistSquared2D(A->GetActorLocation(), SelfLoc);
		const float Score = T.Confidence * 1000.f - Dist2 * 0.001f;

		if (Score > BestScore)
		{
			BestScore = Score;
			Best = A;
			OutConfidence = T.Confidence;
		}
	}

	if (!Best) OutConfidence = 0.f;
	return Best;
}

TArray<AActor*> ASoldierAIController::GetCurrentlyVisibleHostiles() const
{
	TArray<AActor*> Result;
	if (PerceptionComp)
	{
		PerceptionComp->GetPerceivedHostileActors(Result);
	}
	return Result;
}

// =====================================================================
//  SUPPRESSION
// =====================================================================
void ASoldierAIController::AddSuppression(float Amount)
{
	Suppression = FMath::Clamp(Suppression + Amount, 0.f, 1.f);

	// Reactive tick: suppression = being shot at → wake up
	bForceFullTick = true;
	ForceFullTickUntil = GetWorld()->GetTimeSeconds() + 2.f;
}

// =====================================================================
//  COVER REQUEST — stores result on controller (safe across StateTree frames)
// =====================================================================
void ASoldierAIController::RequestCover(FVector ThreatLocation, float SearchRadius)
{
	bHasPendingCover = false;

	UCoverSystemSubsystem* CoverSys = GetWorld()->GetSubsystem<UCoverSystemSubsystem>();
	if (!CoverSys || !GetPawn()) return;

	// Store result on the controller (long-lived UObject), NOT in StateTree instance data
	CoverSys->RequestCoverAsync(
		GetPawn(), GetPawn()->GetActorLocation(), ThreatLocation, SearchRadius,
		[this](bool bFound, const FCoverPoint& Result)
		{
			PendingCoverResult = Result;
			bHasPendingCover = bFound;
		});
}

// =====================================================================
//  HELPERS
// =====================================================================
ASoldierCharacter* ASoldierAIController::GetSoldierCharacter() const
{
	return Cast<ASoldierCharacter>(GetPawn());
}

bool ASoldierAIController::HasLineOfSightTo(AActor* Target) const
{
	if (!Target || !GetPawn()) return false;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetPawn());

	const FVector Start = GetPawn()->GetActorLocation() + FVector(0, 0, 60.f);
	const FVector End = Target->GetActorLocation() + FVector(0, 0, 60.f);

	bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	return !bBlocked || Hit.GetActor() == Target;
}
