// STTask_LeaderPath.cpp — Leader movement following QuestWaypoints with pacing sync
#include "AI/StateTreeTasks/STTask_LeaderPath.h"
#include "AI/SoldierAIController.h"
#include "StateTreeExecutionContext.h"
#include "Characters/LeaderCharacter.h"
#include "Tasks/AITask_MoveTo.h"

const UStruct* FSTTask_LeaderPath::GetInstanceDataType() const
{
	return FSTTask_LeaderPathData::StaticStruct();
}

EStateTreeRunStatus FSTTask_LeaderPath::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	auto& D = Context.GetInstanceData<FSTTask_LeaderPathData>(*this);
	D.MoveTask = nullptr;
	D.LastWaypointIndex = -1;

	AActor* Owner = Cast<AActor>(Context.GetOwner());
	APawn* Pawn = Cast<APawn>(Owner);
	if (!Pawn) return EStateTreeRunStatus::Failed;

	ALeaderCharacter* Leader = Cast<ALeaderCharacter>(Pawn);
	if (Leader)
	{
		Leader->BeginQuest();
		D.bIsQuestActive = true;
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_LeaderPath::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	auto& D = Context.GetInstanceData<FSTTask_LeaderPathData>(*this);

	AActor* Owner = Cast<AActor>(Context.GetOwner());
	APawn* Pawn = Cast<APawn>(Owner);
	if (!Pawn) return EStateTreeRunStatus::Failed;

	ALeaderCharacter* Leader = Cast<ALeaderCharacter>(Pawn);
	ASoldierAIController* AI = Cast<ASoldierAIController>(Pawn->GetController());
	if (!Leader || !AI) return EStateTreeRunStatus::Failed;

	if (Leader->IsQuestComplete())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// Update waypoint logic
	const int32 CurrentIdx = Leader->CurrentWaypointIndex;
	const FVector TargetLoc = Leader->GetCurrentWaypointLocation();

	// If waypoint changed or we don't have a task, start moving
	bool bNeedsMove = (CurrentIdx != D.LastWaypointIndex) || (D.MoveTask == nullptr);

	if (bNeedsMove && Leader->WaypointState == ELeaderWaypointState::Moving)
	{
		if (D.MoveTask)
		{
			D.MoveTask->ExternalCancel();
			D.MoveTask = nullptr;
		}

		D.MoveTask = UAITask_MoveTo::AIMoveTo(AI, TargetLoc, nullptr, 100.f);
		if (D.MoveTask)
		{
			D.MoveTask->ReadyForActivation();
		}
		D.LastWaypointIndex = CurrentIdx;
	}

	// If we are waiting for player/clear, cancel move
	if (Leader->WaypointState != ELeaderWaypointState::Moving && D.MoveTask)
	{
		D.MoveTask->ExternalCancel();
		D.MoveTask = nullptr;
	}

	return EStateTreeRunStatus::Running;
}

void FSTTask_LeaderPath::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	auto& D = Context.GetInstanceData<FSTTask_LeaderPathData>(*this);
	if (D.MoveTask)
	{
		D.MoveTask->ExternalCancel();
		D.MoveTask = nullptr;
	}
}
