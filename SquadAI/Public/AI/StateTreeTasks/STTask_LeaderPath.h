// STTask_LeaderPath.h — Task for ALeaderCharacter to follow its QuestWaypoints
#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STTask_LeaderPath.generated.h"

class UAITask_MoveTo;

USTRUCT(meta = (DisplayName = "Leader Follow Path"))
struct SQUADAI_API FSTTask_LeaderPath : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FSTTask_LeaderPath()
	{
		bShouldCallTick = true;
	}

	using FInstanceDataType = FSTTask_LeaderPathData;

	virtual const UStruct* GetInstanceDataType() const override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FSTTask_LeaderPathData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAITask_MoveTo> MoveTask = nullptr;

	int32 LastWaypointIndex = -1;
	bool bIsQuestActive = false;
};
