// STTask_EnemyMission.h — Task for enemies to move to assigned attack points
#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STTask_EnemyMission.generated.h"

class UAITask_MoveTo;

USTRUCT(meta = (DisplayName = "Enemy Move To Attack Point"))
struct SQUADAI_API FSTTask_EnemyMission : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FSTTask_EnemyMission() { bShouldCallTick = true; }

	using FInstanceDataType = FSTTask_EnemyMissionData;

	virtual const UStruct* GetInstanceDataType() const override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FSTTask_EnemyMissionData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAITask_MoveTo> MoveTask = nullptr;

	FVector LastGoalLoc = FVector::ZeroVector;
	float TimeSinceRepath = 0.f;
};
