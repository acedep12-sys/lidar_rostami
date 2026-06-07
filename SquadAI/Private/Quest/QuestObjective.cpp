// QuestObjective.cpp — Auto Zone Objective: Reach / Defend / Clear
#include "Quest/QuestObjective.h"
#include "Quest/QuestRegistry.h"
#include "Quest/QuestZone.h"
#include "Quest/WaveSpawner.h"
#include "Quest/AutoSpawnZone.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Characters/LeaderCharacter.h"

// =====================================================================
//  AUTO ZONE OBJECTIVE
// =====================================================================
void UQuestObjective_AutoZone::OnStart(UWorld* World)
{
	Super::OnStart(World);

	if (!World) return;

	UQuestRegistry* Reg = World->GetSubsystem<UQuestRegistry>();
	if (!Reg) return;

	// Find zone (could be a QuestZone or an AutoSpawnZone)
	AActor* Found = Reg->Find(ZoneTag);
	FVector GoalLoc = Found ? Found->GetActorLocation() : FVector::ZeroVector;

	if (AAutoSpawnZone* ASZ = Cast<AAutoSpawnZone>(Found))
	{
		CachedZone = ASZ->GetZone();
		CachedSpawner = ASZ->GetSpawner();
	}
	else if (AQuestZone* QZ = Cast<AQuestZone>(Found))
	{
		CachedZone = QZ;

		// Try to find a spawner with matching tag
		FQuestTag SpawnerTag;
		SpawnerTag.Tag = FName(*(ZoneTag.Tag.ToString() + TEXT("_Spawner")));
		if (AActor* SpawnerActor = Reg->Find(SpawnerTag))
		{
			CachedSpawner = Cast<AWaveSpawner>(SpawnerActor);
		}
	}

	// AUTOMATION: Tell the leader character where to go
	// Using a more robust iterator search
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		if (ALeaderCharacter* Leader = Cast<ALeaderCharacter>(*It))
		{
			FQuestWaypoint WP;
			WP.Location = GoalLoc;
			WP.AcceptanceRadius = 600.f;
			Leader->QuestWaypoints.Empty();
			Leader->QuestWaypoints.Add(WP);
			Leader->BeginQuest();
		}
	}

	ResolveMode(World);

	// Start waves if defending
	if (ResolvedMode == EObjectiveMode::Defend && CachedSpawner.IsValid())
	{
		CachedSpawner->StartWaves();
	}

	// Count initial hostiles for Clear mode
	if (ResolvedMode == EObjectiveMode::Clear && CachedZone.IsValid())
	{
		InitialHostileCount = CachedZone->CountHostilesInZone();
	}

	UE_LOG(LogSquadAI, Log, TEXT("Objective '%s' started: mode=%s, zone=%s"),
		*DisplayText.ToString(),
		*UEnum::GetValueAsString(ResolvedMode),
		*ZoneTag.Tag.ToString());
}

void UQuestObjective_AutoZone::Tick(float DeltaTime, UWorld* World)
{
	if (State != EObjectiveState::Active) return;

	switch (ResolvedMode)
	{
	case EObjectiveMode::Reach:
	{
		if (CachedZone.IsValid() && CachedZone->IsPlayerInZone())
		{
			State = EObjectiveState::Succeeded;
		}
		break;
	}

	case EObjectiveMode::Defend:
	{
		// Defend = all waves spawned AND all enemies dead
		if (CachedSpawner.IsValid() && CachedSpawner->AreAllWavesComplete())
		{
			State = EObjectiveState::Succeeded;
		}

		// Optional: fail if player is outside zone too long
		if (MaxSecondsOutsideZone > 0.f && CachedZone.IsValid())
		{
			if (!CachedZone->IsPlayerInZone())
			{
				OutsideTimer += DeltaTime;
				if (OutsideTimer >= MaxSecondsOutsideZone)
				{
					State = EObjectiveState::Failed;
				}
			}
			else
			{
				OutsideTimer = 0.f;
			}
		}
		break;
	}

	case EObjectiveMode::Clear:
	{
		if (CachedZone.IsValid() && CachedZone->CountHostilesInZone() <= 0)
		{
			// Also check if spawner has finished (if any)
			if (!CachedSpawner.IsValid() || CachedSpawner->AreAllWavesComplete())
			{
				State = EObjectiveState::Succeeded;
			}
		}
		break;
	}

	default:
		break;
	}
}

void UQuestObjective_AutoZone::OnFinish()
{
	if (CachedSpawner.IsValid())
	{
		CachedSpawner->StopWaves();
	}
	Super::OnFinish();
}

void UQuestObjective_AutoZone::ResolveMode(UWorld* World)
{
	if (Mode != EObjectiveMode::Auto)
	{
		ResolvedMode = Mode;
		return;
	}

	// Auto-detect from configuration
	if (CachedSpawner.IsValid() && CachedSpawner->WaveTemplate)
	{
		ResolvedMode = EObjectiveMode::Defend;
	}
	else if (CachedZone.IsValid() && CachedZone->CountHostilesInZone() > 0)
	{
		ResolvedMode = EObjectiveMode::Clear;
	}
	else
	{
		ResolvedMode = EObjectiveMode::Reach;
	}
}
