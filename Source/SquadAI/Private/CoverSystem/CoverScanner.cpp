// CoverScanner.cpp — 4-phase async cover scanner
#include "CoverSystem/CoverScanner.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "SquadAITuning.h"

void FCoverScanner::Configure(float InCrouchHeight, float InStandHeight, float InMinThickness,
                               float InNavExtent, float InChunkSizeXY, int32 InSamplesPerAxis,
                               int32 InTraceBudget)
{
	CrouchHeight = InCrouchHeight;
	StandHeight = InStandHeight;
	MinThickness = InMinThickness;
	NavExtent = InNavExtent;
	ChunkSizeXY = InChunkSizeXY;
	SamplesPerAxis = InSamplesPerAxis;
	TraceBudgetPerFrame = InTraceBudget;
	InitDirections();
}

void FCoverScanner::InitDirections()
{
	if (bDirectionsInitialized) return;
	for (int32 i = 0; i < NUM_DIRECTIONS; ++i)
	{
		const float Angle = (360.f / NUM_DIRECTIONS) * i;
		const float Rad = FMath::DegreesToRadians(Angle);
		ProbeDirections[i] = FVector(FMath::Cos(Rad), FMath::Sin(Rad), 0.f);
	}

	// Update probe heights from tuning
	ProbeHeights[0] = 15.f;
	ProbeHeights[1] = 35.f;
	ProbeHeights[2] = CrouchHeight;
	ProbeHeights[3] = 100.f;
	ProbeHeights[4] = StandHeight;

	bDirectionsInitialized = true;
}

// =====================================================================
//  PHASE 1 — Sample the floor via NavMesh projection
// =====================================================================
TArray<FVector> FCoverScanner::GenerateFloorSamples(FIntPoint ChunkKey, UWorld* World)
{
	TArray<FVector> Samples;
	if (!World) return Samples;

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
	if (!NavSys) return Samples;

	const FVector ChunkOrigin(ChunkKey.X * ChunkSizeXY, ChunkKey.Y * ChunkSizeXY, 0.f);
	const float Spacing = ChunkSizeXY / FMath::Max(1, SamplesPerAxis);

	for (int32 ix = 0; ix < SamplesPerAxis; ++ix)
	{
		for (int32 iy = 0; iy < SamplesPerAxis; ++iy)
		{
			FVector ProbeOrigin = ChunkOrigin + FVector(
				(ix + 0.5f) * Spacing,
				(iy + 0.5f) * Spacing,
				5000.f  // Start high
			);

			// Trace down to find ground
			FHitResult GroundHit;
			FCollisionQueryParams Params;
			Params.bTraceComplex = false;

			if (World->LineTraceSingleByChannel(GroundHit, ProbeOrigin,
				ProbeOrigin - FVector(0, 0, 10000.f), ECC_WorldStatic, Params))
			{
				// Project to NavMesh — only accept walkable ground
				FNavLocation NavLoc;
				if (NavSys->ProjectPointToNavigation(GroundHit.ImpactPoint, NavLoc,
					FVector(50.f, 50.f, NavExtent)))
				{
					Samples.Add(NavLoc.Location);
				}
			}
		}
	}

	return Samples;
}

// =====================================================================
//  PHASE 2 — Submit async traces for one floor sample
// =====================================================================
int32 FCoverScanner::SubmitAsyncProbes(FVector FloorPoint, FIntPoint ChunkKey, UWorld* World)
{
	if (!World) return 0;

	// Create in-flight sample storage
	const int32 SampleIndex = InFlightSamples.Num(); // Simple incrementing
	const int64 Key = MakeSampleKey(ChunkKey, SampleIndex);

	FProbeSample& Sample = InFlightSamples.Add(Key);
	Sample.FloorLocation = FloorPoint;
	Sample.PendingTraces = NUM_DIRECTIONS * NUM_HEIGHTS;
	FMemory::Memzero(Sample.HitResults, sizeof(Sample.HitResults));

	FCollisionQueryParams TraceParams;
	TraceParams.bTraceComplex = false;

	int32 TracesSubmitted = 0;
	const float TraceLength = 200.f; // How far to look for cover geometry

	for (int32 d = 0; d < NUM_DIRECTIONS; ++d)
	{
		for (int32 h = 0; h < NUM_HEIGHTS; ++h)
		{
			const FVector Start = FloorPoint + FVector(0, 0, ProbeHeights[h]);
			const FVector End = Start + ProbeDirections[d] * TraceLength;

			// Use async trace delegate — store context in PendingTraceMap
			FTraceDelegate Delegate;
			Delegate.BindRaw(this, &FCoverScanner::OnAsyncTraceComplete);

			FTraceHandle Handle = World->AsyncLineTraceByChannel(
				EAsyncTraceType::Single,
				Start, End,
				ECC_WorldStatic,
				TraceParams,
				FCollisionResponseParams::DefaultResponseParam,
				&Delegate
			);

			// Store context for this trace so we can look it up in the callback
			FPendingTraceContext Ctx;
			Ctx.ChunkKey = ChunkKey;
			Ctx.SampleIndex = SampleIndex;
			Ctx.DirIndex = d;
			Ctx.HeightIndex = h;
			PendingTraceMap.Add(Handle, Ctx);

			TracesSubmitted++;
		}
	}

	return TracesSubmitted;
}

