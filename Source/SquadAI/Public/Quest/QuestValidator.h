// QuestValidator.h — Runs once on BeginPlay, catches setup errors with clear messages
// Errors appear as red text on screen in non-Shipping builds
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "QuestValidator.generated.h"

UCLASS()
class SQUADAI_API UQuestValidatorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

private:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	void ValidateAll(UWorld* World);

	void LogError(const FString& Message);
	void LogWarning(const FString& Message);
	int32 ErrorCount = 0;
};
