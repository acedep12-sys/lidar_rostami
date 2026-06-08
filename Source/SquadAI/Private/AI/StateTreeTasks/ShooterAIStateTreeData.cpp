// ShooterAIStateTreeData.cpp — StateTree evaluators
#include "AI/StateTreeTasks/ShooterAIStateTreeData.h"
#include "StateTreeExecutionContext.h"
#include "AI/SoldierAIController.h"
#include "Characters/SoldierCharacter.h"
#include "Components/HealthComponent.h"
#include "AIController.h"

// Helper: get controller from execution context
static ASoldierAIController* GetControllerFromContext(FStateTreeExecutionContext& Context)
{
	ASoldierAIController* AIC = Cast<ASoldierAIController>(Context.GetOwner());
	if (AIC) return AIC;

	if (APawn* Pawn = Cast<APawn>(Context.GetOwner()))
	{
		return Cast<ASoldierAIController>(Pawn->GetController());
	}
	return nullptr;
}

void FSTE_Threat::TreeStart(FStateTreeExecutionContext& Context) const { Tick(Context, 0.f); }

void FSTE_Threat::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	auto& Data = Context.GetInstanceData<FSTE_ThreatInstanceData>(*this);
	ASoldierAIController* AI = GetControllerFromContext(Context);
	if (!AI) { Data.PrimaryThreat = nullptr; Data.ThreatConfidence = 0.f; Data.bHasVisibleEnemy = false; return; }

	float Confidence = 0.f;
	AActor* Threat = AI->GetPrimaryThreat(Confidence);
	Data.PrimaryThreat = Threat;
	Data.ThreatConfidence = Confidence;

	if (Threat) {
		Data.LastKnownEnemyLoc = Threat->GetActorLocation();
		TArray<AActor*> Visible = AI->GetCurrentlyVisibleHostiles();
		Data.bHasVisibleEnemy = Visible.Contains(Threat);
	} else {
		Data.bHasVisibleEnemy = false;
	}
}

void FSTE_Suppression::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	auto& Data = Context.GetInstanceData<FSTE_SuppressionInstanceData>(*this);
	ASoldierAIController* AI = GetControllerFromContext(Context);
	Data.Suppression = AI ? AI->GetSuppression() : 0.f;
}

void FSTE_Health::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	auto& Data = Context.GetInstanceData<FSTE_HealthInstanceData>(*this);
	ASoldierAIController* AI = GetControllerFromContext(Context);
	if (AI) {
		ASoldierCharacter* Soldier = AI->GetSoldierCharacter();
		Data.HealthPercent = Soldier ? Soldier->GetHealthPercent() : 1.f;
	}
}
