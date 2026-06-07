// STTask_TakeCover.cpp — Async cover → UAITask_MoveTo → crouch
#include "AI/StateTreeTasks/STTask_TakeCover.h"
#include "StateTreeExecutionContext.h"
#include "AI/SoldierAIController.h"
#include "Characters/SoldierCharacter.h"
#include "CoverSystem/CoverSystemSubsystem.h"
#include "Tasks/AITask_MoveTo.h"

const UStruct* FSTTask_TakeCover::GetInstanceDataType() const
{
	return FSTTask_TakeCoverData::StaticStruct();
}

EStateTreeRunStatus FSTTask_TakeCover::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	auto& D = Context.GetInstanceData<FSTTask_TakeCoverData>(*this);
	D.Phase = FSTTask_TakeCoverData::EPhase::RequestingCover;
	D.WaitTimer = 0.f;
	D.MoveTask = nullptr;
	D.bReserved = false;

	// Get controller (correct 5.7 API)
	AActor* OwnerActor = Cast<AActor>(Context.GetOwner());
	APawn* Pawn = Cast<APawn>(OwnerActor);
	if (!Pawn) return EStateTreeRunStatus::Failed;

	ASoldierAIController* AI = Cast<ASoldierAIController>(Pawn->GetController());
	if (!AI) return EStateTreeRunStatus::Failed;

	// Request cover — result stored on controller (NOT instance data)
	float Confidence = 0.f;
	AActor* Threat = AI->GetPrimaryThreat(Confidence);
	FVector ThreatLoc = Threat ? Threat->GetActorLocation() : AI->GetPawn()->GetActorLocation() + FVector(1000, 0, 0);

	AI->bHasPendingCover = false;
	AI->RequestCover(ThreatLoc, SearchRadius);

	D.Phase = FSTTask_TakeCoverData::EPhase::WaitingForResult;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_TakeCover::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	auto& D = Context.GetInstanceData<FSTTask_TakeCoverData>(*this);

	AActor* OwnerActor = Cast<AActor>(Context.GetOwner());
	APawn* Pawn = Cast<APawn>(OwnerActor);
	if (!Pawn) return EStateTreeRunStatus::Failed;

	ASoldierAIController* AI = Cast<ASoldierAIController>(Pawn->GetController());
	if (!AI) return EStateTreeRunStatus::Failed;

	ASoldierCharacter* Soldier = AI->GetSoldierCharacter();
	if (!Soldier) return EStateTreeRunStatus::Failed;

	switch (D.Phase)
	{
	// ---- WAITING FOR ASYNC COVER RESULT ----
	case FSTTask_TakeCoverData::EPhase::WaitingForResult:
	{
		D.WaitTimer += DeltaTime;

		// Poll the controller for the result (safe — stored on long-lived UObject)
		if (AI->bHasPendingCover)
		{
			const FCoverPoint& Cover = AI->PendingCoverResult;

			// Reserve the cover point
			UCoverSystemSubsystem* CoverSys = Pawn->GetWorld()->GetSubsystem<UCoverSystemSubsystem>();
			if (CoverSys)
			{
				FIntPoint ChunkKey = Cover.GetChunkKey();
				if (CoverSys->ReserveCover(ChunkKey, Cover.LocalID, Pawn))
				{
					D.ReservedChunkKey = ChunkKey;
					D.ReservedLocalID = Cover.LocalID;
					D.bReserved = true;
				}
			}

			// Store cover on character
			Soldier->SetCurrentCover(Cover);
			AI->CurrentCover = Cover;
			AI->bHasCover = true;

			// Create UAITask_MoveTo (created ONCE, cancelled in ExitState)
			// Location-based move: pass nullptr for TargetActor
			D.MoveTask = UAITask_MoveTo::AIMoveTo(
				AI,               // AIController
				Cover.Location,   // Destination
				nullptr,          // TargetActor (nullptr = location-based)
				80.f,             // AcceptanceRadius
				EAIOptionFlag::Default, // StopOnOverlap
				EAIOptionFlag::Default, // AcceptPartialPath
				true,             // bUsePathfinding
				true              // bLockAILogic
			);

			if (D.MoveTask)
			{
				D.MoveTask->ReadyForActivation();
				D.Phase = FSTTask_TakeCoverData::EPhase::Moving;
			}
			else
			{
				return EStateTreeRunStatus::Failed;
			}
		}

		// Timeout
		if (D.WaitTimer > 1.5f)
		{
			return EStateTreeRunStatus::Failed;
		}

		return EStateTreeRunStatus::Running;
	}

	// ---- MOVING TO COVER ----
	case FSTTask_TakeCoverData::EPhase::Moving:
	{
		if (!D.MoveTask) return EStateTreeRunStatus::Failed;

		// Check move task status
		if (D.MoveTask->IsFinished())
		{
			if (D.MoveTask->WasMoveSuccessful())
			{
				// Arrived — crouch if low cover
				const bool bLow = AI->CurrentCover.Height == ECoverHeight::Low;
				Soldier->SetCoverState(true, bLow);

				// Face the cover normal (toward threat side)
				Pawn->SetActorRotation(AI->CurrentCover.Normal.Rotation());

				D.Phase = FSTTask_TakeCoverData::EPhase::Arrived;
				return EStateTreeRunStatus::Succeeded;
			}
			else
			{
				return EStateTreeRunStatus::Failed;
			}
		}

		return EStateTreeRunStatus::Running;
	}

	case FSTTask_TakeCoverData::EPhase::Arrived:
		return EStateTreeRunStatus::Succeeded;

	default:
		return EStateTreeRunStatus::Failed;
	}
}

void FSTTask_TakeCover::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	auto& D = Context.GetInstanceData<FSTTask_TakeCoverData>(*this);

	// Cancel any in-flight move (prevents leaked path requests)
	if (D.MoveTask)
	{
		D.MoveTask->ExternalCancel();
		D.MoveTask = nullptr;
	}

	// Release cover reservation
	if (D.bReserved)
	{
		AActor* OwnerActor = Cast<AActor>(Context.GetOwner());
		APawn* Pawn = Cast<APawn>(OwnerActor);
		if (Pawn)
		{
			UCoverSystemSubsystem* CoverSys = Pawn->GetWorld()->GetSubsystem<UCoverSystemSubsystem>();
			if (CoverSys)
			{
				CoverSys->ReleaseCover(D.ReservedChunkKey, D.ReservedLocalID);
			}
		}
		D.bReserved = false;
	}

	// Stand up
	if (AActor* OwnerActor = Cast<AActor>(Context.GetOwner()))
	{
		if (APawn* Pawn = Cast<APawn>(OwnerActor))
		{
			if (ASoldierCharacter* Soldier = Cast<ASoldierCharacter>(Pawn))
			{
				Soldier->ClearCover();
			}
		}
	}
}
