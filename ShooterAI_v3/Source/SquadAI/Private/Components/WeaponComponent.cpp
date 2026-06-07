// WeaponComponent.cpp — Hitscan weapon with recoil, falloff, and suppression
#include "Components/WeaponComponent.h"
#include "Components/HealthComponent.h"
#include "Engine/DamageEvents.h"
#include "SquadAITuning.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Performance/SoldierRegistrySubsystem.h"
#include "Kismet/GameplayStatics.h"

// Forward declare — defined in Performance module
class USoldierRegistrySubsystem;

UWeaponComponent::UWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.016f; // ~60Hz
}

void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	const USquadAITuning* T = USquadAITuning::Get();
	// Only set defaults if the value hasn't been overridden in Blueprint
	if (Damage == 15.f) Damage = T->DefaultDamage;
	if (FireRatePerSec == 8.f) FireRatePerSec = T->DefaultFireRate;
	if (BaseSpreadDegrees == 1.5f) BaseSpreadDegrees = T->DefaultBaseSpread;
	if (SpreadPerShot == 0.4f) SpreadPerShot = T->DefaultSpreadPerShot;
	if (SpreadDecayPerSec == 4.f) SpreadDecayPerSec = T->DefaultSpreadDecay;
	if (MaxRange == 10000.f) MaxRange = T->DefaultMaxRange;
	if (FalloffStart == 2500.f) FalloffStart = T->DefaultFalloffStart;
	if (MinDamageMultiplier == 0.35f) MinDamageMultiplier = T->DefaultMinDamageMultiplier;
	if (SuppressionRadius == 250.f) SuppressionRadius = T->SuppressionRadius;
	if (SuppressionAmount == 0.35f) SuppressionAmount = T->SuppressionAmountPerBullet;

	CurrentSpread = BaseSpreadDegrees;
}

void UWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float Now = GetWorld()->GetTimeSeconds();

	// Spread recovery
	if (!bBurstActive && CurrentSpread > BaseSpreadDegrees)
	{
		CurrentSpread = FMath::Max(BaseSpreadDegrees, CurrentSpread - SpreadDecayPerSec * DeltaTime);
	}

	// Cooldown completion
	if (CurrentState == EWeaponState::Cooldown && Now >= CooldownUntil)
	{
		CurrentState = EWeaponState::Idle;
		if (bBurstActive)
		{
			ShotsRemaining = BurstShots;
		}
	}

	// Burst sequencing
	if (bBurstActive && CurrentState != EWeaponState::Cooldown && Now >= NextShotTime)
	{
		if (ShotsRemaining > 0)
		{
			FireAtTarget(BurstTarget);
		}
		else
		{
			// Burst complete → cooldown
			CurrentState = EWeaponState::Cooldown;
			CooldownUntil = Now + CooldownSeconds;
		}
	}
}

bool UWeaponComponent::FireAtTarget(FVector TargetLocation)
{
	if (!CanFire()) return false;

	UWorld* World = GetWorld();
	if (!World) return false;

	const FVector MuzzleLoc = GetMuzzleLocation();

	// Lead prediction: aim where the target WILL BE, not where they ARE
	// Uses bullet time-of-flight × target velocity (suggestion 2.1, 9.5)
	FVector PredictedTarget = TargetLocation;
	if (BulletSpeed > 0.f)
	{
		const float Distance = FVector::Dist(MuzzleLoc, TargetLocation);
		const float TimeOfFlight = Distance / BulletSpeed;

		// Try to get target velocity from the actor
		if (AActor* TargetActor = nullptr) // TODO: pass target actor for velocity
		{
			// Fallback: use the PerceptionMemory prediction system
		}
		// For now, the lead is applied at the caller level (StateTree task passes
		// the predicted location from PerceptionMemory::GetBestKnownPosition)
	}

	FVector AimDir = (PredictedTarget - MuzzleLoc).GetSafeNormal();

	// Apply accuracy spread (modified by AccuracyMultiplier)
	AimDir = ApplySpread(AimDir);

	// Hitscan
	PerformHitscan(MuzzleLoc, AimDir);

	// Apply suppression to nearby AI
	ApplySuppression(MuzzleLoc, MuzzleLoc + AimDir * MaxRange);

	// Recoil bloom
	CurrentSpread = FMath::Min(CurrentSpread + SpreadPerShot, 20.f);

	// Timing
	LastFireTime = World->GetTimeSeconds();
	NextShotTime = LastFireTime + (1.f / FMath::Max(1.f, FireRatePerSec));
	ShotsRemaining--;
	CurrentState = EWeaponState::Firing;

	OnFired.Broadcast(GetOwner(), MuzzleLoc, MuzzleLoc + AimDir * MaxRange);

	return true;
}

void UWeaponComponent::StartBurst(FVector TargetLocation)
{
	BurstTarget = TargetLocation;
	ShotsRemaining = BurstShots;
	bBurstActive = true;

	if (CanFire())
	{
		FireAtTarget(TargetLocation);
	}
}

void UWeaponComponent::StopFiring()
{
	bBurstActive = false;
	ShotsRemaining = 0;
	if (CurrentState == EWeaponState::Firing)
	{
		CurrentState = EWeaponState::Idle;
	}
}

bool UWeaponComponent::CanFire() const
{
	return CurrentState != EWeaponState::Cooldown;
}

