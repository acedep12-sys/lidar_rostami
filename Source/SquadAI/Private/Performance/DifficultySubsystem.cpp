// DifficultySubsystem.cpp — One-call difficulty adjustment
#include "Performance/DifficultySubsystem.h"
#include "Performance/SoldierRegistrySubsystem.h"
#include "EngineUtils.h"
#include "Characters/SoldierCharacter.h"
#include "Characters/AllySoldier.h"
#include "Characters/EnemySoldier.h"
#include "Components/WeaponComponent.h"
#include "SquadAITuning.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UDifficultySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentDifficulty = EGameDifficulty::Normal;
}

UDifficultySubsystem* UDifficultySubsystem::Get(const UObject* WorldContext)
{
	if (!WorldContext) return nullptr;

	UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContext);
	return GI ? GI->GetSubsystem<UDifficultySubsystem>() : nullptr;
}

void UDifficultySubsystem::SetDifficulty(EGameDifficulty NewDifficulty)
{
	CurrentDifficulty = NewDifficulty;

	const float AllyPower = GetAllyPower(NewDifficulty);
	const float EnemyDmg = GetEnemyDamage(NewDifficulty);
	const float EnemyAcc = GetEnemyAccuracy(NewDifficulty);

	// Walk the world via SoldierRegistry if available, else TActorIterator as fallback
	UWorld* World = GetWorld();
	if (!World) return;

	USoldierRegistrySubsystem* Registry = World->GetSubsystem<USoldierRegistrySubsystem>();

	// Get all soldiers (query a huge radius or iterate)
	TArray<ASoldierCharacter*> AllSoldiers;

	if (Registry)
	{
		// Use the registry — O(all registered), which is the same as all soldiers
		AllSoldiers = Registry->QueryRadius(FVector::ZeroVector, 999999.f);
	}
	else
	{
		// Fallback
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (ASoldierCharacter* S = Cast<ASoldierCharacter>(*It))
			{
				AllSoldiers.Add(S);
			}
		}
	}

	for (ASoldierCharacter* S : AllSoldiers)
	{
		if (AAllySoldier* Ally = Cast<AAllySoldier>(S))
		{
			Ally->PowerLevel = AllyPower;
			Ally->ApplyPowerLevel();
		}
		else if (AEnemySoldier* Enemy = Cast<AEnemySoldier>(S))
		{
			Enemy->AccuracyMultiplier = EnemyAcc;
			if (Enemy->WeaponComp)
			{
				Enemy->WeaponComp->AccuracyMultiplier = EnemyAcc;
				Enemy->WeaponComp->DamageMultiplier = EnemyDmg;
			}
		}
	}

	UE_LOG(LogSquadAI, Log, TEXT("Difficulty set to %s: AllyPower=%.2f, EnemyDmg=%.2f, EnemyAcc=%.2f"),
		*UEnum::GetValueAsString(NewDifficulty), AllyPower, EnemyDmg, EnemyAcc);
}

float UDifficultySubsystem::GetAllyPower(EGameDifficulty Diff) const
{
	const USquadAITuning* T = USquadAITuning::Get();
	switch (Diff)
	{
	case EGameDifficulty::Easy:   return T->AllyPower_Easy;
	case EGameDifficulty::Normal: return T->AllyPower_Normal;
	case EGameDifficulty::Hard:   return T->AllyPower_Hard;
	case EGameDifficulty::Lethal: return T->AllyPower_Lethal;
	default: return T->AllyPower_Normal;
	}
}

float UDifficultySubsystem::GetEnemyDamage(EGameDifficulty Diff) const
{
	const USquadAITuning* T = USquadAITuning::Get();
	switch (Diff)
	{
	case EGameDifficulty::Easy:   return T->EnemyDamage_Easy;
	case EGameDifficulty::Normal: return T->EnemyDamage_Normal;
	case EGameDifficulty::Hard:   return T->EnemyDamage_Hard;
	case EGameDifficulty::Lethal: return T->EnemyDamage_Lethal;
	default: return T->EnemyDamage_Normal;
	}
}

float UDifficultySubsystem::GetEnemyAccuracy(EGameDifficulty Diff) const
{
	const USquadAITuning* T = USquadAITuning::Get();
	switch (Diff)
	{
	case EGameDifficulty::Easy:   return T->EnemyAccuracy_Easy;
	case EGameDifficulty::Normal: return T->EnemyAccuracy_Normal;
	case EGameDifficulty::Hard:   return T->EnemyAccuracy_Hard;
	case EGameDifficulty::Lethal: return T->EnemyAccuracy_Lethal;
	default: return T->EnemyAccuracy_Normal;
	}
}
