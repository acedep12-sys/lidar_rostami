// STTask_HoldPosition.cpp
#include "AI/StateTreeTasks/STTask_HoldPosition.h"
#include "StateTreeExecutionContext.h"

const UStruct* FSTTask_HoldPosition::GetInstanceDataType() const
{
	return FSTTask_HoldPositionData::StaticStruct();
}

EStateTreeRunStatus FSTTask_HoldPosition::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	auto& D = Context.GetInstanceData<FSTTask_HoldPositionData>(*this);
	D.Timer = 0.f;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_HoldPosition::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	auto& D = Context.GetInstanceData<FSTTask_HoldPositionData>(*this);

	if (HoldDuration > 0.f)
	{
		D.Timer += DeltaTime;
		if (D.Timer >= HoldDuration)
		{
			return EStateTreeRunStatus::Succeeded;
		}
	}

	return EStateTreeRunStatus::Running;
}
