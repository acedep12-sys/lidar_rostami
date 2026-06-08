// STTask_FollowLeader.h — V-formation follow with stable slot + repath threshold
// Only repaths when slot drifts beyond threshold — no "leader's foot moved" stutter
#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STTask_FollowLeader.generated.h"

class UAITask_MoveTo;

USTRUCT(meta = (DisplayName = "Follow Leader"))
struct SQUADAI_API FSTTask_FollowLeader : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FSTTask_FollowLeader()
	{
		bShouldCallTick = true;
	}

	using FInstanceDataType = FSTTask_FollowLeaderData;

	UPROPERTY(EditAnywhere, Category = "Follow")
	int32 FormationSlot = 0;

	UPROPERTY(EditAnywhere, Category = "Follow")
	float SlotSpacing = 300.f;

	UPROPERTY(EditAnywhere, Category = "Follow")
	float RepathThreshold = 400.f;

	UPROPERTY(EditAnywhere, Category = "Follow")
	float AcceptanceRadius = 250.f;

	// Minimum time between repaths (prevents flutter)
	UPROPERTY(EditAnywhere, Category = "Follow")
	float MinRepathInterval = 0.5f;

	// Fire at enemies while following?
	UPROPERTY(EditAnywhere, Category = "Follow")
	bool bShootWhileFollowing = true;

	UPROPERTY(EditAnywhere, Category = "Follow")
	float ShootCooldown = 2.5f;

	virtual const UStruct* GetInstanceDataType() const override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FSTTask_FollowLeaderData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAITask_MoveTo> MoveTask = nullptr;

	FVector LastTargetSlot = FVector::ZeroVector;
	float TimeSinceRepath = 0.f;
	float ShootTimer = 0.f;
	float TimeStuck = 0.f;
	bool bMoving = false;
};
