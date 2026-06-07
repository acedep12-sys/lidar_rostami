// QuestTickerSubsystem.cpp — 4Hz tick for active missions only
#include "Quest/QuestTickerSubsystem.h"
#include "Quest/QuestDirector.h"
#include "Quest/QuestMission.h"
#include "SquadAITuning.h"
#include "EngineUtils.h"

void UQuestTickerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UQuestTickerSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

bool UQuestTickerSubsystem::IsTickable() const
{
	if (!Director.IsValid()) return true; // Tick to find director

	AQuestMission* Current = Director->GetCurrentMission();
	return Current && Current->MissionState == EMissionState::Active;
}

void UQuestTickerSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!Director.IsValid())
	{
		FindDirector();
		return;
	}

	AQuestMission* Current = Director->GetCurrentMission();
	if (!Current) return;

	if (Current->MissionState == EMissionState::Active)
	{
		// Tick the mission at quest tick rate (accumulated DeltaTime covers this)
		Current->TickMission(DeltaTime);

		// Broadcast HUD update
		BroadcastHUD(Current);

		// Check for completion
		if (Current->MissionState == EMissionState::Succeeded)
		{
			Director->OnMissionCompleted(Current);
		}
	}
}

void UQuestTickerSubsystem::FindDirector()
{
	for (TActorIterator<AQuestDirector> It(GetWorld()); It; ++It)
	{
		Director = *It;
		return;
	}
}

void UQuestTickerSubsystem::SetDirector(AQuestDirector* Dir)
{
	Director = Dir;
}

void UQuestTickerSubsystem::BroadcastHUD(AQuestMission* Mission)
{
	if (!Mission) return;

	FQuestHudSnapshot Snap = Mission->GetHudSnapshot();
	OnHudUpdated.Broadcast(Snap);
}
