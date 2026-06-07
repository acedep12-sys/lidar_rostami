#include "AI/StateTreeTasks/STTask_HoldPosition.h"
#include "AI/SoldierAIController.h"
#include "Characters/SoldierCharacter.h"
#include "Components/AimComponent.h"
#include "StateTreeExecutionContext.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"

const UStruct* FSTTask_HoldPosition::GetInstanceDataType() const
{
	return FSTTask_HoldPositionData::StaticStruct();
}

EStateTreeRunStatus FSTTask_HoldPosition::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	auto& D = Context.GetInstanceData<FSTTask_HoldPositionData>(*this);
	D.Timer = 0.f;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_HoldPosition::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	auto& D = Context.GetInstanceData<FSTTask_HoldPositionData>(*this);

	// ---- AUTOMATION: Look at threat even while idling ----
	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		if (ASoldierAIController* AI = Cast<ASoldierAIController>(Pawn->GetController()))
		{
			float Confidence = 0.f;
			if (AActor* Threat = AI->GetPrimaryThreat(Confidence))
			{
				if (ASoldierCharacter* Soldier = Cast<ASoldierCharacter>(Pawn))
				{
					if (Soldier->AimComp) Soldier->AimComp->SetAimTarget(Threat->GetActorLocation());
				}
			}
		}
	}

	if (HoldDuration > 0.f)
	{
		D.Timer += DeltaTime;
		if (D.Timer >= HoldDuration)
		{
			return EStateTreeRunStatus::Succeeded;
		}
	}

	return EStateTreeRunStatus::Running;
}