// =====================================================================
//  PHASE 3 — Harvest async trace results
// =====================================================================
void FCoverScanner::OnAsyncTraceComplete(const FTraceHandle& Handle, FTraceDatum& Data)
{
	// Context lookup is O(1)
	FPendingTraceContext Ctx;
	if (!PendingTraceMap.RemoveAndCopyValue(Handle, Ctx)) return;

	const int64 Key = MakeSampleKey(Ctx.ChunkKey, Ctx.SampleIndex);
	FProbeSample* Sample = InFlightSamples.Find(Key);
	if (!Sample) return;

	if (Data.OutHits.Num() > 0 && Data.OutHits[0].bBlockingHit)
	{
		Sample->HitResults[Ctx.DirIndex][Ctx.HeightIndex] = true;
		Sample->HitDistances[Ctx.DirIndex][Ctx.HeightIndex] = Data.OutHits[0].Distance;
		Sample->HitActors[Ctx.DirIndex][Ctx.HeightIndex] = Data.OutHits[0].GetActor();
	}

	Sample->PendingTraces--;
	if (Sample->PendingTraces <= 0)
	{
		Sample->bComplete = true;
	}
}

bool FCoverScanner::IsSampleComplete(FIntPoint ChunkKey, int32 SampleIndex) const
{
	const int64 Key = MakeSampleKey(ChunkKey, SampleIndex);
	const FProbeSample* Sample = InFlightSamples.Find(Key);
	return Sample ? Sample->bComplete : true;
}

// =====================================================================
//  PHASE 4 — Score completed samples and add to chunk
// =====================================================================
void FCoverScanner::FinalizeAndScore(FIntPoint ChunkKey, int32 SampleIndex,
                                      TMap<FIntPoint, FCoverChunk>& Chunks)
{
	const int64 Key = MakeSampleKey(ChunkKey, SampleIndex);
	FProbeSample* Sample = InFlightSamples.Find(Key);
	if (!Sample || !Sample->bComplete) return;

	// Find the best direction (most blocked)
	const int32 BestDir = FindBestDirection(*Sample);
	if (BestDir < 0)
	{
		InFlightSamples.Remove(Key);
		return; // No cover in any direction
	}

	// Classify cover height for the best direction
	ECoverHeight CoverHt = ClassifyHeight(*Sample, BestDir);
	if (CoverHt == ECoverHeight::None)
	{
		InFlightSamples.Remove(Key);
		return;
	}

	// Compute lean sides
	ELeanSide Leans = ComputeLeanSides(*Sample, BestDir);

	// Compute arc score (angular coverage)
	float ArcScore = ComputeArcScore(*Sample);

	// Calculate AI stand position (behind the cover)
	const float StandoffDist = 70.f;
	FVector AIPos = Sample->FloorLocation + (-ProbeDirections[BestDir]) * StandoffDist;

	// Build cover point
	FCoverPoint CP;
	CP.Location = AIPos;
	CP.Normal = ProbeDirections[BestDir]; // Points toward threat side
	CP.Height = CoverHt;
	CP.AvailableLeans = Leans;
	CP.ArcScore = ArcScore;
	CP.ChunkHash = CoverHash::ChunkToHash(ChunkKey);

	// Add to chunk (with dedup check)
	FCoverChunk& Chunk = Chunks.FindOrAdd(ChunkKey);

	// Deduplicate: skip if too close to existing point with similar normal
	bool bTooClose = false;
	for (const FCoverPoint& Existing : Chunk.Points)
	{
		if (FVector::Dist(Existing.Location, CP.Location) < 25.f &&
			FVector::DotProduct(Existing.Normal, CP.Normal) > 0.8f)
		{
			bTooClose = true;
			break;
		}
	}

	if (!bTooClose)
	{
		CP.LocalID = Chunk.NextLocalID++;
		Chunk.Points.Add(CP);
	}

	InFlightSamples.Remove(Key);
}

