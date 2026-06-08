// QuestDirector.h — Auto-discovers all QuestMission actors, chains them by Order
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director")
	bool bUseOutlinerHierarchy = false;

	// ---- Runtime ----
	UPROPERTY(BlueprintReadOnly, Category = "Director|Runtime")
	TArray<TObjectPtr<AQuestMission>> Missions;

	UPROPERTY(BlueprintReadOnly, Category = "Director|Runtime")
	int32 CurrentMissionIndex = -1;

	// ---- API ----
	UFUNCTION(BlueprintCallable, Category = "Director")
	void StartFirstMission();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Director")
	AQuestMission* GetCurrentMission() const;

	void OnMissionCompleted(AQuestMission* Mission);

protected:
	virtual void BeginPlay() override;

private:
	void CollectMissions();
	void AdvanceToNextMission();
};
