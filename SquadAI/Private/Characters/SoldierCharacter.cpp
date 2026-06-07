// SoldierCharacter.cpp — Base soldier with components, faction, cover state
#include "Characters/SoldierCharacter.h"
#include "AI/SoldierAIController.h"
#include "Components/AimComponent.h"
#include "Components/WeaponComponent.h"
#include "Components/HealthComponent.h"
#include "Performance/AISignificanceManager.h"
#include "Performance/SoldierRegistrySubsystem.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Damage.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/DamageEvents.h"
#include "TimerManager.h"
#include "SquadAITuning.h"

ASoldierCharacter::ASoldierCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// Components
	AimComp = CreateDefaultSubobject<UAimComponent>(TEXT("AimComp"));
	WeaponComp = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComp"));
	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));

	// Perception stimuli — makes this character detectable by AI
	PerceptionStimuli = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionStimuli"));
	PerceptionStimuli->bAutoRegister = true;
	PerceptionStimuli->RegisterForSense(UAISense_Sight::StaticClass());
	PerceptionStimuli->RegisterForSense(UAISense_Hearing::StaticClass());
	PerceptionStimuli->RegisterForSense(UAISense_Damage::StaticClass());

	// Movement defaults
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->MaxWalkSpeed = 450.f;
		CMC->MaxWalkSpeedCrouched = 200.f;
		CMC->NavAgentProps.bCanCrouch = true;
		CMC->bCanWalkOffLedges = true;
		CMC->SetCrouchedHalfHeight(50.f);
	}
}

void ASoldierCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Set team from faction
	TeamId = (Faction == ESquadFaction::Enemy) ? FGenericTeamId(1) : FGenericTeamId(0);

	// Register with subsystems
	RegisterWithSystems();

	// Apply accuracy to weapon
	if (WeaponComp)
	{
		WeaponComp->AccuracyMultiplier = AccuracyMultiplier;
	}

	// Bind death event
	if (HealthComp)
	{
		HealthComp->OnDeath.AddDynamic(this, &ASoldierCharacter::OnHealthDied);
	}
}

void ASoldierCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterFromSystems();
	Super::EndPlay(EndPlayReason);
}

void ASoldierCharacter::RegisterWithSystems()
{
	if (UWorld* World = GetWorld())
	{
		if (UAISignificanceManager* SigMan = World->GetSubsystem<UAISignificanceManager>())
		{
			SigMan->RegisterSoldier(this);
		}
		if (USoldierRegistrySubsystem* Reg = World->GetSubsystem<USoldierRegistrySubsystem>())
		{
			Reg->RegisterSoldier(this);
		}
	}
}

void ASoldierCharacter::UnregisterFromSystems()
{
	if (UWorld* World = GetWorld())
	{
		if (UAISignificanceManager* SigMan = World->GetSubsystem<UAISignificanceManager>())
		{
			SigMan->UnregisterSoldier(this);
		}
		if (USoldierRegistrySubsystem* Reg = World->GetSubsystem<USoldierRegistrySubsystem>())
		{
			Reg->UnregisterSoldier(this);
		}
	}
}

float ASoldierCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	// Route through HealthComponent chokepoint
	if (HealthComp && HealthComp->IsAlive())
	{
		HealthComp->ApplyDamage(DamageAmount, DamageCauser);

		// Play hit reaction montage (flinch) — only if alive after damage
		if (HealthComp->IsAlive() && HitReactMontage)
		{
			if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
			{
				// Only play if not already playing one
				if (!AnimInst->Montage_IsPlaying(HitReactMontage))
				{
					AnimInst->Montage_Play(HitReactMontage, 1.f);
				}
			}
		}

		// Also report to UE perception (so Damage sense fires)
		UAISense_Damage::ReportDamageEvent(GetWorld(), this, DamageCauser,
			DamageAmount, GetActorLocation(),
			DamageCauser ? DamageCauser->GetActorLocation() : GetActorLocation());
	}

	return DamageAmount;
}

void ASoldierCharacter::OnHealthDied(AActor* OwnerActor)
{
	HandleDeath();
}

void ASoldierCharacter::HandleDeath()
{
	// Unregister from subsystems
	UnregisterFromSystems();

	// Stop weapon
	if (WeaponComp) WeaponComp->StopFiring();

	// Stop aim
	if (AimComp) AimComp->ClearAimTarget();

	// Stop movement
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->StopMovementImmediately();
		CMC->DisableMovement();
	}

	// Disable capsule collision (but keep mesh for ragdoll)
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Play death montage BEFORE ragdoll (if assigned)
	if (DeathMontage)
	{
		if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
		{
			AnimInst->Montage_Play(DeathMontage, 1.f);

			// After montage plays briefly, transition to ragdoll
			FTimerHandle RagdollTimer;
			GetWorldTimerManager().SetTimer(RagdollTimer, [this]()
			{
				if (USkeletalMeshComponent* MeshComp = GetMesh())
				{
					MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
					MeshComp->SetAllBodiesSimulatePhysics(true);
					MeshComp->SetSimulatePhysics(true);
					MeshComp->WakeAllRigidBodies();
				}
			}, DeathMontageToRagdollDelay, false);
		}
	}
	else
	{
		// No death montage — immediate ragdoll
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
			MeshComp->SetAllBodiesSimulatePhysics(true);
			MeshComp->SetSimulatePhysics(true);
			MeshComp->WakeAllRigidBodies();
		}
	}

	DetachFromControllerPendingDestroy();

	// Note: actual destruction handled by EnemyPoolSubsystem or SetLifeSpan
}

void ASoldierCharacter::SetCoverState(bool bInCover, bool bCrouching)
{
	bIsInCover = bInCover;
	bIsCrouchingInCover = bCrouching;

	if (bCrouching && !bIsCrouched) Crouch();
	else if (!bCrouching && bIsCrouched) UnCrouch();
}

void ASoldierCharacter::SetCurrentCover(const FCoverPoint& Cover)
{
	CurrentCoverPoint = Cover;
	bIsInCover = true;
}

void ASoldierCharacter::ClearCover()
{
	CurrentCoverPoint = FCoverPoint();
	bIsInCover = false;
	bIsCrouchingInCover = false;
	if (bIsCrouched) UnCrouch();
}

bool ASoldierCharacter::IsAlive() const
{
	return HealthComp ? HealthComp->IsAlive() : true;
}

float ASoldierCharacter::GetHealthPercent() const
{
	return HealthComp ? HealthComp->GetHealthPercent() : 1.f;
}
