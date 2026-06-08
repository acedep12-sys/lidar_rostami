// QuestObjective.cpp — The Direct Command Injector
#include "Quest/QuestObjective.h"
#include "Quest/QuestRegistry.h"
#include "AI/SoldierAIController.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

void UQuestObjective_AutoZone::OnStart(UWorld* World)
{
	Super::OnStart(World);
	if (!World) return;
	UQuestRegistry* Reg = World->GetSubsystem<UQuestRegistry>();
	if (!Reg) return;
	
	AActor* Found = Reg->Find(ZoneTag);
	FVector GoalLoc = Found ? Found->GetActorLocation() : FVector::ZeroVector;

	// SET THE GPS BEACON
	Reg->CurrentActiveGoal = GoalLoc;

	// ---- DIRECT COMMAND: Force-push the goal to EVERY AI brain in the world ----
	for (TActorIterator<ASoldierAIController> It(World); It; ++It)
	{
		ASoldierAIController* AIC = *It;
		if (AIC)
		{
			AIC->CurrentGoalLocation = GoalLoc;
			AIC->bHasActiveGoal = (GoalLoc.Size() > 10.f);
		}
	}

	ResolveMode(World);
}

void UQuestObjective_AutoZone::Tick(float DeltaTime, UWorld* World)
{
	if (State != EObjectiveState::Active) return;
	
	UQuestRegistry* Reg = World->GetSubsystem<UQuestRegistry>();
	if (!Reg) return;

	// Check success by Allied characters reaching the goal
	for (TActorIterator<ASoldierAIController> It(World); It; ++It)
	{
		ASoldierAIController* AIC = *It;
		if (AIC && AIC->GetPawn() && AIC->GetGenericTeamId().GetId() == 0) { // 0 = Allies/Leader
			const float DistanceToGoal = FVector::Dist(AIC->GetPawn()->GetActorLocation(), Reg->CurrentActiveGoal);
			if (DistanceToGoal < 600.f) {
				State = EObjectiveState::Succeeded;
				break;
			}
		}
	}
}

void UQuestObjective_AutoZone::OnFinish() { Super::OnFinish(); }
void UQuestObjective_AutoZone::ResolveMode(UWorld* World) { ResolvedMode = EObjectiveMode::Reach; }
