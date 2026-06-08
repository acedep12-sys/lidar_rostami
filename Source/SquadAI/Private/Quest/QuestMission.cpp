// QuestMission.cpp — Mission execution with objective chaining + music + timer
#include "Quest/QuestMission.h"
#include "Quest/QuestObjective.h"
#include "Performance/SoldierRegistrySubsystem.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AQuestMission::AQuestMission()
{
	PrimaryActorTick.bCanEverTick = false; // Ticked by QuestTickerSubsystem
}

void AQuestMission::StartMission()
{
	if (MissionState == EMissionState::Active) return;

	MissionState = EMissionState::Active;
	CurrentObjectiveIndex = -1;
	ElapsedTime = 0.f;

	PlayMusic();
	AdvanceObjective();

	OnMissionStateChanged.Broadcast(this);
	UE_LOG(LogSquadAI, Log, TEXT("Mission started: %s"), *MissionTitle.ToString());
}

void AQuestMission::TickMission(float DeltaTime)
{
	if (MissionState != EMissionState::Active) return;

	ElapsedTime += DeltaTime;

	// Timer check
	if (TimeLimitSeconds > 0.f && ElapsedTime >= TimeLimitSeconds)
	{
		FailMission();
		return;
	}

	// Tick current objective
	UQuestObjective* Current = GetCurrentObjective();
	if (Current)
	{
		Current->Tick(DeltaTime, GetWorld());

		if (Current->IsComplete())
		{
			Current->OnFinish();
			AdvanceObjective();
		}
		else if (Current->IsFailed())
		{
			Current->OnFinish();
			FailMission();
		}
	}
}

void AQuestMission::AdvanceObjective()
{
	CurrentObjectiveIndex++;

	if (CurrentObjectiveIndex >= Objectives.Num())
	{
		SucceedMission();
		return;
	}

	StartCurrentObjective();
	OnMissionStateChanged.Broadcast(this);
}

void AQuestMission::StartCurrentObjective()
{
	UQuestObjective* Obj = GetCurrentObjective();
	if (Obj)
	{
		Obj->OnStart(GetWorld());
		UE_LOG(LogSquadAI, Log, TEXT("  Objective %d: %s"), CurrentObjectiveIndex, *Obj->DisplayText.ToString());
	}
}

void AQuestMission::SucceedMission()
{
	MissionState = EMissionState::Succeeded;
	StopMusic();
	OnMissionStateChanged.Broadcast(this);
	UE_LOG(LogSquadAI, Log, TEXT("Mission SUCCEEDED: %s (%.1fs)"), *MissionTitle.ToString(), ElapsedTime);
}

void AQuestMission::FailMission()
{
	MissionState = EMissionState::Failed;

	UQuestObjective* Current = GetCurrentObjective();
	if (Current) Current->OnFinish();

	StopMusic();
	OnMissionStateChanged.Broadcast(this);
	UE_LOG(LogSquadAI, Log, TEXT("Mission FAILED: %s"), *MissionTitle.ToString());
}

UQuestObjective* AQuestMission::GetCurrentObjective() const
{
	if (CurrentObjectiveIndex >= 0 && CurrentObjectiveIndex < Objectives.Num())
	{
		return Objectives[CurrentObjectiveIndex];
	}
	return nullptr;
}

FQuestHudSnapshot AQuestMission::GetHudSnapshot() const
{
	FQuestHudSnapshot Snap;
	Snap.MissionTitle = MissionTitle;
	Snap.bMissionActive = (MissionState == EMissionState::Active);
	Snap.bRightToLeft = bRightToLeft;

	if (TimeLimitSeconds > 0.f)
	{
		Snap.TimerSeconds = FMath::Max(0.f, TimeLimitSeconds - ElapsedTime);
	}

	UQuestObjective* Obj = GetCurrentObjective();
	if (Obj)
	{
		Snap.ObjectiveText = Obj->DisplayText;

		// PULL ENEMY COUNTS: Auto-detect enemies for the HUD
		UWorld* World = GetWorld();
		if (World)
		{
			// Find how many enemies are actually alive in the world
			if (USoldierRegistrySubsystem* Reg = World->GetSubsystem<USoldierRegistrySubsystem>())
			{
				// Team 0 = Ally, Team 1 = Enemy. Count hostiles (Team 1)
				Snap.EnemiesRemaining = Reg->CountHostilesNear(GetActorLocation(), 999999.f, 0);
			}
		}
	}

	return Snap;
}

void AQuestMission::PlayMusic()
{
	if (!MusicCue) return;
	StopMusic();
	MusicAudio = UGameplayStatics::SpawnSound2D(GetWorld(), MusicCue, 1.f, 1.f, 0.f);
	if (MusicAudio) MusicAudio->FadeIn(2.f);
}

void AQuestMission::StopMusic()
{
	if (MusicAudio)
	{
		MusicAudio->FadeOut(1.5f, 0.f);
		MusicAudio = nullptr;
	}
}
