// ShooterAIStateTreeData.cpp — StateTree evaluators
#include "AI/StateTreeTasks/ShooterAIStateTreeData.h"
#include "StateTreeExecutionContext.h"
#include "AI/SoldierAIController.h"
#include "Characters/SoldierCharacter.h"
#include "Components/HealthComponent.h"
#include "AIController.h"

// Helper: get controller from execution context (correct 5.7 API)
static ASoldierAIController* GetControllerFromContext(FStateTreeExecutionContext& Context)
{
	// GetOwner() returns the actor the StateTree is running on
	AActor* OwnerActor = Cast<AActor>(Context.GetOwner());
	if (!OwnerActor) return nullptr;

	if (APawn* Pawn = Cast<APawn>(OwnerActor))
	{
		return Cast<ASoldierAIController>(Pawn->GetController());
	}

	return Cast<ASoldierAIController>(OwnerActor);
}

// =====================================================================
//  THREAT EVALUATOR
// =====================================================================
void FSTE_Threat::TreeStart(FStateTreeExecutionContext& Context) const
{
	// Initialize on tree start
	Tick(Context, 0.f);
}

void FSTE_Threat::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	auto& Data = Context.GetInstanceData<FSTE_ThreatInstanceData>(*this);

	ASoldierAIController* AI = GetControllerFromContext(Context);
	if (!AI)
	{
		Data.PrimaryThreat = nullptr;
		Data.ThreatConfidence = 0.f;
		Data.bHasVisibleEnemy = false;
		return;
	}

	float Confidence = 0.f;
	AActor* Threat = AI->GetPrimaryThreat(Confidence);

	Data.PrimaryThreat = Threat;
	Data.ThreatConfidence = Confidence;

	if (Threat)
	{
		Data.LastKnownEnemyLoc = Threat->GetActorLocation();

		// Check if currently visible (not just remembered)
		TArray<AActor*> Visible = AI->GetCurrentlyVisibleHostiles();
		Data.bHasVisibleEnemy = Visible.Contains(Threat);
	}
	else
	{
		Data.bHasVisibleEnemy = false;
		// Keep LastKnownEnemyLoc from previous frame (for "advance to last seen")
	}
}

// =====================================================================
//  SUPPRESSION EVALUATOR
// =====================================================================
void FSTE_Suppression::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	auto& Data = Context.GetInstanceData<FSTE_SuppressionInstanceData>(*this);

	ASoldierAIController* AI = GetControllerFromContext(Context);
	Data.Suppression = AI ? AI->GetSuppression() : 0.f;
}

// =====================================================================
//  HEALTH EVALUATOR
// =====================================================================
void FSTE_Health::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	auto& Data = Context.GetInstanceData<FSTE_HealthInstanceData>(*this);

	ASoldierAIController* AI = GetControllerFromContext(Context);
	if (AI)
	{
		ASoldierCharacter* Soldier = AI->GetSoldierCharacter();
		Data.HealthPercent = Soldier ? Soldier->GetHealthPercent() : 1.f;
	}
}
