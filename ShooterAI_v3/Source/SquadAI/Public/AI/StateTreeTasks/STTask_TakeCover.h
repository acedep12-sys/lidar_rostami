// STTask_TakeCover.h — StateTree Task: async cover query → move to cover → crouch
// Uses UAITask_MoveTo (auto-cancel on ExitState). Result stored on controller.
// Correct 5.6+ API: bShouldCallTick=true (ExitState is always called)
#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STTask_TakeCover.generated.h"

class UAITask_MoveTo;

USTRUCT(meta = (DisplayName = "Take Cover"))
struct SQUADAI_API FSTTask_TakeCover : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FSTTask_TakeCover()
	{
		bShouldCallTick = true;       // 5.6+ MANDATORY — tasks don't tick by default
	}

	using FInstanceDataType = FSTTask_TakeCoverData;

	UPROPERTY(EditAnywhere, Category = "Cover")
	float SearchRadius = 1800.f;

	virtual const UStruct* GetInstanceDataType() const override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FSTTask_TakeCoverData
{
	GENERATED_BODY()

	enum class EPhase : uint8 { RequestingCover, WaitingForResult, Moving, Arrived };

	EPhase Phase = EPhase::RequestingCover;
	float WaitTimer = 0.f;

	UPROPERTY()
	TObjectPtr<UAITask_MoveTo> MoveTask = nullptr;

	// Cover reservation info (for cleanup on exit)
	FIntPoint ReservedChunkKey = FIntPoint::ZeroValue;
	int32 ReservedLocalID = -1;
	bool bReserved = false;
};
