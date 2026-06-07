// TerrainCoverHelper.h — Specialized cover detection for open terrain
// Handles: landscape bumps, partial cover, trenches, continuous height scoring
// This is an extension to the CoverScanner that runs AFTER the standard 8-dir probe
#pragma once

#include "CoreMinimal.h"
#include "SquadTypes.h"

class UWorld;

namespace TerrainCover
{
	// =====================================================================
	//  PARTIAL COVER CLASSIFICATION
	//  Standard scanner requires 2+ heights blocked. On open terrain with
	//  landscape spikes, often only 1 height is blocked. This reclassifies
	//  "rejected" samples as usable partial cover.
	// =====================================================================

	// Returns the exact height (cm) of the blocking geometry in the given direction
	// Probes every 10cm from ground up to MaxHeight
	float MeasureExactCoverHeight(FVector FloorPos, FVector Direction, float MaxHeight, UWorld* World);

	// Determine what body percentage is covered at a given height
	// Returns 0-1 (0 = fully exposed, 1 = fully covered)
	// Based on a standard soldier model:
	//   0-50cm = legs (30% of body)
	//   50-100cm = torso (40% of body)
	//   100-170cm = chest+head (30% of body)
	float BodyCoverageAtHeight(float CoverHeight);

	// =====================================================================
	//  TRENCH DETECTION
	//  A trench provides cover by being LOWER than surrounding terrain.
	//  The standard horizontal probes miss this because the "cover" is
	//  the ground above the trench lip.
	// =====================================================================

	struct FTrenchResult
	{
		bool bIsTrench = false;
		float TrenchDepth = 0.f;         // How deep below surrounding terrain
		float EffectiveCoverHeight = 0.f; // Depth acts as cover height
		FVector TrenchCenter = FVector::ZeroVector;
	};

	// Check if a NavMesh-projected floor point is in a trench
	// Probes upward from the point and compares to surrounding terrain height
	FTrenchResult DetectTrench(FVector FloorPos, UWorld* World);

	// =====================================================================
	//  CEILING / ENCLOSED SPACE CHECK
	//  Reject cover that's inside a fully enclosed space (coffin/box)
	//  BUT exempt trenches — they have "ceiling" (the ground above) which is fine
	// =====================================================================

	// Returns true if the position has a ceiling (enclosed)
	// Exempts positions already identified as trenches
	bool IsEnclosedSpace(FVector FloorPos, bool bIsTrench, UWorld* World);

	// =====================================================================
	//  ESCAPE PATH CHECK
	//  Can the soldier retreat from this cover position?
	//  Fires a trace 180° from the threat direction to check for clearance
	// =====================================================================

	// Returns true if the soldier can retreat (path behind is clear)
	bool HasEscapePath(FVector CoverPos, FVector ThreatDirection, float MinClearance, UWorld* World);

	// =====================================================================
	//  LEAN QUALITY
	//  How clear is each lean direction? 0 = blocked, 1 = fully clear
	// =====================================================================

	struct FLeanQuality
	{
		float Left = 0.f;    // 0-1
		float Right = 0.f;   // 0-1
		float Over = 0.f;    // 0-1 (peek over top — relevant for short cover)
	};

	FLeanQuality MeasureLeanQuality(FVector CoverPos, FVector CoverNormal, float CoverHeight, UWorld* World);

	// =====================================================================
	//  COVER QUALITY FOR OPEN TERRAIN
	//  Modified scoring that values ANY cover on open terrain
	//  A 50cm landscape bump that covers the torso is better than nothing
	// =====================================================================

	// Compute a quality score for open terrain cover
	// Unlike the standard scorer, this doesn't penalize partial cover heavily
	float ScoreOpenTerrainCover(
		float CoverHeight,
		float BodyCoverage,
		float ArcScore,
		const FLeanQuality& LeanQuality,
		bool bIsTrench,
		bool bHasEscapePath
	);
}
