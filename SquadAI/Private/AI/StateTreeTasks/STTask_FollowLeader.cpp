// STTask_FollowLeader.cpp — V-formation follow with stable repath + opportunistic fire
#include "AI/StateTreeTasks/STTask_FollowLeader.h"
#include "StateTreeExecutionContext.h"
#include "AI/SoldierAIController.h"
#include "Characters/SoldierCharacter.h"
#include "Characters/AllySoldier.h"
#include "Characters/LeaderCharacter.h"
#include "Components/WeaponComponent.h"
#include "Components/AimComponent.h"
#include "Tasks/AITask_MoveTo.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

const UStruct* FSTTask_FollowLeader::GetInstanceDataType() const
{
	return FSTTask_FollowLeaderData::StaticStruct();
}

// Find the leader in the world
static ALeaderCharacter* FindLeader(UWorld* World, AAllySoldier* Ally)
{
	// Check for override first
	if (Ally && Ally->LeaderOverride.IsValid())
	{
		return Cast<ALeaderCharacter>(Ally->LeaderOverride.Get());
	}

	// Auto-find
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		if (ALeaderCharacter* Leader = Cast<ALeaderCharacter>(*It))
		{
			return Leader;
		}
	}
	return nullptr;
}

// Compute V-formation slot position behind the leader
static FVector ComputeSlotPosition(ALeaderCharacter* Leader, int32 Slot, float Spacing)
{
	const FVector LeaderLoc = Leader->GetActorLocation();
	const FVector LeaderFwd = Leader->GetActorForwardVector();
	const FVector LeaderRight = Leader->GetActorRightVector();

	// V-formation: alternating left/right, increasingly behind
	const int32 Row = (Slot / 2) + 1;
	const float Side = (Slot % 2 == 0) ? 1.f : -1.f;

	return LeaderLoc
		- LeaderFwd * (Row * Spacing * 0.8f)     // Behind
		+ LeaderRight * (Side * Row * Spacing * 0.6f); // To the side
}

EStateTreeRunStatus FSTTask_FollowLeader::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	auto& D = Context.GetInstanceData<FSTTask_FollowLeaderData>(*this);
	D.MoveTask = nullptr;
	D.LastTargetSlot = FVector::ZeroVector;
	D.TimeSinceRepath = MinRepathInterval; // Trigger immediate first path
	D.ShootTimer = 0.f;
	D.bMoving = false;

	return EStateTreeRunStatus::Running; // Runs continuously
}

EStateTreeRunStatus FSTTask_FollowLeader::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	auto& D = Context.GetInstanceData<FSTTask_FollowLeaderData>(*this);

	AActor* Owner = Cast<AActor>(Context.GetOwner());
	APawn* Pawn = Cast<APawn>(Owner);
	if (!Pawn) return EStateTreeRunStatus::Failed;

	ASoldierAIController* AI = Cast<ASoldierAIController>(Pawn->GetController());
	ASoldierCharacter* Soldier = Cast<ASoldierCharacter>(Pawn);
	AAllySoldier* Ally = Cast<AAllySoldier>(Pawn);
	if (!AI || !Soldier) return EStateTreeRunStatus::Failed;

	UWorld* World = Pawn->GetWorld();
	ALeaderCharacter* Leader = FindLeader(World, Ally);
	
	// ---- SAFETY FIX: Wait if leader is temporarily missing ----
	if (!Leader) return EStateTreeRunStatus::Running; 

	// ---- MOVEMENT ----
	D.TimeSinceRepath += DeltaTime;

	const int32 Slot = Ally ? Ally->FormationSlot : FormationSlot;
	const FVector DesiredSlot = ComputeSlotPosition(Leader, Slot, SlotSpacing);
	const float SlotDrift = FVector::Dist(DesiredSlot, D.LastTargetSlot);
	const float DistToSlot = FVector::Dist(Pawn->GetActorLocation(), DesiredSlot);

	// Repath only when: slot has drifted significantly AND enough time has passed
	bool bShouldRepath = (SlotDrift > RepathThreshold || !D.bMoving)
		&& D.TimeSinceRepath >= MinRepathInterval
		&& DistToSlot > AcceptanceRadius;

	if (bShouldRepath)
	{
		// Cancel old move
		if (D.MoveTask)
		{
			D.MoveTask->ExternalCancel();
			D.MoveTask = nullptr;
		}

		// Make sure not crouching while following
		Soldier->ClearCover();

		D.MoveTask = UAITask_MoveTo::AIMoveTo(AI, DesiredSlot, nullptr,
			AcceptanceRadius);

		if (D.MoveTask)
		{
			D.MoveTask->ReadyForActivation();
			D.bMoving = true;
		}

		D.LastTargetSlot = DesiredSlot;
		D.TimeSinceRepath = 0.f;
	}

	// Check if arrived
	if (DistToSlot <= AcceptanceRadius && D.bMoving)
	{
		if (D.MoveTask)
		{
			D.MoveTask->ExternalCancel();
			D.MoveTask = nullptr;
		}
		D.bMoving = false;
	}

	// ---- SHOOTING ----
	if (bShootWhileFollowing)
	{
		float Confidence = 0.f;
		AActor* Threat = AI->GetPrimaryThreat(Confidence);

		if (Threat && Confidence > 0.3f)
		{
			D.ShootTimer += DeltaTime;
			if (D.ShootTimer >= ShootCooldown)
			{
				D.ShootTimer = 0.f;

				// Aim and fire (with terrible ally accuracy)
				if (Soldier->AimComp)
				{
					Soldier->AimComp->SetAimTarget(Threat->GetActorLocation());
				}
				if (Soldier->WeaponComp)
				{
					Soldier->WeaponComp->StartBurst(Threat->GetActorLocation());
				}
			}
		}
		else
		{
			if (Soldier->WeaponComp && Soldier->WeaponComp->CurrentState == EWeaponState::Firing)
			{
				Soldier->WeaponComp->StopFiring();
			}
			if (Soldier->AimComp) Soldier->AimComp->ClearAimTarget();
			D.ShootTimer = 0.f;
		}
	}

	return EStateTreeRunStatus::Running; // Never completes — runs until state transition
}

void FSTTask_FollowLeader::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	auto& D = Context.GetInstanceData<FSTTask_FollowLeaderData>(*this);

	if (D.MoveTask)
	{
		D.MoveTask->ExternalCancel();
		D.MoveTask = nullptr;
	}

	AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		if (ASoldierCharacter* S = Cast<ASoldierCharacter>(Pawn))
		{
			if (S->WeaponComp) S->WeaponComp->StopFiring();
			if (S->AimComp) S->AimComp->ClearAimTarget();
		}
	}
}
