// CoverScanner.h — The 4-phase async cover scanner
// Owned by CoverSystemSubsystem. Handles the actual tracing work.
// Phase 1: Sample floor via NavMesh projection
// Phase 2: Fire 8-direction × 5-height async traces (budgeted per frame)
// Phase 3: Harvest results, classify cover type + lean sides
// Phase 4: Score via 7-ray arc, deduplicate, store in chunk
#pragma once

#include "CoreMinimal.h"
#include "CoverTypes.h"

class UWorld;

class FCoverScanner
{
public:
	// Initialize with tuning values (called by subsystem)
	void Configure(float InCrouchHeight, float InStandHeight, float InMinThickness,
	               float InNavExtent, float InChunkSizeXY, int32 InSamplesPerAxis,
	               int32 InTraceBudget);

	// Phase 1: Generate floor sample points for a chunk
	TArray<FVector> GenerateFloorSamples(FIntPoint ChunkKey, UWorld* World);

	// Phase 2: Submit async traces for one floor sample (returns traces queued)
	int32 SubmitAsyncProbes(FVector FloorPoint, FIntPoint ChunkKey, UWorld* World);

	// Phase 3: Called by async trace callback — looks up context from PendingTraceMap
	void OnAsyncTraceComplete(const FTraceHandle& Handle, FTraceDatum& Data);

	// Pending trace context (keyed by FTraceHandle — avoids BindRaw with extra params)
	struct FPendingTraceContext
	{
		FIntPoint ChunkKey;
		int32 SampleIndex;
		int32 DirIndex;
		int32 HeightIndex;
	};
	TMap<FTraceHandle, FPendingTraceContext> PendingTraceMap;

	// Phase 4: After all traces for a sample complete, score and add to chunk
	void FinalizeAndScore(FIntPoint ChunkKey, int32 SampleIndex,
	                      TMap<FIntPoint, FCoverChunk>& Chunks);

	bool IsSampleComplete(FIntPoint ChunkKey, int32 SampleIndex) const;

	// Score a cover point against a specific threat location (for query-time ranking)
	float ScoreAgainstThreat(const FCoverPoint& Point, FVector ThreatLoc) const;

private:
	// Tuning
	float CrouchHeight = 95.f;
	float StandHeight = 180.f;
	float MinThickness = 25.f;
	float NavExtent = 150.f;
	float ChunkSizeXY = 1000.f;
	int32 SamplesPerAxis = 6;
	int32 TraceBudgetPerFrame = 96;

	// The 5 probe heights
	static constexpr int32 NUM_HEIGHTS = 5;
	float ProbeHeights[NUM_HEIGHTS] = { 15.f, 35.f, 65.f, 100.f, 170.f };

	// The 8 horizontal directions (every 45°)
	static constexpr int32 NUM_DIRECTIONS = 8;
	FVector ProbeDirections[NUM_DIRECTIONS];

	// Intermediate storage for in-flight probes
	struct FProbeSample
	{
		FVector FloorLocation = FVector::ZeroVector;
		FVector FloorNormal = FVector::UpVector;
		bool HitResults[NUM_DIRECTIONS][NUM_HEIGHTS] = {};     // true = blocked
		float HitDistances[NUM_DIRECTIONS][NUM_HEIGHTS] = {};
		AActor* HitActors[NUM_DIRECTIONS][NUM_HEIGHTS] = {};
		int32 PendingTraces = 0;
		bool bComplete = false;
	};

	// Key: (ChunkKey.X * 10000 + ChunkKey.Y) * 1000 + SampleIndex
	TMap<int64, FProbeSample> InFlightSamples;

	int64 MakeSampleKey(FIntPoint ChunkKey, int32 SampleIndex) const;
	ECoverHeight ClassifyHeight(const FProbeSample& Sample, int32 DirIndex) const;
	ELeanSide ComputeLeanSides(const FProbeSample& Sample, int32 BestDir) const;
	float ComputeArcScore(const FProbeSample& Sample) const;
	int32 FindBestDirection(const FProbeSample& Sample) const;

	void InitDirections();
	bool bDirectionsInitialized = false;
};
