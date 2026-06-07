// CoverTypes.h — Cover-specific types beyond what's in SquadTypes.h
// Chunk management, scan state, query requests
// NOTE: No USTRUCT/GENERATED_BODY here — these are plain C++ structs
#pragma once

#include "CoreMinimal.h"
#include "SquadTypes.h"

// State of a single spatial chunk
struct FCoverChunk
{
	TArray<FCoverPoint> Points;
	float LastScannedTime = -1.f;
	bool bScanInProgress = false;
	bool bScanComplete = false;
	int32 PendingTraces = 0;          // How many async traces are still in flight
	int32 NextLocalID = 0;            // Incrementing ID for points within this chunk

	bool NeedsRescan(float CurrentTime, float RevalidationInterval) const
	{
		if (!bScanComplete) return true;
		if (RevalidationInterval <= 0.f) return false;
		return (CurrentTime - LastScannedTime) > RevalidationInterval;
	}
};

// A pending cover query from an AI soldier
struct FCoverQuery
{
	TWeakObjectPtr<AActor> Querier;
	FVector QuerierLocation = FVector::ZeroVector;
	FVector ThreatLocation = FVector::ZeroVector;
	float SearchRadius = 2000.f;
	float IssuedTime = 0.f;

	// Result callback — fires on the game thread with the best cover point
	TFunction<void(bool /*bFound*/, const FCoverPoint& /*Result*/)> Callback;

	// Which chunks this query needs
	TArray<FIntPoint> RequiredChunks;
	bool bAllChunksReady = false;
};

// Spatial hash key helper
namespace CoverHash
{
	inline FIntPoint WorldToChunk(FVector WorldPos, float ChunkSizeXY)
	{
		return FIntPoint(
			FMath::FloorToInt(WorldPos.X / ChunkSizeXY),
			FMath::FloorToInt(WorldPos.Y / ChunkSizeXY)
		);
	}

	inline int32 ChunkToHash(FIntPoint ChunkKey)
	{
		return (ChunkKey.X << 16) | (ChunkKey.Y & 0xFFFF);
	}

	inline FVector ChunkCenter(FIntPoint ChunkKey, float ChunkSizeXY)
	{
		return FVector(
			(ChunkKey.X + 0.5f) * ChunkSizeXY,
			(ChunkKey.Y + 0.5f) * ChunkSizeXY,
			0.f
		);
	}
}
