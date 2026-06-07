// QuestRegistry.h — Per-world TMap: every quest actor self-registers by tag
// O(1) lookup. Designer never drags actor references — just types a tag name.
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

	// Register an actor under a tag (called by quest actors in BeginPlay)
	void Register(FQuestTag Tag, AActor* Actor);

	// Unregister (called in EndPlay)
	void Unregister(FQuestTag Tag);

	// Look up an actor by tag — O(1)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest Registry")
	AActor* Find(FQuestTag Tag) const;

	// Check if a tag exists
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest Registry")
	bool Exists(FQuestTag Tag) const;

	// Get all registered tags (for validation)
	TArray<FQuestTag> GetAllTags() const;

private:
	TMap<FName, TWeakObjectPtr<AActor>> Registry;
};
