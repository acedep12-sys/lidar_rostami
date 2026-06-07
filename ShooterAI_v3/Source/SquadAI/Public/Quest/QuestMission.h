// QuestMission.h — One mission = ordered objectives + title + music + optional timer
// Place in level, set Order integer, add objectives. QuestDirector finds it automatically.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Quest/QuestTypes.h"
#include "QuestMission.generated.h"

class UQuestObjective;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionStateChanged, AQuestMission*, Mission);

UCLASS(BlueprintType, Blueprintable)
class SQUADAI_API AQuestMission : public AActor
{
	GENERATED_BODY()

public:
	AQuestMission();

	// ---- Designer config ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	FText MissionTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	int32 Order = 0; // Execution order within the director

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	FQuestTag AutoStartZoneTag; // If set, mission auto-starts when player enters this zone

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	float TimeLimitSeconds = 0.f; // 0 = no limit

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	bool bRightToLeft = false; // For Farsi/Arabic text

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Audio")
	TObjectPtr<USoundBase> MusicCue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Briefing")
	FText BriefingSubtitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Briefing")
	FText BriefingText;

	// Ordered objectives (instanced — edit inline)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Mission")
	TArray<TObjectPtr<UQuestObjective>> Objectives;

	// ---- Runtime ----

	UPROPERTY(BlueprintReadOnly, Category = "Mission|Runtime")
	EMissionState MissionState = EMissionState::Inactive;

	UPROPERTY(BlueprintReadOnly, Category = "Mission|Runtime")
	int32 CurrentObjectiveIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Mission|Runtime")
	float ElapsedTime = 0.f;

	// ---- Events ----

	UPROPERTY(BlueprintAssignable, Category = "Mission|Events")
	FOnMissionStateChanged OnMissionStateChanged;

	// ---- API (called by QuestDirector or QuestTickerSubsystem) ----

	void StartMission();
	void TickMission(float DeltaTime);
	void FailMission();
	void SucceedMission();

	// Get HUD snapshot data
	FQuestHudSnapshot GetHudSnapshot() const;

	// Is the current objective active?
	UQuestObjective* GetCurrentObjective() const;

private:
	void AdvanceObjective();
	void StartCurrentObjective();

	UPROPERTY() TObjectPtr<UAudioComponent> MusicAudio;
	void PlayMusic();
	void StopMusic();
};
