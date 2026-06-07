// WaveSpawner.cpp — Timer-driven batched spawning with NavMesh validation
#include "Quest/WaveSpawner.h"
#include "Quest/WaveTemplate.h"
#include "Quest/QuestRegistry.h"
#include "Quest/EnemyPool.h"
#include "AI/EnemyAttackCoordinator.h"
#include "Characters/SoldierCharacter.h"
#include "Characters/EnemySoldier.h"
#include "Components/HealthComponent.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "SquadAITuning.h"

AWaveSpawner::AWaveSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	SpawnPointSearchExtent = USquadAITuning::Get()->SpawnPointNavProjectExtent;
}

void AWaveSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (SpawnerTag.IsValid())
	{
		if (UQuestRegistry* Reg = GetWorld()->GetSubsystem<UQuestRegistry>())
		{
			Reg->Register(SpawnerTag, this);
		}
	}
}

void AWaveSpawner::EndPlay(const EEndPlayReason::Type Reason)
{
	StopWaves();
	if (SpawnerTag.IsValid())
	{
		if (UQuestRegistry* Reg = GetWorld()->GetSubsystem<UQuestRegistry>())
		{
			Reg->Unregister(SpawnerTag);
		}
	}
	Super::EndPlay(Reason);
}

void AWaveSpawner::StartWaves()
{
	if (!WaveTemplate || WaveTemplate->Waves.Num() == 0) return;

	CurrentWaveIndex = -1;
	TotalSpawned = 0;
	TotalKilled = 0;
	CachedLiving = 0;
	bSpawningActive = true;

	StartNextWave();
}

void AWaveSpawner::StopWaves()
{
	bSpawningActive = false;
	GetWorldTimerManager().ClearTimer(WaveDelayTimer);
	GetWorldTimerManager().ClearTimer(BatchTimer);
}

void AWaveSpawner::StartNextWave()
{
	if (!bSpawningActive || !WaveTemplate) return;

	CurrentWaveIndex++;
	if (CurrentWaveIndex >= WaveTemplate->Waves.Num())
	{
		// All waves done — but enemies might still be alive
		return;
	}

	const FWaveDefinition& Wave = WaveTemplate->Waves[CurrentWaveIndex];

	// Delay before this wave
	if (Wave.DelayAfterPreviousSeconds > 0.f && CurrentWaveIndex > 0)
	{
		GetWorldTimerManager().SetTimer(WaveDelayTimer, [this]()
		{
			const FWaveDefinition& W = WaveTemplate->Waves[CurrentWaveIndex];
			SpawnedThisWave = 0;
			TargetThisWave = W.NumSoldiers;
			BatchesDone = 0;
			OnWaveStarted.Broadcast(CurrentWaveIndex);
			SpawnNextBatch();
		}, Wave.DelayAfterPreviousSeconds, false);
	}
	else
	{
		SpawnedThisWave = 0;
		TargetThisWave = Wave.NumSoldiers;
		BatchesDone = 0;
		OnWaveStarted.Broadcast(CurrentWaveIndex);
		SpawnNextBatch();
	}
}

void AWaveSpawner::SpawnNextBatch()
{
	if (!bSpawningActive || !WaveTemplate) return;
	if (CurrentWaveIndex >= WaveTemplate->Waves.Num()) return;

	const FWaveDefinition& Wave = WaveTemplate->Waves[CurrentWaveIndex];
	TSubclassOf<APawn> EnemyClass = Wave.EnemyClassOverride
		? Wave.EnemyClassOverride
		: (DefaultEnemyClass ? DefaultEnemyClass : WaveTemplate->DefaultEnemyClass);

	if (!EnemyClass) return;

	const int32 Remaining = TargetThisWave - SpawnedThisWave;
	const int32 ThisBatch = FMath::Min(Wave.BatchSize, Remaining);

	for (int32 i = 0; i < ThisBatch; ++i)
	{
		APawn* Enemy = SpawnOneEnemy(EnemyClass);
		if (Enemy)
		{
			SpawnedThisWave++;
			TotalSpawned++;
			CachedLiving++;
			AllSpawned.Add(Enemy);
			OnEnemySpawned.Broadcast(Enemy);

			// Register with coordinator if available
			if (Coordinator.IsValid())
			{
				if (AEnemySoldier* ES = Cast<AEnemySoldier>(Enemy))
				{
					Coordinator->AddEnemy(ES);
				}
			}

			// Subscribe to death
			if (UHealthComponent* HC = Enemy->FindComponentByClass<UHealthComponent>())
			{
				HC->OnDeath.AddDynamic(this, &AWaveSpawner::HandleSpawnedDied);
			}
		}
	}

	// More batches needed?
	if (SpawnedThisWave < TargetThisWave)
	{
		GetWorldTimerManager().SetTimer(BatchTimer, this,
			&AWaveSpawner::SpawnNextBatch, Wave.SecondsBetweenBatches, false);
	}
}

