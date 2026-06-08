// STTask_TakeCover.cpp
#include "AI/StateTreeTasks/STTask_TakeCover.h"
#include "StateTreeExecutionContext.h"
#include "AI/SoldierAIController.h"
#include "Characters/SoldierCharacter.h"
#include "CoverSystem/CoverSystemSubsystem.h"
#include "Tasks/AITask_MoveTo.h"

const UStruct* FSTTask_TakeCover::GetInstanceDataType() const { return FSTTask_TakeCoverData::StaticStruct(); }

EStateTreeRunStatus FSTTask_TakeCover::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const {
	auto& D = Context.GetInstanceData<FSTTask_TakeCoverData>(*this);
	D.Phase = FSTTask_TakeCoverData::EPhase::RequestingCover; D.WaitTimer = 0.f; D.MoveTask = nullptr; D.bReserved = false;
	ASoldierAIController* AI = Cast<ASoldierAIController>(Context.GetOwner());
	if (!AI || !AI->GetPawn()) return EStateTreeRunStatus::Failed;
	float Confidence = 0.f; AActor* Threat = AI->GetPrimaryThreat(Confidence);
	FVector ThreatLoc = Threat ? Threat->GetActorLocation() : AI->GetPawn()->GetActorLocation() + FVector(1000, 0, 0);
	AI->bHasPendingCover = false; AI->RequestCover(ThreatLoc, SearchRadius);
	D.Phase = FSTTask_TakeCoverData::EPhase::WaitingForResult;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_TakeCover::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const {
	auto& D = Context.GetInstanceData<FSTTask_TakeCoverData>(*this);
	ASoldierAIController* AI = Cast<ASoldierAIController>(Context.GetOwner());
	if (!AI || !AI->GetPawn()) return EStateTreeRunStatus::Failed;
	APawn* Pawn = AI->GetPawn();
	ASoldierCharacter* Soldier = Cast<ASoldierCharacter>(Pawn);
	if (!Soldier) return EStateTreeRunStatus::Failed;

	switch (D.Phase) {
	case FSTTask_TakeCoverData::EPhase::WaitingForResult:
		D.WaitTimer += DeltaTime;
		if (AI->bHasPendingCover) {
			const FCoverPoint& Cover = AI->PendingCoverResult;
			UCoverSystemSubsystem* CoverSys = AI->GetWorld()->GetSubsystem<UCoverSystemSubsystem>();
			if (CoverSys) {
				if (CoverSys->ReserveCover(Cover.GetChunkKey(), Cover.LocalID, Pawn)) {
					D.ReservedChunkKey = Cover.GetChunkKey(); D.ReservedLocalID = Cover.LocalID; D.bReserved = true;
				}
			}
			Soldier->SetCurrentCover(Cover); AI->CurrentCover = Cover; AI->bHasCover = true;
			D.MoveTask = UAITask_MoveTo::AIMoveTo(AI, Cover.Location, nullptr, 80.f, EAIOptionFlag::Default, EAIOptionFlag::Enable);
			if (D.MoveTask) { D.MoveTask->ReadyForActivation(); D.Phase = FSTTask_TakeCoverData::EPhase::Moving; }
			else return EStateTreeRunStatus::Running;
		}
		if (D.WaitTimer > 1.5f) return EStateTreeRunStatus::Failed;
		break;
	case FSTTask_TakeCoverData::EPhase::Moving:
		if (!D.MoveTask) return EStateTreeRunStatus::Failed;
		if (D.MoveTask->IsFinished()) {
			if (D.MoveTask->WasMoveSuccessful()) {
				Soldier->SetCoverState(true, AI->CurrentCover.Height == ECoverHeight::Low);
				Pawn->SetActorRotation(AI->CurrentCover.Normal.Rotation());
				D.Phase = FSTTask_TakeCoverData::EPhase::Arrived; return EStateTreeRunStatus::Succeeded;
			} else return EStateTreeRunStatus::Failed;
		}
		break;
	case FSTTask_TakeCoverData::EPhase::Arrived: return EStateTreeRunStatus::Succeeded;
	default: return EStateTreeRunStatus::Failed;
	}
	return EStateTreeRunStatus::Running;
}

void FSTTask_TakeCover::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const {
	auto& D = Context.GetInstanceData<FSTTask_TakeCoverData>(*this);
	if (D.MoveTask) { D.MoveTask->ExternalCancel(); D.MoveTask = nullptr; }
	ASoldierAIController* AI = Cast<ASoldierAIController>(Context.GetOwner());
	if (D.bReserved && AI) {
		if (UCoverSystemSubsystem* CoverSys = AI->GetWorld()->GetSubsystem<UCoverSystemSubsystem>())
			CoverSys->ReleaseCover(D.ReservedChunkKey, D.ReservedLocalID);
		D.bReserved = false;
	}
	if (AI && AI->GetPawn()) if (ASoldierCharacter* Soldier = Cast<ASoldierCharacter>(AI->GetPawn())) Soldier->ClearCover();
}
