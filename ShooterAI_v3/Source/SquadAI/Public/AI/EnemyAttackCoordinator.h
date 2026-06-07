// EnemyAttackCoordinator.h — Coordinates a group of enemies through 5 tactical phases
// Gather → Suppress → Flank → Assault → Hold
// Each enemy's own StateTree handles cover/shooting — the coordinator just sets attack points
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SquadTypes.h"
#include "EnemyAttackCoordinator.generated.h"

class AEnemySoldier;

UCLASS(BlueprintType, Blueprintable)
class SQUADAI_API AEnemyAttackCoordinator : public AActor
{
	GENERATED_BODY()

public:
	AEnemyAttackCoordinator();

	// ---- Config (set in Details panel) ----

	// What's being attacked
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coordinator")
	FVector AttackPointLocation = FVector::ZeroVector;

	// Where enemies muster before pressing forward
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coordinator")
	FVector StagingAreaLocation = FVector::ZeroVector;

	// Fraction of group that flanks during Flank phase
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coordinator", meta = (ClampMin = "0", ClampMax = "1"))
	float FlankerRatio = 0.4f;

	// Minimum enemies to start an attack
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coordinator", meta = (ClampMin = "1"))
	int32 MinEnemiesToAttack = 4;

	// Phase durations
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coordinator")
	float SuppressPhaseSeconds = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coordinator")
	float FlankPhaseSeconds = 6.f;

	// ---- Runtime ----

	UPROPERTY(BlueprintReadOnly, Category = "Coordinator")
	EAttackPhase CurrentPhase = EAttackPhase::Idle;

	// ---- API ----

	UFUNCTION(BlueprintCallable, Category = "Coordinator")
	void AddEnemy(AEnemySoldier* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Coordinator")
	void RemoveEnemy(AEnemySoldier* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Coordinator")
	void BeginAttack();

	UFUNCTION(BlueprintCallable, Category = "Coordinator")
	void AbortAttack();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Coordinator")
	int32 GetAliveEnemyCount() const;

protected:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<AEnemySoldier>> Enemies;

	float PhaseTimer = 0.f;

	void TransitionTo(EAttackPhase NewPhase);
	void AssignRoles();
	void SetAllAttackPoints(FVector Point);
	TArray<AEnemySoldier*> GetAliveEnemies() const;

	// Flanking: compute a point to the side of the attack point
	FVector ComputeFlankPoint() const;
};
