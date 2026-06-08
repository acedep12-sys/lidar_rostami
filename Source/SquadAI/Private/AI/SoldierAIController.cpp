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
#include "GenericTeamAgentInterface.h"
#include "GameplayTasksComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

ASoldierAIController::ASoldierAIController(const FObjectInitializer& OI)
	: Super(OI.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	SetPerceptionComponent(*PerceptionComp);
	StateTreeComp = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	TasksComp = CreateDefaultSubobject<UGameplayTasksComponent>(TEXT("GameplayTasks"));
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f; 
	TeamId = FGenericTeamId(255);
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
	if (ASoldierCharacter* Soldier = Cast<ASoldierCharacter>(InPawn))
		SetGenericTeamId(Soldier->GetGenericTeamId());
	Super::OnPossess(InPawn);
	SetupPerception();
	if (StateTreeComp) StateTreeComp->RestartLogic();
	if (PerceptionComp) PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &ASoldierAIController::OnPerceptionUpdated);
}

void ASoldierAIController::OnUnPossess()
{
	if (PerceptionComp) PerceptionComp->OnTargetPerceptionUpdated.RemoveDynamic(this, &ASoldierAIController::OnPerceptionUpdated);
	Super::OnUnPossess();
}

void ASoldierAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ASoldierCharacter* Soldier = GetSoldierCharacter();
	const float CoverDecayMult = (Soldier && Soldier->bIsInCover) ? 0.5f : 1.f;
	Suppression = FMath::Max(0.f, Suppression - SuppressionDecayPerSec * DeltaTime * CoverDecayMult);
	UpdateMemory(DeltaTime);
}

void ASoldierAIController::SetupPerception()
{
	if (!PerceptionComp) return;
	const USquadAITuning* T = USquadAITuning::Get();
	SightConfig = NewObject<UAISenseConfig_Sight>(this);
	SightConfig->SightRadius = T->SightRadius;
	SightConfig->LoseSightRadius = T->LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = T->PeripheralVisionDegrees;
	SightConfig->DetectionByAffiliation.bDetectEnemies = SightConfig->DetectionByAffiliation.bDetectNeutrals = SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	PerceptionComp->ConfigureSense(*SightConfig);
	HearingConfig = NewObject<UAISenseConfig_Hearing>(this);
	HearingConfig->HearingRange = T->HearingRange;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = HearingConfig->DetectionByAffiliation.bDetectNeutrals = HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	PerceptionComp->ConfigureSense(*HearingConfig);
	DamageConfig = NewObject<UAISenseConfig_Damage>(this);
	PerceptionComp->ConfigureSense(*DamageConfig);
}

void ASoldierAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !Cast<APawn>(Actor)) return;
	bool bIsAlive = true;
	if (ASoldierCharacter* S = Cast<ASoldierCharacter>(Actor)) bIsAlive = S->IsAlive();
	if (!bIsAlive) return;

	// Use Engine built-in attitude check (Powered by our Solver in SquadAI.cpp)
	if (GetAttitudeTowards(*Actor) != ETeamAttitude::Hostile) return;

	if (Stimulus.WasSuccessfullySensed()) {
		FPerceivedTarget* Existing = Memory.FindByPredicate([Actor](const FPerceivedTarget& T) { return T.Actor.Get() == Actor; });
		if (Existing) {
			Existing->LastKnownLocation = Actor->GetActorLocation();
			Existing->Confidence = 1.f;
			Existing->bCurrentlyVisible = true;
		} else {
			FPerceivedTarget NewTarget; NewTarget.Actor = Actor; NewTarget.LastKnownLocation = Actor->GetActorLocation();
			NewTarget.Confidence = 1.f; NewTarget.bCurrentlyVisible = true; Memory.Add(NewTarget);
		}
	} else {
		FPerceivedTarget* Existing = Memory.FindByPredicate([Actor](const FPerceivedTarget& T) { return T.Actor.Get() == Actor; });
		if (Existing) Existing->bCurrentlyVisible = false;
	}
}

void ASoldierAIController::UpdateMemory(float DeltaTime)
{
	for (int32 i = Memory.Num() - 1; i >= 0; --i) {
		FPerceivedTarget& T = Memory[i];
		if (!T.Actor.IsValid() || (Cast<ASoldierCharacter>(T.Actor.Get()) && !Cast<ASoldierCharacter>(T.Actor.Get())->IsAlive())) {
			Memory.RemoveAtSwap(i); continue;
		}
		if (!T.bCurrentlyVisible) {
			T.Confidence -= MemoryDecayRate * DeltaTime;
			if (T.Confidence <= 0.f) Memory.RemoveAtSwap(i);
		} else T.LastKnownLocation = T.Actor->GetActorLocation();
	}
}

AActor* ASoldierAIController::GetPrimaryThreat(float& OutConfidence) const
{
	AActor* Best = nullptr; float BestScore = -FLT_MAX;
	for (const FPerceivedTarget& T : Memory) {
		float Score = T.Confidence * 1000.f - FVector::DistSquared(T.LastKnownLocation, GetPawn()->GetActorLocation()) * 0.001f;
		if (Score > BestScore) { BestScore = Score; Best = T.Actor.Get(); OutConfidence = T.Confidence; }
	}
	return Best;
}

TArray<AActor*> ASoldierAIController::GetCurrentlyVisibleHostiles() const
{
	TArray<AActor*> Result;
	for (const FPerceivedTarget& T : Memory) if (T.bCurrentlyVisible && T.Actor.IsValid()) Result.Add(T.Actor.Get());
	return Result;
}

void ASoldierAIController::AddSuppression(float Amount) { Suppression = FMath::Clamp(Suppression + Amount, 0.f, 1.f); }
void ASoldierAIController::RequestCover(FVector TL, float R) {
	bHasPendingCover = false; UCoverSystemSubsystem* CS = GetWorld()->GetSubsystem<UCoverSystemSubsystem>();
	if (!CS || !GetPawn()) return;
	TWeakObjectPtr<ASoldierAIController> WeakSelf(this);
	CS->RequestCoverAsync(GetPawn(), GetPawn()->GetActorLocation(), TL, R, [WeakSelf](bool bFound, const FCoverPoint& Res) {
		if (WeakSelf.IsValid()) { WeakSelf->PendingCoverResult = Res; WeakSelf->bHasPendingCover = bFound; }
	});
}

ASoldierCharacter* ASoldierAIController::GetSoldierCharacter() const { return Cast<ASoldierCharacter>(GetPawn()); }
bool ASoldierAIController::HasLineOfSightTo(AActor* Target) const {
	if (!Target || !GetPawn()) return false;
	FHitResult Hit; FCollisionQueryParams P; P.AddIgnoredActor(GetPawn());
	return !GetWorld()->LineTraceSingleByChannel(Hit, GetPawn()->GetActorLocation() + FVector(0,0,60), Target->GetActorLocation() + FVector(0,0,60), ECC_Visibility, P);
}
