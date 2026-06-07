// PerceptionMemory.cpp — Multi-sense fusion + movement prediction
#include "AI/PerceptionMemory.h"
#include "Characters/SoldierCharacter.h"

// =====================================================================
//  UPDATE FROM EACH SENSE
// =====================================================================
void UPerceptionMemory::UpdateFromSight(AActor* Actor, bool bSensed, float CurrentTime)
{
	if (!Actor) return;

	FEnhancedPerceivedTarget& T = FindOrAddTarget(Actor);

	if (bSensed)
	{
		// Track velocity for prediction
		if (T.LastSeenTime > 0.f && T.bCurrentlyVisible)
		{
			const float DT = CurrentTime - T.LastSeenTime;
			if (DT > 0.01f)
			{
				T.LastKnownVelocity = (Actor->GetActorLocation() - T.LastKnownLocation) / DT;
			}
		}

		T.LastKnownLocation = Actor->GetActorLocation();
		T.LastSeenTime = CurrentTime;
		T.SightConfidence = 1.f;
		T.bCurrentlyVisible = true;
		T.PredictedLocation = T.LastKnownLocation;
	}
	else
	{
		T.bCurrentlyVisible = false;
		// Don't zero sight confidence — it decays over time
		// Compute prediction at the moment of losing sight
		T.PredictedLocation = T.GetBestKnownPosition(CurrentTime);
	}
}

void UPerceptionMemory::UpdateFromHearing(AActor* Actor, FVector SoundLocation, float CurrentTime)
{
	if (!Actor) return;

	FEnhancedPerceivedTarget& T = FindOrAddTarget(Actor);
	T.HearingConfidence = 1.f;

	// Hearing gives an approximate location (the sound source)
	// Only update position if we don't have a better sight-based one
	if (!T.bCurrentlyVisible)
	{
		T.LastKnownLocation = SoundLocation;
		T.PredictedLocation = SoundLocation;
	}
}

void UPerceptionMemory::UpdateFromDamage(AActor* Actor, FVector DamageOrigin, float CurrentTime)
{
	if (!Actor) return;

	FEnhancedPerceivedTarget& T = FindOrAddTarget(Actor);
	T.DamageConfidence = 1.f;
	T.LastSeenTime = CurrentTime; // Refresh — damage is recent contact

	// Damage gives the source direction
	if (!T.bCurrentlyVisible)
	{
		T.LastKnownLocation = DamageOrigin;
		T.PredictedLocation = DamageOrigin;
	}
}

// =====================================================================
//  DECAY — confidence fades differently per sense and per stance
// =====================================================================
void UPerceptionMemory::DecayAll(float DeltaTime, float CurrentTime, bool bOwnerInCover)
{
	// In cover, suppression decays slower (0.5x rate)
	// This models the soldier being "pinned" — they don't forget the threat as fast
	const float CoverMultiplier = bOwnerInCover ? 0.5f : 1.f;

	for (int32 i = Targets.Num() - 1; i >= 0; --i)
	{
		FEnhancedPerceivedTarget& T = Targets[i];

		// Remove invalid actors
		if (!T.Actor.IsValid())
		{
			Targets.RemoveAtSwap(i, EAllowShrinking::No);
			continue;
		}

		// Remove dead actors
		if (ASoldierCharacter* S = Cast<ASoldierCharacter>(T.Actor.Get()))
		{
			if (!S->IsAlive())
			{
				Targets.RemoveAtSwap(i, EAllowShrinking::No);
				continue;
			}
		}

		// Decay each sense independently
		if (!T.bCurrentlyVisible)
		{
			T.SightConfidence = FMath::Max(0.f, T.SightConfidence - SightDecayRate * DeltaTime * CoverMultiplier);
		}
		else
		{
			T.SightConfidence = 1.f;
			T.LastKnownLocation = T.Actor->GetActorLocation();
		}

		T.HearingConfidence = FMath::Max(0.f, T.HearingConfidence - HearingDecayRate * DeltaTime);
		T.DamageConfidence = FMath::Max(0.f, T.DamageConfidence - DamageDecayRate * DeltaTime);

		// Update prediction
		T.PredictedLocation = T.GetBestKnownPosition(CurrentTime);

		// Remove if ALL confidences are zero
		if (T.GetCombinedConfidence() <= 0.01f)
		{
			Targets.RemoveAtSwap(i, EAllowShrinking::No);
		}
	}
}

// =====================================================================
//  THREAT SCORING — combined confidence weighted by distance
// =====================================================================
AActor* UPerceptionMemory::GetPrimaryThreat(float& OutConfidence, FVector SelfLocation, float CurrentTime) const
{
	AActor* Best = nullptr;
	float BestScore = -FLT_MAX;
	OutConfidence = 0.f;

	for (const FEnhancedPerceivedTarget& T : Targets)
	{
		AActor* A = T.Actor.Get();
		if (!A) continue;

		const float Confidence = T.GetCombinedConfidence();
		const float Dist2 = FVector::DistSquared2D(T.GetBestKnownPosition(CurrentTime), SelfLocation);

		// Score: high confidence + close distance = high priority
		const float Score = Confidence * 10000.f - Dist2 * 0.001f;

		if (Score > BestScore)
		{
			BestScore = Score;
			Best = A;
			OutConfidence = Confidence;
		}
	}

	return Best;
}

FVector UPerceptionMemory::GetPredictedLocation(AActor* Target, float CurrentTime) const
{
	for (const FEnhancedPerceivedTarget& T : Targets)
	{
		if (T.Actor.Get() == Target)
		{
			return T.GetBestKnownPosition(CurrentTime);
		}
	}
	return Target ? Target->GetActorLocation() : FVector::ZeroVector;
}

void UPerceptionMemory::CleanupStale()
{
	for (int32 i = Targets.Num() - 1; i >= 0; --i)
	{
		if (!Targets[i].Actor.IsValid() || Targets[i].GetCombinedConfidence() <= 0.f)
		{
			Targets.RemoveAtSwap(i, EAllowShrinking::No);
		}
	}
}

// =====================================================================
//  HELPERS
// =====================================================================
FEnhancedPerceivedTarget* UPerceptionMemory::FindTarget(AActor* Actor)
{
	return Targets.FindByPredicate([Actor](const FEnhancedPerceivedTarget& T) {
		return T.Actor.Get() == Actor;
	});
}

FEnhancedPerceivedTarget& UPerceptionMemory::FindOrAddTarget(AActor* Actor)
{
	FEnhancedPerceivedTarget* Existing = FindTarget(Actor);
	if (Existing) return *Existing;

	FEnhancedPerceivedTarget NewTarget;
	NewTarget.Actor = Actor;
	NewTarget.LastKnownLocation = Actor->GetActorLocation();
	Targets.Add(NewTarget);
	return Targets.Last();
}
