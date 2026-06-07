// EnemyAttackCoordinator.cpp — 5-phase tactical group attack
#include "AI/EnemyAttackCoordinator.h"
#include "Characters/EnemySoldier.h"
#include "Components/HealthComponent.h"

AEnemyAttackCoordinator::AEnemyAttackCoordinator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f; // 4 Hz
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AEnemyAttackCoordinator::AddEnemy(AEnemySoldier* Enemy)
{
	if (Enemy)
	{
		Enemies.AddUnique(Enemy);
	}
}

void AEnemyAttackCoordinator::RemoveEnemy(AEnemySoldier* Enemy)
{
	Enemies.Remove(Enemy);
}

void AEnemyAttackCoordinator::BeginAttack()
{
	if (GetAliveEnemyCount() < MinEnemiesToAttack)
	{
		UE_LOG(LogSquadAI, Warning, TEXT("Coordinator: not enough enemies to attack (%d < %d)"),
			GetAliveEnemyCount(), MinEnemiesToAttack);
		return;
	}

	SetActorTickEnabled(true);
	TransitionTo(EAttackPhase::Gather);
}

void AEnemyAttackCoordinator::AbortAttack()
{
	TransitionTo(EAttackPhase::Idle);
	SetActorTickEnabled(false);
}

void AEnemyAttackCoordinator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentPhase == EAttackPhase::Idle) return;

	// Check if we're out of enemies
	if (GetAliveEnemyCount() == 0)
	{
		AbortAttack();
		return;
	}

	PhaseTimer += DeltaTime;

	switch (CurrentPhase)
	{
	case EAttackPhase::Gather:
	{
		// Wait until most enemies are near the staging area (or timeout)
		int32 NearStaging = 0;
		for (AEnemySoldier* E : GetAliveEnemies())
		{
			if (FVector::Dist(E->GetActorLocation(), StagingAreaLocation) < 800.f)
				NearStaging++;
		}

		if (NearStaging >= GetAliveEnemyCount() * 0.6f || PhaseTimer > 15.f)
		{
			TransitionTo(EAttackPhase::Suppress);
		}
		break;
	}

	case EAttackPhase::Suppress:
		if (PhaseTimer >= SuppressPhaseSeconds)
		{
			TransitionTo(EAttackPhase::Flank);
		}
		break;

	case EAttackPhase::Flank:
		if (PhaseTimer >= FlankPhaseSeconds)
		{
			TransitionTo(EAttackPhase::Assault);
		}
		break;

	case EAttackPhase::Assault:
		// Assault until all enemies are close to the attack point (or most are dead)
		{
			int32 NearObjective = 0;
			for (AEnemySoldier* E : GetAliveEnemies())
			{
				if (FVector::Dist(E->GetActorLocation(), AttackPointLocation) < 600.f)
					NearObjective++;
			}

			if (NearObjective >= GetAliveEnemyCount() * 0.5f || PhaseTimer > 20.f)
			{
				TransitionTo(EAttackPhase::Hold);
			}
		}
		break;

	case EAttackPhase::Hold:
		// Hold indefinitely — enemies defend the captured position
		break;

	default:
		break;
	}
}

void AEnemyAttackCoordinator::TransitionTo(EAttackPhase NewPhase)
{
	CurrentPhase = NewPhase;
	PhaseTimer = 0.f;

	UE_LOG(LogSquadAI, Log, TEXT("Coordinator: phase → %s (%d alive enemies)"),
		*UEnum::GetValueAsString(NewPhase), GetAliveEnemyCount());

	switch (NewPhase)
	{
	case EAttackPhase::Gather:
		SetAllAttackPoints(StagingAreaLocation);
		break;

	case EAttackPhase::Suppress:
		// Everyone finds cover facing the attack point and shoots
		SetAllAttackPoints(AttackPointLocation);
		for (AEnemySoldier* E : GetAliveEnemies())
		{
			E->CurrentPhase = EAttackPhase::Suppress;
		}
		break;

	case EAttackPhase::Flank:
	{
		AssignRoles(); // Split into flankers and suppressors
		FVector FlankPt = ComputeFlankPoint();

		for (AEnemySoldier* E : GetAliveEnemies())
		{
			if (E->CurrentPhase == EAttackPhase::Flank)
			{
				E->AssignedAttackPoint = FlankPt;
			}
			// Suppressors keep their current attack point
		}
		break;
	}

	case EAttackPhase::Assault:
		// Everyone pushes to the objective
		SetAllAttackPoints(AttackPointLocation);
		for (AEnemySoldier* E : GetAliveEnemies())
		{
			E->CurrentPhase = EAttackPhase::Assault;
		}
		break;

	case EAttackPhase::Hold:
		SetAllAttackPoints(AttackPointLocation);
		for (AEnemySoldier* E : GetAliveEnemies())
		{
			E->CurrentPhase = EAttackPhase::Hold;
		}
		break;

	case EAttackPhase::Idle:
		for (AEnemySoldier* E : GetAliveEnemies())
		{
			E->CurrentPhase = EAttackPhase::Idle;
		}
		break;
	}
}

void AEnemyAttackCoordinator::AssignRoles()
{
	TArray<AEnemySoldier*> Alive = GetAliveEnemies();
	const int32 NumFlankers = FMath::CeilToInt(Alive.Num() * FlankerRatio);

	for (int32 i = 0; i < Alive.Num(); ++i)
	{
		if (i < NumFlankers)
		{
			Alive[i]->CurrentPhase = EAttackPhase::Flank;
		}
		else
		{
			Alive[i]->CurrentPhase = EAttackPhase::Suppress;
		}
	}
}

void AEnemyAttackCoordinator::SetAllAttackPoints(FVector Point)
{
	for (AEnemySoldier* E : GetAliveEnemies())
	{
		E->AssignedAttackPoint = Point;
	}
}

FVector AEnemyAttackCoordinator::ComputeFlankPoint() const
{
	// Flank point is 90° to the right of the staging→attack vector
	const FVector Forward = (AttackPointLocation - StagingAreaLocation).GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(Forward, FVector::UpVector);

	// Offset by a significant distance to actually flank
	return AttackPointLocation + Right * 1500.f;
}

int32 AEnemyAttackCoordinator::GetAliveEnemyCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AEnemySoldier>& E : Enemies)
	{
		if (E.IsValid() && E->IsAlive()) Count++;
	}
	return Count;
}

TArray<AEnemySoldier*> AEnemyAttackCoordinator::GetAliveEnemies() const
{
	TArray<AEnemySoldier*> Result;
	for (const TWeakObjectPtr<AEnemySoldier>& E : Enemies)
	{
		if (E.IsValid() && E->IsAlive())
		{
			Result.Add(E.Get());
		}
	}
	return Result;
}
