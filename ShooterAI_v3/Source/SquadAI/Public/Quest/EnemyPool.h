// EnemyPool.h — Pre-warm enemy pool: zero GC churn during waves
// Acquire = O(1), no SpawnActor. Release = hide + sleep. Reuse on next wave.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnemyPool.generated.h"

UCLASS()
class SQUADAI_API UEnemyPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Pre-spawn N enemies of the given class (call from GameMode BeginPlay)
	UFUNCTION(BlueprintCallable, Category = "Enemy Pool")
	void Prewarm(TSubclassOf<APawn> EnemyClass, int32 Count);

	// Get an enemy from the pool (or nullptr if pool is empty for that class)
	APawn* Acquire(TSubclassOf<APawn> EnemyClass, FVector Location, FRotator Rotation);

	// Return an enemy to the pool (called on death after linger delay)
	void Release(APawn* Enemy);

	// How many are available in the pool
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Enemy Pool")
	int32 GetAvailableCount(TSubclassOf<APawn> EnemyClass) const;

private:
	struct FPoolEntry
	{
		TWeakObjectPtr<APawn> Pawn;
		bool bAvailable = true;
	};

	// Pooled enemies per class
	TMap<UClass*, TArray<FPoolEntry>> Pool;

	void SleepEnemy(APawn* Enemy);
	void WakeEnemy(APawn* Enemy, FVector Location, FRotator Rotation);
};
