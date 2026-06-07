// SoldierRegistrySubsystem.h — Sparse 2D spatial hash for O(K) soldier queries
// Every soldier registers on BeginPlay. Bullets query nearby for suppression.
// Rebuilds buckets every 0.2s. O(K) where K = nearby soldiers, not O(N total).
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "SoldierRegistrySubsystem.generated.h"

class ASoldierCharacter;
class ASoldierAIController;

UCLASS()
class SQUADAI_API USoldierRegistrySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(USoldierRegistrySubsystem, STATGROUP_Tickables);
	}

	// ---- Registration ----

	void RegisterSoldier(ASoldierCharacter* Soldier);
	void UnregisterSoldier(ASoldierCharacter* Soldier);

	// ---- Queries ----

	// Find all soldiers within Radius of Center — O(K) not O(N)
	TArray<ASoldierCharacter*> QueryRadius(FVector Center, float Radius) const;

	// Apply suppression to all AI within Radius of a bullet path
	void ApplySuppressionAlongPath(FVector Start, FVector End, float Radius, float Amount, AActor* Ignore = nullptr);

	// Count hostiles near a point (for leader area-clear check)
	int32 CountHostilesNear(FVector Center, float Radius, uint8 FriendlyTeamId) const;

private:
	// All registered soldiers
	UPROPERTY()
	TArray<TWeakObjectPtr<ASoldierCharacter>> AllSoldiers;

	// 2D spatial hash buckets
	struct FBucket
	{
		TArray<ASoldierCharacter*> Soldiers;
	};

	TMap<FIntPoint, FBucket> Buckets;
	float CellSize = 4000.f;
	float RebuildInterval = 0.2f;
	float TimeSinceRebuild = 0.f;

	void RebuildBuckets();
	FIntPoint WorldToCell(FVector Pos) const;
	TArray<FIntPoint> GetCellsInRadius(FVector Center, float Radius) const;
};
