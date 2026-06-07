// HealthComponent.cpp — Damage chokepoint with invincibility
#include "Components/HealthComponent.h"
#include "SquadAITuning.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	const USquadAITuning* T = USquadAITuning::Get();
	if (MaxHealth <= 0.f) MaxHealth = T->DefaultMaxHealth;
	if (bInvincible && InvincibleMinHealth <= 0.f) InvincibleMinHealth = T->InvincibleMinHealth;

	CurrentHealth = MaxHealth;
}

float UHealthComponent::ApplyDamage(float DamageAmount, AActor* DamageCauser)
{
	if (bIsDead || DamageAmount <= 0.f) return 0.f;

	CurrentHealth -= DamageAmount;

	// Invincible: clamp but still fire events
	if (bInvincible)
	{
		CurrentHealth = FMath::Max(CurrentHealth, InvincibleMinHealth);
	}

	// Broadcast damage event (for flinch animations, hit reactions, HUD)
	OnDamaged.Broadcast(DamageAmount, DamageCauser, CurrentHealth);
	OnHealthChanged.Broadcast(GetHealthPercent());

	// Death check
	if (CurrentHealth <= 0.f && !bInvincible)
	{
		bIsDead = true;
		OnDeath.Broadcast(GetOwner());
	}

	return DamageAmount;
}

void UHealthComponent::Heal(float Amount)
{
	if (bIsDead || Amount <= 0.f) return;

	CurrentHealth = FMath::Min(CurrentHealth + Amount, MaxHealth);
	OnHealthChanged.Broadcast(GetHealthPercent());
}