// =====================================================================
//  SCORING — Rate a cover point against a specific threat
// =====================================================================
float FCoverScanner::ScoreAgainstThreat(const FCoverPoint& Point, FVector ThreatLoc) const
{
	float Score = 0.f;

	// 1. Facing (0.4 weight) — cover normal must point toward threat
	const FVector ToThreat = (ThreatLoc - Point.Location).GetSafeNormal();
	const float FacingDot = FVector::DotProduct(ToThreat, Point.Normal);
	Score += FMath::Clamp((FacingDot + 1.f) * 0.5f, 0.f, 1.f) * 0.4f;

	// 2. Arc score (0.3 weight) — angular coverage from the 7-ray check
	Score += Point.ArcScore * 0.3f;

	// 3. Cover height (0.2 weight) — tall is better
	switch (Point.Height)
	{
	case ECoverHeight::High:    Score += 0.2f; break;
	case ECoverHeight::Low:     Score += 0.14f; break;
	case ECoverHeight::Partial: Score += 0.06f; break;
	default: break;
	}

	// 4. Lean availability (0.1 weight) — more options = better
	int32 LeanCount = 0;
	if (EnumHasAnyFlags(Point.AvailableLeans, ELeanSide::Left))  LeanCount++;
	if (EnumHasAnyFlags(Point.AvailableLeans, ELeanSide::Right)) LeanCount++;
	if (EnumHasAnyFlags(Point.AvailableLeans, ELeanSide::Over))  LeanCount++;
	Score += (LeanCount / 3.f) * 0.1f;

	return FMath::Clamp(Score, 0.f, 1.f);
}

// =====================================================================
//  INTERNAL HELPERS
// =====================================================================
int64 FCoverScanner::MakeSampleKey(FIntPoint ChunkKey, int32 SampleIndex) const
{
	return (int64(ChunkKey.X) * 100000 + int64(ChunkKey.Y)) * 1000 + SampleIndex;
}

int32 FCoverScanner::FindBestDirection(const FProbeSample& Sample) const
{
	int32 BestDir = -1;
	int32 BestHits = 0;

	for (int32 d = 0; d < NUM_DIRECTIONS; ++d)
	{
		int32 HitCount = 0;
		for (int32 h = 0; h < NUM_HEIGHTS; ++h)
		{
			if (Sample.HitResults[d][h]) HitCount++;
		}

		// Need at least 2 heights blocked to count as cover
		if (HitCount >= 2 && HitCount > BestHits)
		{
			BestHits = HitCount;
			BestDir = d;
		}
	}

	return BestDir;
}

ECoverHeight FCoverScanner::ClassifyHeight(const FProbeSample& Sample, int32 DirIndex) const
{
	const bool bLowBlocked = Sample.HitResults[DirIndex][2]; // CrouchHeight index
	const bool bHighBlocked = Sample.HitResults[DirIndex][4]; // StandHeight index

	if (bLowBlocked && bHighBlocked) return ECoverHeight::High;
	if (bLowBlocked) return ECoverHeight::Low;
	if (bHighBlocked) return ECoverHeight::Partial;
	return ECoverHeight::None;
}

ELeanSide FCoverScanner::ComputeLeanSides(const FProbeSample& Sample, int32 BestDir) const
{
	ELeanSide Result = ELeanSide::None;

	// Left = BestDir - 2 (90° CCW), Right = BestDir + 2 (90° CW)
	const int32 LeftDir = (BestDir + NUM_DIRECTIONS - 2) % NUM_DIRECTIONS;
	const int32 RightDir = (BestDir + 2) % NUM_DIRECTIONS;

	// Left is clear if the perpendicular direction is NOT blocked at chest height
	if (!Sample.HitResults[LeftDir][2])
		Result |= ELeanSide::Left;

	if (!Sample.HitResults[RightDir][2])
		Result |= ELeanSide::Right;

	// Over = can peek over the top (cover is low, head height is clear)
	if (Sample.HitResults[BestDir][2] && !Sample.HitResults[BestDir][4])
		Result |= ELeanSide::Over;

	return Result;
}

float FCoverScanner::ComputeArcScore(const FProbeSample& Sample) const
{
	// Count how many of the 8 directions have at least one height blocked
	// within a tight distance (80cm) — this measures "how enclosed" the position is
	int32 BlockedDirs = 0;

	for (int32 d = 0; d < NUM_DIRECTIONS; ++d)
	{
		bool bBlocked = false;
		for (int32 h = 0; h < NUM_HEIGHTS; ++h)
		{
			if (Sample.HitResults[d][h] && Sample.HitDistances[d][h] < 80.f)
			{
				bBlocked = true;
				break;
			}
		}
		if (bBlocked) BlockedDirs++;
	}

	// 7 out of 8 blocked = corner (perfect), 1 = open wall
	// We use 7 as the reference (one direction must be open for the AI to shoot from)
	return FMath::Clamp((float)BlockedDirs / 7.f, 0.f, 1.f);
}
