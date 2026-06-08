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
#include "Engine/DamageEvents.h"
#include "TimerManager.h"
#include "SquadAITuning.h"
#include "Engine/World.h"

ASoldierCharacter::ASoldierCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	AimComp = CreateDefaultSubobject<UAimComponent>(TEXT("AimComp"));
	WeaponComp = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComp"));
	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));

	bUseControllerRotationYaw = false; 
	if (UCharacterMovementComponent* CMC = GetCharacterMovement()) {
		CMC->bOrientRotationToMovement = true; 
		CMC->bUseControllerDesiredRotation = true;
		CMC->RotationRate = FRotator(0.f, 720.f, 0.f); // Very fast for tight corners
		CMC->bRequestedMoveUseAcceleration = true; 
		CMC->MaxWalkSpeed = 400.f;
		CMC->MaxStepHeight = 40.f; 
	}

	PerceptionStimuli = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionStimuli"));
	PerceptionStimuli->bAutoRegister = true;
	PerceptionStimuli->RegisterForSense(UAISense_Sight::StaticClass());
	PerceptionStimuli->RegisterForSense(UAISense_Hearing::StaticClass());
	PerceptionStimuli->RegisterForSense(UAISense_Damage::StaticClass());
}

void ASoldierCharacter::BeginPlay() {
	Super::BeginPlay();
	TeamId = (Faction == ESquadFaction::Enemy) ? FGenericTeamId(1) : FGenericTeamId(0);
	RegisterWithSystems();
	if (WeaponComp) WeaponComp->AccuracyMultiplier = AccuracyMultiplier;
	if (HealthComp) HealthComp->OnDeath.AddDynamic(this, &ASoldierCharacter::OnHealthDied);
}

void ASoldierCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason) { UnregisterFromSystems(); Super::EndPlay(EndPlayReason); }

float ASoldierCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) {
	if (HealthComp && HealthComp->IsAlive()) {
		HealthComp->ApplyDamage(DamageAmount, DamageCauser);
		if (HealthComp->IsAlive() && HitReactMontage) {
			if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
				if (!AnimInst->Montage_IsPlaying(HitReactMontage)) AnimInst->Montage_Play(HitReactMontage, 1.f);
		}
		UAISense_Damage::ReportDamageEvent(GetWorld(), this, DamageCauser, DamageAmount, GetActorLocation(), DamageCauser ? DamageCauser->GetActorLocation() : GetActorLocation());
	}
	return DamageAmount;
}

void ASoldierCharacter::OnHealthDied(AActor* OwnerActor) { HandleDeath(); }
void ASoldierCharacter::HandleDeath() {
	UnregisterFromSystems();
	if (WeaponComp) WeaponComp->StopFiring(); if (AimComp) AimComp->ClearAimTarget();
	if (GetCharacterMovement()) { GetCharacterMovement()->StopMovementImmediately(); GetCharacterMovement()->DisableMovement(); }
	if (UCapsuleComponent* Capsule = GetCapsuleComponent()) Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (DeathMontage && GetMesh()) {
		if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance()) AnimInst->Montage_Play(DeathMontage, 1.f);
	}
	DetachFromControllerPendingDestroy();
}

void ASoldierCharacter::RegisterWithSystems() {
	if (UWorld* World = GetWorld()) {
		if (UAISignificanceManager* SM = World->GetSubsystem<UAISignificanceManager>()) SM->RegisterSoldier(this);
		if (USoldierRegistrySubsystem* Reg = World->GetSubsystem<USoldierRegistrySubsystem>()) Reg->RegisterSoldier(this);
	}
}

void ASoldierCharacter::UnregisterFromSystems() {
	if (UWorld* World = GetWorld()) {
		if (UAISignificanceManager* SM = World->GetSubsystem<UAISignificanceManager>()) SM->UnregisterSoldier(this);
		if (USoldierRegistrySubsystem* Reg = World->GetSubsystem<USoldierRegistrySubsystem>()) Reg->UnregisterSoldier(this);
	}
}

void ASoldierCharacter::SetCoverState(bool bInCover, bool bCrouching) {
	bIsInCover = bInCover; bIsCrouchingInCover = bCrouching;
	if (bCrouching && !bIsCrouched) Crouch(); else if (!bCrouching && bIsCrouched) UnCrouch();
}

void ASoldierCharacter::SetCurrentCover(const FCoverPoint& Cover) { CurrentCoverPoint = Cover; bIsInCover = true; }
void ASoldierCharacter::ClearCover() { bIsInCover = false; if (bIsCrouched) UnCrouch(); }
bool ASoldierCharacter::IsAlive() const { return HealthComp ? HealthComp->IsAlive() : true; }
float ASoldierCharacter::GetHealthPercent() const { return HealthComp ? HealthComp->GetHealthPercent() : 1.f; }
