// AISignificanceManager.cpp — Tick-based LOD for AI soldiers
// Evaluates every 0.25s (4Hz), applies tick rate + mesh URO adjustments
#include "Performance/AISignificanceManager.h"
#include "Characters/SoldierCharacter.h"
#include "AI/SoldierAIController.h"
#include "SquadAITuning.h"
#include "Kismet/GameplayStatics.h"

void UAISignificanceManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UpdateInterval = USquadAITuning::Get()->SignificanceUpdateInterval;
}

void UAISignificanceManager::Deinitialize()
{
	RegisteredSoldiers.Empty();
	Super::Deinitialize();
}

void UAISignificanceManager::RegisterSoldier(ASoldierCharacter* Soldier)
{
	if (Soldier) RegisteredSoldiers.AddUnique(Soldier);
}

void UAISignificanceManager::UnregisterSoldier(ASoldierCharacter* Soldier)
{
	RegisteredSoldiers.Remove(Soldier);
}

void UAISignificanceManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceLastUpdate += DeltaTime;
	if (TimeSinceLastUpdate < UpdateInterval) return;
	TimeSinceLastUpdate = 0.f;

	// Evaluate all registered soldiers
	for (int32 i = RegisteredSoldiers.Num() - 1; i >= 0; --i)
	{
		ASoldierCharacter* Soldier = RegisteredSoldiers[i].Get();
		if (!Soldier || !Soldier->IsAlive())
		{
			RegisteredSoldiers.RemoveAtSwap(i, EAllowShrinking::No);
			continue;
		}

		float Significance = EvaluateSignificance(Soldier);
		EAISignificance Bucket = SignificanceToBucket(Significance);
		ApplySignificanceBucket(Soldier, Bucket);

		// Write bucket back to controller
		if (ASoldierAIController* AI = Cast<ASoldierAIController>(Soldier->GetController()))
		{
			AI->CurrentLODBucket = static_cast<int32>(Bucket);
		}
	}
}

// =====================================================================
//  EVALUATE — distance + angular significance
// =====================================================================
float UAISignificanceManager::EvaluateSignificance(ASoldierCharacter* Soldier) const
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player) return 0.5f;

	const FVector SoldierLoc = Soldier->GetActorLocation();
	const FVector PlayerLoc = Player->GetActorLocation();
	const float Distance = FVector::Dist(SoldierLoc, PlayerLoc);

	const float MaxDist = USquadAITuning::Get()->SignificanceDormantDistance;

	// Base: distance-based (0 = dormant, 1 = critical)
	float Significance = 1.f - FMath::Clamp(Distance / MaxDist, 0.f, 1.f);

	// Angular boost: AI in front of player gets +0.1
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		const FVector ViewForward = PC->GetControlRotation().Vector();
		const FVector ToSoldier = (SoldierLoc - PlayerLoc).GetSafeNormal();
		const float Dot = FVector::DotProduct(ViewForward, ToSoldier);

		if (Dot > 0.5f) Significance += 0.1f;       // In front
		else if (Dot < -0.3f) Significance -= 0.1f;  // Behind
	}

	// Reactive tick override: forced full significance
	if (ASoldierAIController* AI = Cast<ASoldierAIController>(Soldier->GetController()))
	{
		if (AI->bForceFullTick)
		{
			Significance = FMath::Max(Significance, 0.85f);
		}
	}

	return FMath::Clamp(Significance, 0.f, 1.f);
}

// =====================================================================
//  APPLY LOD — adjust tick rates, mesh URO
// =====================================================================
void UAISignificanceManager::ApplySignificanceBucket(ASoldierCharacter* Soldier, EAISignificance Bucket)
{
	AController* Controller = Soldier->GetController();

	switch (Bucket)
	{
	case EAISignificance::Critical:
	case EAISignificance::High:
		if (Controller) Controller->PrimaryActorTick.TickInterval = 0.1f;
		if (Soldier->GetMesh()) Soldier->GetMesh()->bEnableUpdateRateOptimizations = false;
		break;

	case EAISignificance::Medium:
		if (Controller) Controller->PrimaryActorTick.TickInterval = 0.2f;
		if (Soldier->GetMesh()) Soldier->GetMesh()->bEnableUpdateRateOptimizations = true;
		break;

	case EAISignificance::Low:
		if (Controller) Controller->PrimaryActorTick.TickInterval = 0.35f;
		if (Soldier->GetMesh()) Soldier->GetMesh()->bEnableUpdateRateOptimizations = true;
		break;

	case EAISignificance::VeryLow:
		if (Controller) Controller->PrimaryActorTick.TickInterval = 0.5f;
		if (Soldier->GetMesh()) Soldier->GetMesh()->bEnableUpdateRateOptimizations = true;
		break;

	case EAISignificance::Dormant:
		if (Controller) Controller->PrimaryActorTick.TickInterval = 1.f;
		if (Soldier->GetMesh()) Soldier->GetMesh()->bEnableUpdateRateOptimizations = true;
		break;
	}
}

EAISignificance UAISignificanceManager::SignificanceToBucket(float Significance) const
{
	if (Significance >= 0.85f) return EAISignificance::Critical;
	if (Significance >= 0.65f) return EAISignificance::High;
	if (Significance >= 0.45f) return EAISignificance::Medium;
	if (Significance >= 0.25f) return EAISignificance::Low;
	if (Significance >= 0.1f)  return EAISignificance::VeryLow;
	return EAISignificance::Dormant;
}
