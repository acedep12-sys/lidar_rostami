// CoverSystemSubsystem.cpp — Spatial hash + async query dispatcher
#include "CoverSystem/CoverSystemSubsystem.h"
#include "SquadAITuning.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

// =====================================================================
//  LIFECYCLE
// =====================================================================
void UCoverSystemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const USquadAITuning* T = USquadAITuning::Get();
	CachedChunkSize = T->ChunkSizeXY;
	CachedMaxChunkScans = T->MaxChunkScansPerFrame;
	CachedMaxQueries = T->MaxQueriesPerFrame;
	CachedTraceBudget = T->AsyncTraceBudgetPerFrame;
	CachedRevalidationTime = T->ChunkRevalidationSeconds;

	Scanner.Configure(T->CrouchHeight, T->StandHeight, T->MinCoverThickness,
	                  T->NavmeshProjectExtent, T->ChunkSizeXY, T->SamplesPerChunkAxis,
	                  T->AsyncTraceBudgetPerFrame);

	UE_LOG(LogSquadAI, Log, TEXT("CoverSystem initialized: chunk=%.0f, samples=%d, traceBudget=%d"),
		CachedChunkSize, T->SamplesPerChunkAxis, CachedTraceBudget);
}

void UCoverSystemSubsystem::Deinitialize()
{
	Chunks.Empty();
	PendingQueries.Empty();
	ScanQueue.Empty();
	Super::Deinitialize();
}

// =====================================================================
//  TICK — drives the async pipeline each frame
// =====================================================================
void UCoverSystemSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	ChunksScannedThisFrame = 0;
	QueriesResolvedThisFrame = 0;
	TracesSubmittedThisFrame = 0;

	// 1. Finalize any completed async trace samples
	FinalizeCompletedSamples();

	// 2. Process scan queue (kick new chunk scans, budgeted)
	ProcessScanQueue(CurrentTime);

	// 3. Process pending queries (resolve if chunks are ready)
	ProcessPendingQueries(CurrentTime);

	// 4. Debug drawing
	if (bDebugDraw)
	{
		DrawDebugCovers(GetWorld());
	}
}

// =====================================================================
//  REQUEST COVER — AI soldier asks for the best cover
// =====================================================================
void UCoverSystemSubsystem::RequestCoverAsync(
	AActor* Querier, FVector QuerierLocation, FVector ThreatLocation,
	float SearchRadius, TFunction<void(bool, const FCoverPoint&)> Callback)
{
	FCoverQuery Query;
	Query.Querier = Querier;
	Query.QuerierLocation = QuerierLocation;
	Query.ThreatLocation = ThreatLocation;
	Query.SearchRadius = SearchRadius;
	Query.Callback = MoveTemp(Callback);
	Query.IssuedTime = GetWorld()->GetTimeSeconds();

	// Figure out which chunks we need
	Query.RequiredChunks = GetChunksInRadius(QuerierLocation, SearchRadius);

	// Queue scan for any unscanned chunks
	for (const FIntPoint& ChunkKey : Query.RequiredChunks)
	{
		FCoverChunk* Chunk = Chunks.Find(ChunkKey);
		if (!Chunk || Chunk->NeedsRescan(Query.IssuedTime, CachedRevalidationTime))
		{
			ScanQueue.AddUnique(ChunkKey);
		}
	}

	PendingQueries.Add(MoveTemp(Query));
}

// =====================================================================
//  PROCESS SCAN QUEUE — kick chunk scans up to budget
// =====================================================================
void UCoverSystemSubsystem::ProcessScanQueue(float CurrentTime)
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (int32 i = ScanQueue.Num() - 1; i >= 0 && ChunksScannedThisFrame < CachedMaxChunkScans; --i)
	{
		const FIntPoint ChunkKey = ScanQueue[i];
		FCoverChunk& Chunk = Chunks.FindOrAdd(ChunkKey);

		if (Chunk.bScanInProgress) continue;

		KickChunkScan(ChunkKey, CurrentTime);
		ScanQueue.RemoveAtSwap(i, EAllowShrinking::No);
		ChunksScannedThisFrame++;
	}
}

