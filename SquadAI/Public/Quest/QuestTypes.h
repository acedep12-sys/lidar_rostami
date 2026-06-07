// QuestTypes.h — Quest-specific types, enums, and the HUD snapshot struct
// Wave definitions are in SquadTypes.h (FWaveDefinition)
#pragma once

#include "CoreMinimal.h"
#include "SquadTypes.h"
#include "QuestTypes.generated.h"

// Tag for cross-referencing quest actors without dragging actor refs
USTRUCT(BlueprintType)
struct FQuestTag
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Tag = NAME_None;

	bool IsValid() const { return Tag != NAME_None; }
	bool operator==(const FQuestTag& Other) const { return Tag == Other.Tag; }
	friend uint32 GetTypeHash(const FQuestTag& T) { return GetTypeHash(T.Tag); }
};

UENUM(BlueprintType)
enum class EObjectiveMode : uint8
{
	Auto     UMETA(DisplayName = "Auto-Detect"),
	Reach    UMETA(DisplayName = "Reach Zone"),
	Defend   UMETA(DisplayName = "Defend (Waves)"),
	Clear    UMETA(DisplayName = "Clear All Enemies")
};

UENUM(BlueprintType)
enum class EMissionState : uint8
{
	Inactive,
	Active,
	Succeeded,
	Failed
};

UENUM(BlueprintType)
enum class EObjectiveState : uint8
{
	Inactive,
	Active,
	Succeeded,
	Failed
};

// Log entry for mission history
USTRUCT(BlueprintType)
struct FMissionLogEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FText Title;
	UPROPERTY(BlueprintReadOnly) int32 Order = 0;
	UPROPERTY(BlueprintReadOnly) EMissionState FinalState = EMissionState::Inactive;
	UPROPERTY(BlueprintReadOnly) float DurationSeconds = 0.f;
	UPROPERTY(BlueprintReadOnly) FDateTime CompletionTime;
};
