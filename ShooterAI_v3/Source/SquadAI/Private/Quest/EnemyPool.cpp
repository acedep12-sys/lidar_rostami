// EnemyPool.cpp — Pre-warmed enemy pool for zero-GC wave spawning
#include "Quest/EnemyPool.h"
#include "Characters/SoldierCharacter.h"
#include "Components/HealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "SquadAITuning.h"

void UEnemyPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UEnemyPoolSubsystem::Deinitialize()
{
	// Destroy all pooled enemies
	for (auto& Pair : Pool)
	{
		for (FPoolEntry& Entry : Pair.Value)
		{
			if (APawn* P = Entry.Pawn.Get())
			{
				P->Destroy();
			}
		}
	}
	Pool.Empty();
	Super::Deinitialize();
}

void UEnemyPoolSubsystem::Prewarm(TSubclassOf<APawn> EnemyClass, int32 Count)
{
	if (!EnemyClass || Count <= 0) return;

	UWorld* World = GetWorld();
	if (!World) return;

	TArray<FPoolEntry>& ClassPool = Pool.FindOrAdd(EnemyClass.Get());

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 i = 0; i < Count; ++i)
	{
		// Spawn far away and hidden
		APawn* Enemy = World->SpawnActor<APawn>(EnemyClass,
			FVector(0, 0, -10000.f), FRotator::ZeroRotator, Params);

		if (Enemy)
		{
			SleepEnemy(Enemy);

			FPoolEntry Entry;
			Entry.Pawn = Enemy;
			Entry.bAvailable = true;
			ClassPool.Add(Entry);
		}
	}

	UE_LOG(LogSquadAI, Log, TEXT("EnemyPool: prewarmed %d of %s"), Count, *EnemyClass->GetName());
}

APawn* UEnemyPoolSubsystem::Acquire(TSubclassOf<APawn> EnemyClass, FVector Location, FRotator Rotation)
{
	if (!EnemyClass) return nullptr;

	TArray<FPoolEntry>* ClassPool = Pool.Find(EnemyClass.Get());
	if (!ClassPool) return nullptr;

	for (FPoolEntry& Entry : *ClassPool)
	{
		if (Entry.bAvailable && Entry.Pawn.IsValid())
		{
			Entry.bAvailable = false;
			WakeEnemy(Entry.Pawn.Get(), Location, Rotation);
			return Entry.Pawn.Get();
		}
	}

	return nullptr; // Pool exhausted — caller falls back to SpawnActor
}

void UEnemyPoolSubsystem::Release(APawn* Enemy)
{
	if (!Enemy) return;

	for (auto& Pair : Pool)
	{
		for (FPoolEntry& Entry : Pair.Value)
		{
			if (Entry.Pawn.Get() == Enemy)
			{
				SleepEnemy(Enemy);
				Entry.bAvailable = true;
				return;
			}
		}
	}
}

int32 UEnemyPoolSubsystem::GetAvailableCount(TSubclassOf<APawn> EnemyClass) const
{
	if (!EnemyClass) return 0;

	const TArray<FPoolEntry>* ClassPool = Pool.Find(EnemyClass.Get());
	if (!ClassPool) return 0;

	int32 Count = 0;
	for (const FPoolEntry& E : *ClassPool) { if (E.bAvailable) Count++; }
	return Count;
}

void UEnemyPoolSubsystem::SleepEnemy(APawn* Enemy)
{
	if (!Enemy) return;

	Enemy->SetActorHiddenInGame(true);
	Enemy->SetActorEnableCollision(false);
	Enemy->SetActorTickEnabled(false);
	Enemy->SetActorLocation(FVector(0, 0, -10000.f));

	if (ACharacter* Char = Cast<ACharacter>(Enemy))
	{
		if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
		{
			CMC->StopMovementImmediately();
			CMC->DisableMovement();
		}
	}

	// Reset health
	if (UHealthComponent* HC = Enemy->FindComponentByClass<UHealthComponent>())
	{
		HC->CurrentHealth = HC->MaxHealth;
		HC->bIsDead = false;
	}

	// Detach controller (will be re-possessed on wake)
	if (AController* C = Enemy->GetController())
	{
		C->UnPossess();
	}
}

void UEnemyPoolSubsystem::WakeEnemy(APawn* Enemy, FVector Location, FRotator Rotation)
{
	if (!Enemy) return;

	Enemy->SetActorLocation(Location);
	Enemy->SetActorRotation(Rotation);
	Enemy->SetActorHiddenInGame(false);
	Enemy->SetActorEnableCollision(true);
	Enemy->SetActorTickEnabled(true);

	if (ACharacter* Char = Cast<ACharacter>(Enemy))
	{
		if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
		{
			CMC->SetMovementMode(MOVE_Walking);
		}

		// Re-enable ragdoll mesh
		if (USkeletalMeshComponent* Mesh = Char->GetMesh())
		{
			Mesh->SetSimulatePhysics(false);
			Mesh->SetCollisionProfileName(TEXT("CharacterMesh"));
		}

		// Re-enable capsule
		if (UCapsuleComponent* Capsule = Char->GetCapsuleComponent())
		{
			Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
	}

	// Reset health
	if (UHealthComponent* HC = Enemy->FindComponentByClass<UHealthComponent>())
	{
		HC->CurrentHealth = HC->MaxHealth;
		HC->bIsDead = false;
	}

	// Spawn default controller
	Enemy->SpawnDefaultController();
}