void UCoverSystemSubsystem::KickChunkScan(FIntPoint ChunkKey, float CurrentTime)
{
	UWorld* World = GetWorld();
	FCoverChunk& Chunk = Chunks.FindOrAdd(ChunkKey);

	Chunk.bScanInProgress = true;
	Chunk.bScanComplete = false;
	Chunk.Points.Empty();
	Chunk.NextLocalID = 0;

	// Phase 1: Generate floor samples
	TArray<FVector> FloorSamples = Scanner.GenerateFloorSamples(ChunkKey, World);

	if (FloorSamples.Num() == 0)
	{
		Chunk.bScanInProgress = false;
		Chunk.bScanComplete = true;
		Chunk.LastScannedTime = CurrentTime;
		return;
	}

	// Phase 2: Submit async traces for each sample (budgeted)
	int32 TotalTraces = 0;
	TArray<int32>& SampleIndices = PendingSamplesByChunk.FindOrAdd(ChunkKey);
	SampleIndices.Empty();

	for (int32 i = 0; i < FloorSamples.Num(); ++i)
	{
		if (TracesSubmittedThisFrame >= CachedTraceBudget) break;

		int32 TracesQueued = Scanner.SubmitAsyncProbes(FloorSamples[i], ChunkKey, World);
		TracesSubmittedThisFrame += TracesQueued;
		TotalTraces += TracesQueued;
		SampleIndices.Add(i);
	}

	Chunk.PendingTraces = TotalTraces;

	UE_LOG(LogSquadAI, Verbose, TEXT("Cover: kicked scan for chunk (%d,%d), %d samples, %d traces"),
		ChunkKey.X, ChunkKey.Y, FloorSamples.Num(), TotalTraces);
}

// =====================================================================
//  FINALIZE — check for completed async trace batches
// =====================================================================
void UCoverSystemSubsystem::FinalizeCompletedSamples()
{
	for (auto It = PendingSamplesByChunk.CreateIterator(); It; ++It)
	{
		const FIntPoint ChunkKey = It.Key();
		TArray<int32>& Samples = It.Value();
		FCoverChunk* Chunk = Chunks.Find(ChunkKey);
		if (!Chunk) continue;

		for (int32 i = Samples.Num() - 1; i >= 0; --i)
		{
			if (Scanner.IsSampleComplete(ChunkKey, Samples[i]))
			{
				Scanner.FinalizeAndScore(ChunkKey, Samples[i], Chunks);
				Samples.RemoveAtSwap(i);
			}
		}

		if (Samples.Num() == 0 && Chunk->bScanInProgress)
		{
			Chunk->bScanInProgress = false;
			Chunk->bScanComplete = true;
			Chunk->LastScannedTime = GetWorld()->GetTimeSeconds();
		}
	}
}

// =====================================================================
//  PROCESS QUERIES — resolve pending AI requests
// =====================================================================
void UCoverSystemSubsystem::ProcessPendingQueries(float CurrentTime)
{
	for (int32 i = PendingQueries.Num() - 1; i >= 0; --i)
	{
		if (QueriesResolvedThisFrame >= CachedMaxQueries) break;

		FCoverQuery& Query = PendingQueries[i];

		// Check if all required chunks are ready
		bool bReady = true;
		for (const FIntPoint& ChunkKey : Query.RequiredChunks)
		{
			const FCoverChunk* Chunk = Chunks.Find(ChunkKey);
			if (!Chunk || !Chunk->bScanComplete)
			{
				bReady = false;
				break;
			}
		}

		// Timeout after 1 second
		if (!bReady && (CurrentTime - Query.IssuedTime) < 1.f)
			continue;

		if (bReady)
		{
			ResolveSingleQuery(Query, CurrentTime);
		}
		else
		{
			// Timed out — return failure
			if (Query.Callback)
			{
				Query.Callback(false, FCoverPoint());
			}
		}

		PendingQueries.RemoveAtSwap(i, EAllowShrinking::No);
		QueriesResolvedThisFrame++;
	}
}

void UCoverSystemSubsystem::ResolveSingleQuery(FCoverQuery& Query, float CurrentTime)
{
	FCoverPoint BestPoint;
	float BestScore = -1.f;

	for (const FIntPoint& ChunkKey : Query.RequiredChunks)
	{
		const FCoverChunk* Chunk = Chunks.Find(ChunkKey);
		if (!Chunk) continue;

		for (const FCoverPoint& CP : Chunk->Points)
		{
			if (CP.IsReserved()) continue; // Skip claimed covers

			const float Dist = FVector::Dist(CP.Location, Query.QuerierLocation);
			if (Dist > Query.SearchRadius) continue;

			// Score against threat
			float Score = Scanner.ScoreAgainstThreat(CP, Query.ThreatLocation);

			// Distance bonus (closer = better, within reason)
			const float DistScore = 1.f - FMath::Clamp(Dist / Query.SearchRadius, 0.f, 1.f);
			Score = Score * 0.7f + DistScore * 0.3f;

			if (Score > BestScore)
			{
				BestScore = Score;
				BestPoint = CP;
			}
		}
	}

	if (Query.Callback)
	{
		Query.Callback(BestScore > 0.f, BestPoint);
	}
}

