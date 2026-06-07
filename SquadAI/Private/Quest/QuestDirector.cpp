// QuestDirector.cpp — Auto-discovers missions, chains by Order integer
#include "Quest/QuestDirector.h"
#include "Quest/QuestMission.h"
#include "EngineUtils.h"

AQuestDirector::AQuestDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AQuestDirector::BeginPlay()
{
	Super::BeginPlay();
	CollectMissions();

	if (bAutoStart && Missions.Num() > 0)
	{
		// Slight delay for level to finish loading
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, this,
			&AQuestDirector::StartFirstMission, 1.f, false);
	}
}

void AQuestDirector::CollectMissions()
{
	Missions.Empty();

	if (bUseOutlinerHierarchy)
	{
		// Only pick up missions attached to us in the Outliner
		TArray<AActor*> AttachedChildren;
		GetAttachedActors(AttachedChildren, true);
		for (AActor* Child : AttachedChildren)
		{
			if (AQuestMission* M = Cast<AQuestMission>(Child))
			{
				Missions.Add(M);
			}
		}
	}
	else
	{
		// Find ALL missions in the level
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (AQuestMission* Mission = Cast<AQuestMission>(*It))
			{
				Missions.Add(Mission);
			}
		}
	}

	// Sort by Order (stable sort preserves Outliner order for same-Order missions)
	Missions.StableSort([](const AQuestMission& A, const AQuestMission& B) {
		return A.Order < B.Order;
	});

	UE_LOG(LogSquadAI, Log, TEXT("QuestDirector: found %d missions"), Missions.Num());
}

void AQuestDirector::StartFirstMission()
{
	if (Missions.Num() == 0) return;

	CurrentMissionIndex = 0;
	Missions[0]->StartMission();
}

void AQuestDirector::OnMissionCompleted(AQuestMission* Mission)
{
	AdvanceToNextMission();
}

void AQuestDirector::AdvanceToNextMission()
{
	CurrentMissionIndex++;

	if (CurrentMissionIndex >= Missions.Num())
	{
		UE_LOG(LogSquadAI, Log, TEXT("QuestDirector: all missions complete!"));
		return;
	}

	// Brief delay between missions
	FTimerHandle Handle;
	const int32 NextIdx = CurrentMissionIndex;
	GetWorldTimerManager().SetTimer(Handle, [this, NextIdx]()
	{
		if (NextIdx < Missions.Num())
		{
			Missions[NextIdx]->StartMission();
		}
	}, 3.f, false);
}

AQuestMission* AQuestDirector::GetCurrentMission() const
{
	if (CurrentMissionIndex >= 0 && CurrentMissionIndex < Missions.Num())
	{
		return Missions[CurrentMissionIndex];
	}
	return nullptr;
}
