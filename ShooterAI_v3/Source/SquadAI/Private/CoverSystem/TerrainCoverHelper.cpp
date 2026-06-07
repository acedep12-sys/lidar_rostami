// TerrainCoverHelper.cpp — Open terrain cover detection
#include "CoverSystem/TerrainCoverHelper.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

namespace TerrainCover
{

// =====================================================================
//  MEASURE EXACT COVER HEIGHT — probe every 10cm to find where cover ends
// =====================================================================
float MeasureExactCoverHeight(FVector FloorPos, FVector Direction, float MaxHeight, UWorld* World)
{
	if (!World) return 0.f;

	FCollisionQueryParams Params;
	Params.bTraceComplex = false;

	const float TraceLen = 200.f;
	float HighestBlocked = 0.f;

	// Probe every 10cm from ground to MaxHeight
	for (float H = 10.f; H <= MaxHeight; H += 10.f)
	{
		const FVector Start = FloorPos + FVector(0, 0, H);
		const FVector End = Start + Direction * TraceLen;

		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
		{
			HighestBlocked = H;
		}
		else
		{
			// First unblocked height — cover ends here
			break;
		}
	}

	return HighestBlocked;
}

// =====================================================================
//  BODY COVERAGE — what percentage of a soldier's body is covered?
// =====================================================================
float BodyCoverageAtHeight(float CoverHeight)
{
	// Standard soldier body segments:
	//   0-50cm   = legs     (30% of body area)
	//   50-100cm = torso    (40% of body area)
	//   100-170cm = chest+head (30% of body area)

	if (CoverHeight <= 0.f) return 0.f;

	float Coverage = 0.f;

	// Legs (0-50cm)
	if (CoverHeight >= 50.f)
		Coverage += 0.30f; // Full leg coverage
	else
		Coverage += 0.30f * (CoverHeight / 50.f);

	// Torso (50-100cm)
	if (CoverHeight >= 100.f)
		Coverage += 0.40f; // Full torso coverage
	else if (CoverHeight > 50.f)
		Coverage += 0.40f * ((CoverHeight - 50.f) / 50.f);

	// Chest + Head (100-170cm)
	if (CoverHeight >= 170.f)
		Coverage += 0.30f; // Full upper body coverage
	else if (CoverHeight > 100.f)
		Coverage += 0.30f * ((CoverHeight - 100.f) / 70.f);

	return FMath::Clamp(Coverage, 0.f, 1.f);
}

// =====================================================================
//  TRENCH DETECTION — check if this point is below surrounding terrain
// =====================================================================
FTrenchResult DetectTrench(FVector FloorPos, UWorld* World)
{
	FTrenchResult Result;
	if (!World) return Result;

	FCollisionQueryParams Params;
	Params.bTraceComplex = false;

	// Sample surrounding terrain height in 4 cardinal directions at 150cm distance
	float SurroundingHeightSum = 0.f;
	int32 SurroundingSamples = 0;

	const FVector CardinalDirs[] = {
		FVector(1, 0, 0), FVector(-1, 0, 0),
		FVector(0, 1, 0), FVector(0, -1, 0)
	};

	for (const FVector& Dir : CardinalDirs)
	{
		FVector ProbeStart = FloorPos + Dir * 150.f + FVector(0, 0, 500.f);
		FVector ProbeEnd = ProbeStart - FVector(0, 0, 1000.f);

		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, ProbeStart, ProbeEnd, ECC_WorldStatic, Params))
		{
			SurroundingHeightSum += Hit.ImpactPoint.Z;
			SurroundingSamples++;
		}
	}

	if (SurroundingSamples < 2) return Result; // Can't determine

	const float AvgSurroundingHeight = SurroundingHeightSum / SurroundingSamples;
	const float DepthBelow = AvgSurroundingHeight - FloorPos.Z;

	// A trench is >= 40cm below surrounding terrain
	if (DepthBelow >= 40.f)
	{
		Result.bIsTrench = true;
		Result.TrenchDepth = DepthBelow;
		Result.TrenchCenter = FloorPos;

		// Effective cover = trench depth (a 100cm deep trench covers like a 100cm wall)
		Result.EffectiveCoverHeight = FMath::Min(DepthBelow, 170.f);
	}

	return Result;
}

// =====================================================================
//  CEILING / ENCLOSED CHECK — reject boxes, exempt trenches
// =====================================================================
bool IsEnclosedSpace(FVector FloorPos, bool bIsTrench, UWorld* World)
{
	if (!World) return false;

	// Trenches are exempt — they naturally have "ceiling" (ground above the lip)
	if (bIsTrench) return false;

	// Cast straight up from standing height
	FCollisionQueryParams Params;
	Params.bTraceComplex = false;

	FHitResult CeilingHit;
	const FVector Start = FloorPos + FVector(0, 0, 170.f); // Standing head height
	const FVector End = FloorPos + FVector(0, 0, 250.f);   // 80cm above head

	if (World->LineTraceSingleByChannel(CeilingHit, Start, End, ECC_WorldStatic, Params))
	{
		// Ceiling exists — this is likely inside a building/tunnel/box
		// Additional check: are ALL 4 cardinal directions also blocked at standing height?
		int32 BlockedDirs = 0;
		const FVector CardinalDirs[] = {
			FVector(1,0,0), FVector(-1,0,0), FVector(0,1,0), FVector(0,-1,0)
		};

		for (const FVector& Dir : CardinalDirs)
		{
			FHitResult WallHit;
			FVector WallStart = FloorPos + FVector(0, 0, 90.f);
			FVector WallEnd = WallStart + Dir * 100.f;
			if (World->LineTraceSingleByChannel(WallHit, WallStart, WallEnd, ECC_WorldStatic, Params))
			{
				BlockedDirs++;
			}
		}

		// If ceiling AND 3+ walls = enclosed (coffin/box/tunnel)
		// If ceiling but <3 walls = probably a building with openings (OK)
		return BlockedDirs >= 3;
	}

	return false; // No ceiling = not enclosed
}