// =====================================================================
//  RESERVATION
// =====================================================================
bool UCoverSystemSubsystem::ReserveCover(FIntPoint ChunkKey, int32 LocalID, AActor* Claimer)
{
	FCoverChunk* Chunk = Chunks.Find(ChunkKey);
	if (!Chunk) return false;

	for (FCoverPoint& CP : Chunk->Points)
	{
		if (CP.LocalID == LocalID)
		{
			if (CP.IsReserved()) return false;
			CP.ReservedBy = Claimer;
			return true;
		}
	}
	return false;
}

void UCoverSystemSubsystem::ReleaseCover(FIntPoint ChunkKey, int32 LocalID)
{
	FCoverChunk* Chunk = Chunks.Find(ChunkKey);
	if (!Chunk) return;

	for (FCoverPoint& CP : Chunk->Points)
	{
		if (CP.LocalID == LocalID)
		{
			CP.ReservedBy = nullptr;
			return;
		}
	}
}

// =====================================================================
//  INVALIDATION
// =====================================================================
void UCoverSystemSubsystem::InvalidateArea(FVector Center, float Radius)
{
	TArray<FIntPoint> AffectedChunks = GetChunksInRadius(Center, Radius);
	for (const FIntPoint& Key : AffectedChunks)
	{
		Chunks.Remove(Key);
	}

	UE_LOG(LogSquadAI, Log, TEXT("Cover: invalidated %d chunks around %s (r=%.0f)"),
		AffectedChunks.Num(), *Center.ToString(), Radius);
}

void UCoverSystemSubsystem::ForceScanArea(FVector Center, float Radius)
{
	TArray<FIntPoint> ChunkKeys = GetChunksInRadius(Center, Radius);
	for (const FIntPoint& Key : ChunkKeys)
	{
		ScanQueue.AddUnique(Key);
	}
}

// =====================================================================
//  UTILITIES
// =====================================================================
TArray<FIntPoint> UCoverSystemSubsystem::GetChunksInRadius(FVector Center, float Radius) const
{
	TArray<FIntPoint> Result;

	const FIntPoint MinChunk = CoverHash::WorldToChunk(Center - FVector(Radius, Radius, 0), CachedChunkSize);
	const FIntPoint MaxChunk = CoverHash::WorldToChunk(Center + FVector(Radius, Radius, 0), CachedChunkSize);

	for (int32 X = MinChunk.X; X <= MaxChunk.X; ++X)
	{
		for (int32 Y = MinChunk.Y; Y <= MaxChunk.Y; ++Y)
		{
			Result.Add(FIntPoint(X, Y));
		}
	}

	return Result;
}

TArray<FCoverPoint> UCoverSystemSubsystem::GetAllCoverPointsInRadius(FVector Center, float Radius) const
{
	TArray<FCoverPoint> Result;
	TArray<FIntPoint> ChunkKeys = GetChunksInRadius(Center, Radius);

	for (const FIntPoint& Key : ChunkKeys)
	{
		const FCoverChunk* Chunk = Chunks.Find(Key);
		if (!Chunk) continue;

		for (const FCoverPoint& CP : Chunk->Points)
		{
			if (FVector::Dist(CP.Location, Center) <= Radius)
			{
				Result.Add(CP);
			}
		}
	}

	return Result;
}

void UCoverSystemSubsystem::DrawDebugCovers(UWorld* World) const
{
	if (!World) return;

	for (const auto& Pair : Chunks)
	{
		for (const FCoverPoint& CP : Pair.Value.Points)
		{
			FColor Color;
			switch (CP.Height)
			{
			case ECoverHeight::High:    Color = FColor::Green; break;
			case ECoverHeight::Low:     Color = FColor::Yellow; break;
			case ECoverHeight::Partial: Color = FColor::Orange; break;
			default:                    Color = FColor::Red; break;
			}

			if (CP.IsReserved()) Color = FColor::Purple;

			DrawDebugBox(World, CP.Location, FVector(12.f), FQuat::Identity, Color, false, 0.3f, 0, 2.f);
			DrawDebugDirectionalArrow(World, CP.Location + FVector(0,0,20.f),
				CP.Location + FVector(0,0,20.f) + CP.Normal * 60.f,
				15.f, FColor::Cyan, false, 0.3f, 0, 1.5f);
		}
	}
}
