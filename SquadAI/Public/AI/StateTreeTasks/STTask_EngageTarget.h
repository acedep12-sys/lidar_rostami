// STTask_EngageTarget.h — Peek/fire/cooldown cycle, suppression-gated
// Refuses to peek while suppression > threshold
#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STTask_EngageTarget.generated.h"

UENUM()
enum class EEngagePhase : uint8 { Waiting, Peeking, Cooldown };

USTRUCT(meta = (DisplayName = "Engage Target"))
struct SQUADAI_API FSTTask_EngageTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FSTTask_EngageTarget()
	{
		bShouldCallTick = true;
	}

	using FInstanceDataType = FSTTask_EngageTargetData;

	UPROPERTY(EditAnywhere, Category = "Engage")
	int32 ShotsPerBurst = 3;

	// Random range for peek duration (makes each peek feel different)
	UPROPERTY(EditAnywhere, Category = "Engage")
	float PeekDurationMin = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Engage")
	float PeekDurationMax = 2.0f;

	// Random range for cooldown between peeks
	UPROPERTY(EditAnywhere, Category = "Engage")
	float CooldownMin = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Engage")
	float CooldownMax = 1.5f;

	// How many peek cycles before this task succeeds (triggers advance evaluation)
	UPROPERTY(EditAnywhere, Category = "Engage")
	int32 MaxCycles = 3;

	virtual const UStruct* GetInstanceDataType() const override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FSTTask_EngageTargetData
{
	GENERATED_BODY()

	EEngagePhase Phase = EEngagePhase::Waiting;
	float PhaseTimer = 0.f;
	int32 CyclesCompleted = 0;
	float CurrentPeekDuration = 1.4f;  // Randomized per cycle
	float CurrentCooldownDuration = 0.7f;
};
