// SquadTypes.h — Shared enums, structs, and constants used across the entire module
// No .cpp needed — header only
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SquadTypes.generated.h"

// =====================================================================
//  ENUMS
// =====================================================================

UENUM(BlueprintType)
enum class ESquadFaction : uint8
{
	Ally     UMETA(DisplayName = "Ally"),
	Enemy    UMETA(DisplayName = "Enemy"),
	Neutral  UMETA(DisplayName = "Neutral")
};

UENUM(BlueprintType)
enum class ECoverHeight : uint8
{
	None    UMETA(DisplayName = "None"),
	Low     UMETA(DisplayName = "Low (Crouch)"),
	High    UMETA(DisplayName = "High (Stand)"),
	Partial UMETA(DisplayName = "Partial")
};

UENUM(BlueprintType)
enum class ELeanSide : uint8
{
	None  = 0,
	Left  = 1 << 0,
	Right = 1 << 1,
	Over  = 1 << 2
};
ENUM_CLASS_FLAGS(ELeanSide);

UENUM(BlueprintType)
enum class EGameDifficulty : uint8
{
	Easy    UMETA(DisplayName = "Easy"),
	Normal  UMETA(DisplayName = "Normal"),
	Hard    UMETA(DisplayName = "Hard"),
	Lethal  UMETA(DisplayName = "Lethal")
};

UENUM(BlueprintType)
enum class EAttackPhase : uint8
{
	Idle,
	Gather,
	Suppress,
	Flank,
	Assault,
	Hold
};

// =====================================================================
//  COVER POINT (POD — 56 bytes, no UObject overhead)
// =====================================================================

USTRUCT(BlueprintType)
struct FCoverPoint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FVector Location = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) FVector Normal = FVector::ForwardVector;   // Points from cover toward threat side
	UPROPERTY(BlueprintReadOnly) ECoverHeight Height = ECoverHeight::None;
	UPROPERTY(BlueprintReadOnly) ELeanSide AvailableLeans = ELeanSide::None;
	UPROPERTY(BlueprintReadOnly) float ArcScore = 0.f;                     // 0-1, 7-ray angular coverage
	UPROPERTY(BlueprintReadOnly) int32 ChunkHash = 0;                      // Which chunk owns this
	UPROPERTY(BlueprintReadOnly) int32 LocalID = 0;                        // Unique within chunk

	// Reservation
	TWeakObjectPtr<AActor> ReservedBy = nullptr;

	bool IsReserved() const { return ReservedBy.IsValid(); }
	bool IsValid() const { return Height != ECoverHeight::None && ArcScore > 0.f; }

	FIntPoint GetChunkKey() const { return FIntPoint(ChunkHash >> 16, ChunkHash & 0xFFFF); }
};

// =====================================================================
//  PERCEIVED TARGET (fading memory)
// =====================================================================

USTRUCT(BlueprintType)
struct FPerceivedTarget
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) TWeakObjectPtr<AActor> Actor = nullptr;
	UPROPERTY(BlueprintReadOnly) FVector LastKnownLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) float LastSeenTime = 0.f;
	UPROPERTY(BlueprintReadOnly) float Confidence = 0.f;            // 0-1, decays over time
	UPROPERTY(BlueprintReadOnly) bool bCurrentlyVisible = false;
};

// =====================================================================
//  QUEST WAYPOINT
// =====================================================================

USTRUCT(BlueprintType)
struct FQuestWaypoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Location = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AcceptanceRadius = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bRequireAreaClear = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float LingerSeconds = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AreaClearRadius = 1500.f;
};

// =====================================================================
//  QUEST HUD SNAPSHOT (sent to HUD widget on every update)
// =====================================================================

USTRUCT(BlueprintType)
struct FQuestHudSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FText MissionTitle;
	UPROPERTY(BlueprintReadOnly) FText ObjectiveText;
	UPROPERTY(BlueprintReadOnly) float TimerSeconds = -1.f;          // -1 = hide
	UPROPERTY(BlueprintReadOnly) int32 EnemiesRemaining = -1;        // -1 = hide
	UPROPERTY(BlueprintReadOnly) int32 EnemiesKilled = 0;
	UPROPERTY(BlueprintReadOnly) int32 WavesCurrent = -1;            // -1 = hide
	UPROPERTY(BlueprintReadOnly) int32 WavesTotal = -1;
	UPROPERTY(BlueprintReadOnly) bool bMissionActive = false;
	UPROPERTY(BlueprintReadOnly) bool bRightToLeft = false;
};

// =====================================================================
//  WAVE DEFINITION (used in WaveTemplate DataAsset)
// =====================================================================

USTRUCT(BlueprintType)
struct FWaveDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 NumSoldiers = 25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 BatchSize = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1"))
	float SecondsBetweenBatches = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float DelayAfterPreviousSeconds = 5.f;

	// Override enemy class for this specific wave (null = use spawner default)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<APawn> EnemyClassOverride;

	// Announcement text (shown on HUD)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText AnnouncementText;
};

// =====================================================================
//  STATETREE RUNTIME DATA (5.6+ pattern)
// =====================================================================

USTRUCT(BlueprintType)
struct FST_RuntimeData
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr<class ASoldierAIController> Controller;
	UPROPERTY() TWeakObjectPtr<class ASoldierCharacter>     Self;
	UPROPERTY() TWeakObjectPtr<AActor>                      PrimaryThreat;
	UPROPERTY() float  ThreatConfidence    = 0.f;
	UPROPERTY() float  Suppression         = 0.f;
	UPROPERTY() float  HealthPercent       = 1.f;
	UPROPERTY() bool   bHasVisibleEnemy    = false;
	UPROPERTY() FVector LastKnownEnemyLoc  = FVector::ZeroVector;
};

// =====================================================================
//  LOG CATEGORY
// =====================================================================

DECLARE_LOG_CATEGORY_EXTERN(LogSquadAI, Log, All);
