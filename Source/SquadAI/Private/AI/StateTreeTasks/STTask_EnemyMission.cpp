// STTask_EnemyMission.cpp
#include "AI/StateTreeTasks/STTask_EnemyMission.h"
#include "AI/SoldierAIController.h"
#include "Characters/EnemySoldier.h"
#include "Quest/QuestRegistry.h"
#include "Tasks/AITask_MoveTo.h"
#include "StateTreeExecutionContext.h"

const UStruct* FSTTask_EnemyMission::GetInstanceDataType() const { return FSTTask_EnemyMissionData::StaticStruct(); }

EStateTreeRunStatus FSTTask_EnemyMission::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const {
	auto& D = Context.GetInstanceData<FSTTask_EnemyMissionData>(*this);
	D.MoveTask = nullptr; D.LastGoalLoc = FVector::ZeroVector; D.TimeSinceRepath = 1.0f; return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_EnemyMission::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const {
	auto& D = Context.GetInstanceData<FSTTask_EnemyMissionData>(*this);
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

	if (D.MoveTask && !D.MoveTask->IsFinished() && FVector::DistSquared(GoalLoc, D.LastGoalLoc) < 10000.f) return EStateTreeRunStatus::Running;

	if (D.MoveTask) { D.MoveTask->ExternalCancel(); D.MoveTask = nullptr; }
	
	D.MoveTask = UAITask_MoveTo::AIMoveTo(AI, GoalLoc, nullptr, 500.f, EAIOptionFlag::Default, EAIOptionFlag::Enable);
	if (D.MoveTask) D.MoveTask->ReadyForActivation();
	
	D.LastGoalLoc = GoalLoc;
	D.TimeSinceRepath = 0.f;
	return EStateTreeRunStatus::Running;
}

void FSTTask_EnemyMission::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const {
	auto& D = Context.GetInstanceData<FSTTask_EnemyMissionData>(*this);
	if (D.MoveTask) { D.MoveTask->ExternalCancel(); D.MoveTask = nullptr; }
}