// =====================================================================
//  ESCAPE PATH — can the soldier retreat?
// =====================================================================
bool HasEscapePath(FVector CoverPos, FVector ThreatDirection, float MinClearance, UWorld* World)
{
	if (!World) return true;

	// Check behind the cover (180° from threat)
	const FVector RetreatDir = -ThreatDirection;
	const FVector Start = CoverPos + FVector(0, 0, 90.f);
	const FVector End = Start + RetreatDir * MinClearance;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.bTraceComplex = false;

	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
	{
		// Blocked within MinClearance — also check 30° left and right
		const FVector Left = RetreatDir.RotateAngleAxis(-30.f, FVector::UpVector);
		const FVector Right = RetreatDir.RotateAngleAxis(30.f, FVector::UpVector);

		FHitResult LeftHit, RightHit;
		bool bLeftBlocked = World->LineTraceSingleByChannel(LeftHit, Start, Start + Left * MinClearance, ECC_WorldStatic, Params);
		bool bRightBlocked = World->LineTraceSingleByChannel(RightHit, Start, Start + Right * MinClearance, ECC_WorldStatic, Params);

		// If ALL three retreat paths are blocked, no escape
		return !(bLeftBlocked && bRightBlocked);
	}

	return true; // Direct retreat path is clear
}

// =====================================================================
//  LEAN QUALITY — how clear is each lean direction?
// =====================================================================
FLeanQuality MeasureLeanQuality(FVector CoverPos, FVector CoverNormal, float CoverHeight, UWorld* World)
{
	FLeanQuality Result;
	if (!World) return Result;

	FCollisionQueryParams Params;
	Params.bTraceComplex = false;

	const FVector Right = FVector::CrossProduct(CoverNormal, FVector::UpVector).GetSafeNormal();
	const float CheckDist = 100.f;
	const float PeekHeight = FMath::Min(CoverHeight + 20.f, 170.f);

	// Left lean: trace to the left at peek height
	{
		FVector Start = CoverPos + FVector(0, 0, PeekHeight);
		FVector End = Start - Right * CheckDist + CoverNormal * 50.f; // Left + slightly forward
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
		{
			Result.Left = FMath::Clamp(Hit.Distance / CheckDist, 0.f, 1.f);
		}
		else
		{
			Result.Left = 1.f; // Fully clear
		}
	}

	// Right lean
	{
		FVector Start = CoverPos + FVector(0, 0, PeekHeight);
		FVector End = Start + Right * CheckDist + CoverNormal * 50.f;
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
		{
			Result.Right = FMath::Clamp(Hit.Distance / CheckDist, 0.f, 1.f);
		}
		else
		{
			Result.Right = 1.f;
		}
	}

	// Over (peek over top — only relevant for short cover)
	if (CoverHeight < 140.f)
	{
		FVector Start = CoverPos + FVector(0, 0, CoverHeight + 30.f);
		FVector End = Start + CoverNormal * 80.f;
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
		{
			Result.Over = FMath::Clamp(Hit.Distance / 80.f, 0.f, 1.f);
		}
		else
		{
			Result.Over = 1.f;
		}
	}

	return Result;
}

// =====================================================================
//  OPEN TERRAIN SCORING — values ANY cover, not just full cover
// =====================================================================
float ScoreOpenTerrainCover(
	float CoverHeight,
	float BodyCoverage,
	float ArcScore,
	const FLeanQuality& LeanQuality,
	bool bIsTrench,
	bool bHasEscapePath)
{
	float Score = 0.f;

	// 1. Body coverage is the PRIMARY score for open terrain (0.40 weight)
	//    Even 30% coverage (legs only) gets some score
	Score += BodyCoverage * 0.40f;

	// 2. Arc score (angular protection) (0.20 weight)
	Score += ArcScore * 0.20f;

	// 3. Lean availability (0.15 weight) — can peek to shoot?
	float BestLean = FMath::Max3(LeanQuality.Left, LeanQuality.Right, LeanQuality.Over);
	Score += BestLean * 0.15f;

	// 4. Trench bonus (0.10 weight) — trenches are excellent cover
	if (bIsTrench)
	{
		Score += 0.10f;
	}

	// 5. Escape path (0.10 weight) — can retreat if needed
	if (bHasEscapePath)
	{
		Score += 0.10f;
	}

	// 6. Height bonus (0.05 weight) — taller is slightly better
	Score += FMath::Clamp(CoverHeight / 170.f, 0.f, 1.f) * 0.05f;

	// IMPORTANT: On open terrain, even low-scoring cover (0.15) is usable
	// The AI's "no cover found" fallback should have a threshold of ~0.10
	// rather than the typical 0.30 used in urban environments

	return FMath::Clamp(Score, 0.f, 1.f);
}

} // namespace TerrainCover
