// STTask_LeaderPath.cpp
#include "AI/StateTreeTasks/STTask_LeaderPath.h"
#include "AI/SoldierAIController.h"
#include "Characters/LeaderCharacter.h"
#include "Quest/QuestRegistry.h"
#include "Tasks/AITask_MoveTo.h"
#include "StateTreeExecutionContext.h"

const UStruct* FSTTask_LeaderPath::GetInstanceDataType() const { return FSTTask_LeaderPathData::StaticStruct(); }

EStateTreeRunStatus FSTTask_LeaderPath::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const {
	auto& D = Context.GetInstanceData<FSTTask_LeaderPathData>(*this);
	D.MoveTask = nullptr; D.LastTargetLoc = FVector::ZeroVector; D.TimeSinceRepath = 0.5f; return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_LeaderPath::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const {
	auto& D = Context.GetInstanceData<FSTTask_LeaderPathData>(*this);
	ASoldierAIController* AI = Cast<ASoldierAIController>(Context.GetOwner());
	if (!AI || !AI->GetPawn()) return EStateTreeRunStatus::Failed;

	D.TimeSinceRepath += DeltaTime;
	FVector GoalLoc = AI->CurrentGoalLocation;

	if (GoalLoc.IsNearlyZero()) {
		if (UQuestRegistry* GPS = AI->GetWorld()->GetSubsystem<UQuestRegistry>()) {
			GoalLoc = GPS->CurrentActiveGoal;
		}
	}

	if (GoalLoc.IsNearlyZero()) return EStateTreeRunStatus::Running;

	if (D.MoveTask && !D.MoveTask->IsFinished() && FVector::DistSquared(GoalLoc, D.LastTargetLoc) < 10000.f) return EStateTreeRunStatus::Running;

	if (D.MoveTask) { D.MoveTask->ExternalCancel(); D.MoveTask = nullptr; }

	D.MoveTask = UAITask_MoveTo::AIMoveTo(AI, GoalLoc, nullptr, 500.f, EAIOptionFlag::Default, EAIOptionFlag::Enable);
	if (D.MoveTask) D.MoveTask->ReadyForActivation();
	
	D.LastTargetLoc = GoalLoc;
	D.TimeSinceRepath = 0.f;
	return EStateTreeRunStatus::Running;
}

void FSTTask_LeaderPath::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const {
	auto& D = Context.GetInstanceData<FSTTask_LeaderPathData>(*this);
	if (D.MoveTask) { D.MoveTask->ExternalCancel(); D.MoveTask = nullptr; }
}
