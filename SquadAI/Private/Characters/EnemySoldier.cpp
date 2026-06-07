// EnemySoldier.cpp — Enemy with tuning-driven stats
#include "Characters/EnemySoldier.h"
#include "Components/WeaponComponent.h"
#include "Components/HealthComponent.h"
#include "SquadAITuning.h"

AEnemySoldier::AEnemySoldier()
{
	Faction = ESquadFaction::Enemy;
}

void AEnemySoldier::BeginPlay()
{
	Super::BeginPlay();

	const USquadAITuning* T = USquadAITuning::Get();

	AccuracyMultiplier = T->EnemyAccuracyMultiplier;
	Aggression = T->EnemyDefaultAggression;
	SuppressionThreshold = T->EnemySuppressionThreshold;

	if (WeaponComp)
	{
		WeaponComp->AccuracyMultiplier = AccuracyMultiplier;
	}

	if (HealthComp)
	{
		HealthComp->bInvincible = false;
		HealthComp->MaxHealth = T->DefaultMaxHealth;
	}
}