APawn* AWaveSpawner::SpawnOneEnemy(TSubclassOf<APawn> Class)
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	// Try enemy pool first
	UEnemyPoolSubsystem* Pool = World->GetSubsystem<UEnemyPoolSubsystem>();
	if (Pool)
	{
		APawn* Pooled = Pool->Acquire(Class, GetNextSpawnLocation(), GetActorRotation());
		if (Pooled) return Pooled;
	}

	// Fallback: direct spawn
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	return World->SpawnActor<APawn>(Class, GetNextSpawnLocation(), GetActorRotation(), Params);
}

FVector AWaveSpawner::GetNextSpawnLocation()
{
	if (SpawnPointLocations.Num() == 0) return GetActorLocation();

	// Round-robin through spawn points
	SpawnPointRoundRobin = (SpawnPointRoundRobin + 1) % SpawnPointLocations.Num();
	FVector Loc = SpawnPointLocations[SpawnPointRoundRobin];

	// Add random offset
	Loc += FVector(FMath::RandRange(-200.f, 200.f), FMath::RandRange(-200.f, 200.f), 0.f);

	// NavMesh project
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (NavSys)
	{
		FNavLocation NavLoc;
		if (NavSys->ProjectPointToNavigation(Loc, NavLoc,
			FVector(SpawnPointSearchExtent, SpawnPointSearchExtent, 400.f)))
		{
			Loc = NavLoc.Location;
		}
	}

	return Loc;
}

void AWaveSpawner::HandleSpawnedDied(AActor* DeadActor)
{
	CachedLiving = FMath::Max(0, CachedLiving - 1);
	TotalKilled++;

	// Release to pool after a delay (corpse linger)
	if (APawn* P = Cast<APawn>(DeadActor))
	{
		UWorld* World = GetWorld();
		const float Linger = USquadAITuning::Get()->CorpseLingerSeconds;

		FTimerHandle ReleaseTimer;
		World->GetTimerManager().SetTimer(ReleaseTimer, [World, P]()
		{
			if (UEnemyPoolSubsystem* Pool = World->GetSubsystem<UEnemyPoolSubsystem>())
			{
				Pool->Release(P);
			}
			else
			{
				P->Destroy();
			}
		}, Linger, false);
	}

	// Auto-compact stale weak refs when the array is 2x the living count
	if (AllSpawned.Num() > CachedLiving * 2 + 10)
	{
		CompactStaleRefs();
	}

	// Check if current wave is cleared
	if (CachedLiving <= 0 && SpawnedThisWave >= TargetThisWave)
	{
		OnWaveCleared.Broadcast(CurrentWaveIndex);

		if (CurrentWaveIndex + 1 >= WaveTemplate->Waves.Num())
		{
			OnAllWavesCleared.Broadcast();
			bSpawningActive = false;
		}
		else
		{
			StartNextWave();
		}
	}
}

void AWaveSpawner::CompactStaleRefs()
{
	AllSpawned.RemoveAll([](const TWeakObjectPtr<APawn>& P) { return !P.IsValid(); });
}

bool AWaveSpawner::AreAllWavesComplete() const
{
	return WaveTemplate && CurrentWaveIndex >= WaveTemplate->Waves.Num() - 1
		&& SpawnedThisWave >= TargetThisWave && CachedLiving <= 0;
}

int32 AWaveSpawner::GetTotalWaveCount() const
{
	return WaveTemplate ? WaveTemplate->Waves.Num() : 0;
}
