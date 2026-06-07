// PerceptionMemory.h — Enhanced fading memory with multi-sense fusion + movement prediction
// Replaces the basic FPerceivedTarget array with a proper memory system
#pragma once

#include "CoreMinimal.h"
#include "SquadTypes.h"
#include "PerceptionMemory.generated.h"

// Enhanced perceived target with per-sense confidence + velocity tracking
USTRUCT(BlueprintType)
struct FEnhancedPerceivedTarget
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) TWeakObjectPtr<AActor> Actor = nullptr;
	UPROPERTY(BlueprintReadOnly) FVector LastKnownLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) FVector LastKnownVelocity = FVector::ZeroVector;  // For prediction
	UPROPERTY(BlueprintReadOnly) FVector PredictedLocation = FVector::ZeroVector;  // Extrapolated

	// Per-sense confidence (multi-sense fusion)
	UPROPERTY(BlueprintReadOnly) float SightConfidence = 0.f;
	UPROPERTY(BlueprintReadOnly) float HearingConfidence = 0.f;
	UPROPERTY(BlueprintReadOnly) float DamageConfidence = 0.f;

	UPROPERTY(BlueprintReadOnly) float LastSeenTime = 0.f;
	UPROPERTY(BlueprintReadOnly) bool bCurrentlyVisible = false;

	// Combined confidence with weighted fusion
	float GetCombinedConfidence() const
	{
		return FMath::Clamp(
			0.7f * SightConfidence +
			0.2f * HearingConfidence +
			0.1f * DamageConfidence,
			0.f, 1.f);
	}

	// Get the best known/predicted position
	FVector GetBestKnownPosition(float CurrentTime) const
	{
		if (bCurrentlyVisible) return LastKnownLocation;

		// Extrapolate from last known velocity
		const float TimeSinceSeen = CurrentTime - LastSeenTime;
		if (TimeSinceSeen < 3.f && !LastKnownVelocity.IsNearlyZero())
		{
			return LastKnownLocation + LastKnownVelocity * TimeSinceSeen;
		}

		return LastKnownLocation;
	}
};

// Memory system that manages all perceived targets
UCLASS()
class SQUADAI_API UPerceptionMemory : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	TArray<FEnhancedPerceivedTarget> Targets;

	// Decay rates (from tuning)
	float SightDecayRate = 0.3f;
	float HearingDecayRate = 0.5f;
	float DamageDecayRate = 0.4f;

	// Update or add a target from a perception event
	void UpdateFromSight(AActor* Actor, bool bSensed, float CurrentTime);
	void UpdateFromHearing(AActor* Actor, FVector SoundLocation, float CurrentTime);
	void UpdateFromDamage(AActor* Actor, FVector DamageOrigin, float CurrentTime);

	// Decay all confidences over time
	void DecayAll(float DeltaTime, float CurrentTime, bool bOwnerInCover);

	// Get the highest-priority threat
	AActor* GetPrimaryThreat(float& OutConfidence, FVector SelfLocation, float CurrentTime) const;

	// Get predicted location of a specific target
	FVector GetPredictedLocation(AActor* Target, float CurrentTime) const;

	// Remove dead/invalid entries
	void CleanupStale();

private:
	FEnhancedPerceivedTarget* FindTarget(AActor* Actor);
	FEnhancedPerceivedTarget& FindOrAddTarget(AActor* Actor);
};
