// CoverSystemSubsystem.h — World subsystem: spatial hash of cover chunks + async query API
// AI soldiers call RequestCoverAsync(). The subsystem manages chunks,
// kicks scans on demand, and delivers results via delegate.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "CoverTypes.h"
#include "CoverScanner.h"
#include "CoverSystemSubsystem.generated.h"

UCLASS()
class SQUADAI_API UCoverSystemSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UCoverSystemSubsystem, STATGROUP_Tickables);
	}

	// =====================================================================
	//  PUBLIC API — called by AI controllers
	// =====================================================================

	// Request the best cover point near QuerierLoc facing ThreatLoc.
	// Result delivered via Callback on the game thread (may take 1-16 frames).
	void RequestCoverAsync(
		AActor* Querier,
		FVector QuerierLocation,
		FVector ThreatLocation,
		float SearchRadius,
		TFunction<void(bool, const FCoverPoint&)> Callback);

	// Reserve a cover point (prevent others from claiming it)
	bool ReserveCover(FIntPoint ChunkKey, int32 LocalID, AActor* Claimer);

	// Release a reservation
	void ReleaseCover(FIntPoint ChunkKey, int32 LocalID);

	// Invalidate all cover in a radius (explosion, destruction)
	UFUNCTION(BlueprintCallable, Category = "Cover")
	void InvalidateArea(FVector Center, float Radius);

	// Force a scan of a specific area (called by CoverScanVolume or auto-config)
	UFUNCTION(BlueprintCallable, Category = "Cover")
	void ForceScanArea(FVector Center, float Radius);

	// Debug: get all cover points (expensive, for editor visualization only)
	UFUNCTION(BlueprintCallable, Category = "Cover|Debug")
	TArray<FCoverPoint> GetAllCoverPointsInRadius(FVector Center, float Radius) const;

	// Debug drawing toggle
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover|Debug")
	bool bDebugDraw = false;

private:
	// The spatial hash of cover chunks
	TMap<FIntPoint, FCoverChunk> Chunks;

	// The scanner (does the actual tracing work)
	FCoverScanner Scanner;

	// Pending queries from AI soldiers
	TArray<FCoverQuery> PendingQueries;

	// Scan queue: chunks that need scanning this frame
	TArray<FIntPoint> ScanQueue;

	// Per-frame bookkeeping
	int32 ChunksScannedThisFrame = 0;
	int32 QueriesResolvedThisFrame = 0;
	int32 TracesSubmittedThisFrame = 0;

	// Tuning cache (read from USquadAITuning on init)
	float CachedChunkSize = 1000.f;
	int32 CachedMaxChunkScans = 2;
	int32 CachedMaxQueries = 16;
	int32 CachedTraceBudget = 96;
	float CachedRevalidationTime = 30.f;

	// ---- Internal methods ----
	void ProcessScanQueue(float CurrentTime);
	void ProcessPendingQueries(float CurrentTime);
	void ResolveSingleQuery(FCoverQuery& Query, float CurrentTime);
	void KickChunkScan(FIntPoint ChunkKey, float CurrentTime);
	void FinalizeCompletedSamples();
	TArray<FIntPoint> GetChunksInRadius(FVector Center, float Radius) const;
	void DrawDebugCovers(UWorld* World) const;

	// Pending floor samples waiting for async traces to complete
	// Key: ChunkKey, Value: array of sample indices
	TMap<FIntPoint, TArray<int32>> PendingSamplesByChunk;
};
