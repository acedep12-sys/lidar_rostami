// QuestMission.h — One mission = ordered objectives + title + music + optional timer
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	FText MissionTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	int32 Order = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	FQuestTag AutoStartZoneTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	float TimeLimitSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	bool bRightToLeft = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Audio")
	TObjectPtr<USoundBase> MusicCue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Briefing")
	FText BriefingSubtitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Briefing")
	FText BriefingText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Mission")
	TArray<TObjectPtr<UQuestObjective>> Objectives;

	// ---- Runtime ----
	UPROPERTY(BlueprintReadOnly, Category = "Mission|Runtime")
	EMissionState MissionState = EMissionState::Inactive;

	UPROPERTY(BlueprintReadOnly, Category = "Mission|Runtime")
	int32 CurrentObjectiveIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Mission|Runtime")
	float ElapsedTime = 0.f;

	UPROPERTY(BlueprintAssignable, Category = "Mission|Events")
	FOnMissionStateChanged OnMissionStateChanged;

	void StartMission();
	void TickMission(float DeltaTime);
	void FailMission();
	void SucceedMission();

	FQuestHudSnapshot GetHudSnapshot() const;
	UQuestObjective* GetCurrentObjective() const;

private:
	void AdvanceObjective();
	void StartCurrentObjective();

	UPROPERTY() TObjectPtr<UAudioComponent> MusicAudio;
	void PlayMusic();
	void StopMusic();
};
