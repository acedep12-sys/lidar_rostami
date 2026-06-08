// STTask_EngageTarget.cpp — Suppression-gated peek/fire/cooldown
#include "AI/StateTreeTasks/STTask_EngageTarget.h"
#include "StateTreeExecutionContext.h"
#include "AI/SoldierAIController.h"
#include "Characters/SoldierCharacter.h"
#include "Characters/EnemySoldier.h"
#include "Components/WeaponComponent.h"
#include "Components/AimComponent.h"

const UStruct* FSTTask_EngageTarget::GetInstanceDataType() const
{
	return FSTTask_EngageTargetData::StaticStruct();
}

EStateTreeRunStatus FSTTask_EngageTarget::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	auto& D = Context.GetInstanceData<FSTTask_EngageTargetData>(*this);
	D.Phase = EEngagePhase::Waiting;
	D.PhaseTimer = 0.f;
	D.CyclesCompleted = 0;
	D.CurrentPeekDuration = FMath::RandRange(PeekDurationMin, PeekDurationMax);
	D.CurrentCooldownDuration = FMath::RandRange(CooldownMin, CooldownMax);

	// Start crouched in cover
	ASoldierAIController* AI = Cast<ASoldierAIController>(Context.GetOwner());
	if (AI && AI->GetPawn())
	{
		if (ASoldierCharacter* S = Cast<ASoldierCharacter>(AI->GetPawn()))
		{
			S->SetCoverState(true, true);
		}
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTTask_EngageTarget::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	auto& D = Context.GetInstanceData<FSTTask_EngageTargetData>(*this);

	ASoldierAIController* AI = Cast<ASoldierAIController>(Context.GetOwner());
	if (!AI || !AI->GetPawn()) return EStateTreeRunStatus::Failed;

	ASoldierCharacter* Soldier = Cast<ASoldierCharacter>(AI->GetPawn());
	if (!Soldier) return EStateTreeRunStatus::Failed;

	// No threat? Succeed out so StateTree can transition
	float Confidence = 0.f;
	AActor* Threat = AI->GetPrimaryThreat(Confidence);
	if (!Threat)
	{
		if (Soldier->WeaponComp) Soldier->WeaponComp->StopFiring();
		return EStateTreeRunStatus::Succeeded;
	}

	// Get suppression threshold from enemy config
	float SupThreshold = 0.6f;
	if (AEnemySoldier* Enemy = Cast<AEnemySoldier>(AI->GetPawn()))
	{
		SupThreshold = Enemy->SuppressionThreshold;
	}

	D.PhaseTimer += DeltaTime;

	switch (D.Phase)
	{
	// ---- WAITING (behind cover) ----
	case EEngagePhase::Waiting:
	{
		// Suppression gate — refuse to peek while suppressed
		if (AI->GetSuppression() > SupThreshold)
		{
			D.PhaseTimer = 0.f; // Reset timer — stay hidden
			return EStateTreeRunStatus::Running;
		}

		// Wait before peeking (randomized cooldown duration)
		if (D.PhaseTimer >= D.CurrentCooldownDuration)
		{
			// Peek!
			D.Phase = EEngagePhase::Peeking;
			D.PhaseTimer = 0.f;

			Soldier->SetCoverState(true, false); // Stand up in cover

			// Aim at threat
			if (Soldier->AimComp)
			{
				Soldier->AimComp->SetAimTarget(Threat->GetActorLocation());
			}

			// Start burst
			if (Soldier->WeaponComp)
			{
				Soldier->WeaponComp->BurstShots = ShotsPerBurst;
				Soldier->WeaponComp->StartBurst(Threat->GetActorLocation());
			}
		}
		break;
	}

	// ---- PEEKING (exposed, firing) ----
	case EEngagePhase::Peeking:
	{
		// Update aim to track moving threat
		if (Soldier->AimComp && Threat)
		{
			Soldier->AimComp->SetAimTarget(Threat->GetActorLocation());
		}

		// Update burst target
		if (Soldier->WeaponComp && Threat)
		{
			Soldier->WeaponComp->StartBurst(Threat->GetActorLocation());
		}

		// Time's up or suppressed mid-peek → duck
		if (D.PhaseTimer >= D.CurrentPeekDuration || AI->GetSuppression() > SupThreshold * 1.2f)
		{
			D.Phase = EEngagePhase::Cooldown;
			D.PhaseTimer = 0.f;

			// Duck back
			if (Soldier->WeaponComp) Soldier->WeaponComp->StopFiring();
			Soldier->SetCoverState(true, true);
			Soldier->AimComp->ClearAimTarget();
			D.CyclesCompleted++;

			// Randomize durations for next cycle (each peek feels different)
			D.CurrentPeekDuration = FMath::RandRange(PeekDurationMin, PeekDurationMax);
			D.CurrentCooldownDuration = FMath::RandRange(CooldownMin, CooldownMax);
		}
		break;
	}

	// ---- COOLDOWN (behind cover, waiting) ----
	case EEngagePhase::Cooldown:
	{
		if (D.PhaseTimer >= D.CurrentCooldownDuration)
		{
			if (D.CyclesCompleted >= MaxCycles)
			{
				return EStateTreeRunStatus::Succeeded; // Done — StateTree decides what's next
			}

			D.Phase = EEngagePhase::Waiting;
			D.PhaseTimer = 0.f;
		}
		break;
	}
	}

	return EStateTreeRunStatus::Running;
}

void FSTTask_EngageTarget::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	ASoldierAIController* AI = Cast<ASoldierAIController>(Context.GetOwner());
	if (AI && AI->GetPawn())
	{
		if (ASoldierCharacter* S = Cast<ASoldierCharacter>(AI->GetPawn()))
		{
			if (S->WeaponComp) S->WeaponComp->StopFiring();
			S->SetCoverState(true, true);
			if (S->AimComp) S->AimComp->ClearAimTarget();
		}
	}
}
