// QuestDirector.h — Auto-discovers all QuestMission actors, chains them by Order
// Drop ONE in your level. bAutoStart = true → begins on BeginPlay. Zero Blueprint code.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Quest/QuestTypes.h"
#include "QuestDirector.generated.h"

class AQuestMission;

UCLASS(BlueprintType, Blueprintable)
class SQUADAI_API AQuestDirector : public AActor
{
	GENERATED_BODY()

public:
	AQuestDirector();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director")
	bool bAutoStart = true;

	// Use Outliner hierarchy for ordering (drag missions onto director)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director")
	bool bUseOutlinerHierarchy = false;

	// ---- Runtime ----

	UPROPERTY(BlueprintReadOnly, Category = "Director|Runtime")
	TArray<AQuestMission*> Missions;

	UPROPERTY(BlueprintReadOnly, Category = "Director|Runtime")
	int32 CurrentMissionIndex = -1;

	// ---- API ----

	UFUNCTION(BlueprintCallable, Category = "Director")
	void StartFirstMission();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Director")
	AQuestMission* GetCurrentMission() const;

	// Called by QuestTickerSubsystem when a mission completes
	void OnMissionCompleted(AQuestMission* Mission);

protected:
	virtual void BeginPlay() override;

private:
	void CollectMissions();
	void AdvanceToNextMission();
};
