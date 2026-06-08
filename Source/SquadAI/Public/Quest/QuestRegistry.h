// QuestRegistry.h — GPS Beacon for the whole squad
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Quest/QuestTypes.h"
#include "QuestRegistry.generated.h"

UCLASS()
class SQUADAI_API UQuestRegistry : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void Register(FQuestTag Tag, AActor* Actor);
	void Unregister(FQuestTag Tag);
	
	UFUNCTION(BlueprintCallable, Category = "Quest")
	AActor* Find(FQuestTag Tag) const;

	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool Exists(FQuestTag Tag) const;

	TArray<FQuestTag> GetAllTags() const;

	// THE GPS BEACON: Every AI looks at this to find the objective
	UPROPERTY(BlueprintReadWrite, Category = "Quest")
	FVector CurrentActiveGoal = FVector::ZeroVector;

private:
	TMap<FName, TWeakObjectPtr<AActor>> Registry;
};