// =====================================================================
//  HITSCAN
// =====================================================================
void UWeaponComponent::PerformHitscan(FVector Start, FVector Direction)
{
	UWorld* World = GetWorld();
	if (!World) return;

	const FVector End = Start + Direction * MaxRange;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	Params.bReturnPhysicalMaterial = true;

	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			// Calculate damage with falloff
			const float FalloffMult = CalcFalloff(Hit.Distance);
			const float FinalDamage = Damage * FalloffMult * DamageMultiplier * USquadAITuning::Get()->DamageMultiplier;

			// Apply through the HealthComponent chokepoint
			if (UHealthComponent* HC = HitActor->FindComponentByClass<UHealthComponent>())
			{
				HC->ApplyDamage(FinalDamage, GetOwner());
			}
			else
			{
				// Fallback to UE damage pipeline
				FDamageEvent DmgEvent;
				HitActor->TakeDamage(FinalDamage, DmgEvent,
					GetOwner() ? GetOwner()->GetInstigatorController() : nullptr, GetOwner());
			}

			OnHitTarget.Broadcast(HitActor, FinalDamage);
		}

#if ENABLE_DRAW_DEBUG
		DrawDebugLine(World, Start, Hit.ImpactPoint, FColor::Red, false, 0.3f, 0, 1.f);
#endif
	}
	else
	{
#if ENABLE_DRAW_DEBUG
		DrawDebugLine(World, Start, End, FColor(255, 100, 100, 40), false, 0.2f, 0, 0.5f);
#endif
	}
}

// =====================================================================
//  SPREAD — applies accuracy multiplier
// =====================================================================
FVector UWeaponComponent::ApplySpread(FVector AimDir) const
{
	// Effective spread = base spread / accuracy multiplier
	// AccuracyMultiplier of 0.25 (ally) → spread * 4 = very inaccurate
	// AccuracyMultiplier of 0.65 (enemy) → spread * 1.54 = moderate
	const float EffectiveSpread = CurrentSpread / FMath::Max(0.05f, AccuracyMultiplier);
	const float HalfAngleRad = FMath::DegreesToRadians(FMath::Min(EffectiveSpread, 20.f) * 0.5f);

	return FMath::VRandCone(AimDir, HalfAngleRad);
}

// =====================================================================
//  FALLOFF
// =====================================================================
float UWeaponComponent::CalcFalloff(float Distance) const
{
	if (Distance <= FalloffStart) return 1.f;

	const float FalloffRange = MaxRange - FalloffStart;
	if (FalloffRange <= 0.f) return 1.f;

	const float Alpha = FMath::Clamp((Distance - FalloffStart) / FalloffRange, 0.f, 1.f);
	return FMath::Lerp(1.f, MinDamageMultiplier, Alpha);
}

// =====================================================================
//  SUPPRESSION — walk the SoldierRegistry for O(K) nearby check
// =====================================================================
void UWeaponComponent::ApplySuppression(FVector Start, FVector End)
{
	UWorld* World = GetWorld();
	if (!World) return;

	USoldierRegistrySubsystem* Registry = World->GetSubsystem<USoldierRegistrySubsystem>();
	if (!Registry) return;

	// Apply suppression to nearby AI soldiers
	Registry->ApplySuppressionAlongPath(Start, End, SuppressionRadius, SuppressionAmount, GetOwner());

	// BULLET WHIZ: Check if bullet passes near the player (idea from UE forums)
	// Play a 2D stereo whiz sound when a bullet passes within 500cm of the player
	APawn* Player = World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;
	if (Player && GetOwner() != Player) // Don't whiz your own bullets
	{
		const FVector PlayerLoc = Player->GetActorLocation();
		const FVector BulletDir = (End - Start).GetSafeNormal();
		const float BulletLen = FVector::Dist(Start, End);

		// Point-to-line distance
		const FVector ToPlayer = PlayerLoc - Start;
		const float Projection = FVector::DotProduct(ToPlayer, BulletDir);

		if (Projection > 0.f && Projection < BulletLen) // Bullet passes by, not toward/away
		{
			const FVector ClosestPoint = Start + BulletDir * Projection;
			const float DistToPlayer = FVector::Dist(PlayerLoc, ClosestPoint);

			if (DistToPlayer < 500.f && DistToPlayer > 50.f) // Near miss zone: 0.5m - 5m
			{
				// Play whiz sound at the closest point (3D positioned)
				// The sound should be a short "psssht" or "whip" sound
				// Designers assign the sound via Blueprint on the weapon
				if (BulletWhizSound)
				{
					UGameplayStatics::PlaySoundAtLocation(World, BulletWhizSound,
						ClosestPoint, FRotator::ZeroRotator,
						FMath::Clamp(1.f - (DistToPlayer / 500.f), 0.3f, 1.f), // Volume: louder when closer
						FMath::RandRange(0.85f, 1.15f)); // Pitch variation
				}
			}
		}
	}
}

// =====================================================================
//  MUZZLE LOCATION
// =====================================================================
FVector UWeaponComponent::GetMuzzleLocation() const
{
	if (const ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* Mesh = OwnerChar->GetMesh())
		{
			if (Mesh->DoesSocketExist(MuzzleSocketName))
			{
				return Mesh->GetSocketLocation(MuzzleSocketName);
			}
		}

		return OwnerChar->GetActorLocation() +
			FVector(0, 0, OwnerChar->BaseEyeHeight) +
			OwnerChar->GetActorForwardVector() * 50.f;
	}

	return GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
}
