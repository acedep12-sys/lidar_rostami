// STTask_HoldPosition.h — Wait in place, optionally for a duration
#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STTask_HoldPosition.generated.h"

USTRUCT(meta = (DisplayName = "Hold Position"))
struct SQUADAI_API FSTTask_HoldPosition : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FSTTask_HoldPosition() { bShouldCallTick = true; }

	using FInstanceDataType = FSTTask_HoldPositionData;

	// 0 = hold indefinitely until state transitions
	UPROPERTY(EditAnywhere, Category = "Hold")
	float HoldDuration = 0.f;

	virtual const UStruct* GetInstanceDataType() const override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT()
struct FSTTask_HoldPositionData
{
	GENERATED_BODY()
	float Timer = 0.f;
};
