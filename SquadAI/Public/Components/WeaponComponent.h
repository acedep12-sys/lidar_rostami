// WeaponComponent.h — Hitscan weapon with recoil ramp, accuracy multiplier, suppression
// Uses SoldierRegistry for O(K) bullet-proximity suppression checks
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWeaponFired, AActor*, Owner, FVector, MuzzleLocation, FVector, HitLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponHitTarget, AActor*, HitActor, float, Damage);

UENUM(BlueprintType)
enum class EWeaponState : uint8 { Idle, Firing, Cooldown };

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent, DisplayName = "Weapon"))
class SQUADAI_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponComponent();

	// ---- Config (per-instance in Blueprint, defaults from USquadAITuning) ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FireRatePerSec = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float MaxRange = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float BaseSpreadDegrees = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float SpreadPerShot = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float SpreadDecayPerSec = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float Damage = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FalloffStart = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float MinDamageMultiplier = 0.35f;

	// Accuracy multiplier — 0..1. Allies ~0.25, enemies ~0.65
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float AccuracyMultiplier = 0.65f;

	// Damage multiplier — set by DifficultySubsystem
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float DamageMultiplier = 1.f;

	// Burst config
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Burst")
	int32 BurstShots = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Burst")
	float CooldownSeconds = 0.7f;

	// Suppression (applied to nearby AI via SoldierRegistry)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Suppression")
	float SuppressionRadius = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Suppression")
	float SuppressionAmount = 0.35f;

	// Bullet speed for lead prediction (cm/s). 0 = hitscan (no lead).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ballistics")
	float BulletSpeed = 60000.f; // 600 m/s default

	// Sound played when a bullet passes near the player (near miss / whiz-by)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Audio")
	TObjectPtr<USoundBase> BulletWhizSound;

	// Muzzle socket name on skeletal mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Visual")
	FName MuzzleSocketName = FName("Muzzle");

	// ---- Runtime state ----

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	EWeaponState CurrentState = EWeaponState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	float CurrentSpread = 0.f;

	// ---- Events ----

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnWeaponFired OnFired;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnWeaponHitTarget OnHitTarget;

	// ---- API ----

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool FireAtTarget(FVector TargetLocation);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StartBurst(FVector TargetLocation);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StopFiring();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon")
	bool CanFire() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon")
	FVector GetMuzzleLocation() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool bBurstActive = false;
	FVector BurstTarget = FVector::ZeroVector;
	int32 ShotsRemaining = 0;
	float NextShotTime = 0.f;
	float CooldownUntil = 0.f;
	float LastFireTime = 0.f;

	void PerformHitscan(FVector Start, FVector Direction);
	FVector ApplySpread(FVector AimDir) const;
	float CalcFalloff(float Distance) const;
	void ApplySuppression(FVector Start, FVector End);
};
