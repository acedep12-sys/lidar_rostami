// QuestLog.h — Mission history with save/load serialization
// Records every mission's outcome. ToBytes/FromBytes for USaveGame integration.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Quest/QuestTypes.h"
#include "QuestLog.generated.h"

UCLASS()
class SQUADAI_API UQuestLogSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Record a mission result
	UFUNCTION(BlueprintCallable, Category = "Quest Log")
	void RecordMission(FText Title, int32 Order, EMissionState State, float Duration);

	// Get all entries
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest Log")
	TArray<FMissionLogEntry> GetEntries() const { return Entries; }

	// Serialization for save games
	UFUNCTION(BlueprintCallable, Category = "Quest Log")
	TArray<uint8> ToBytes() const;

	UFUNCTION(BlueprintCallable, Category = "Quest Log")
	void FromBytes(const TArray<uint8>& Data);

	// Clear log
	UFUNCTION(BlueprintCallable, Category = "Quest Log")
	void ClearLog() { Entries.Empty(); }

private:
	UPROPERTY()
	TArray<FMissionLogEntry> Entries;
};
