// DifficultySubsystem.h — One-call difficulty change
// SetDifficulty(EGameDifficulty) walks every registered soldier and adjusts stats
// Uses SoldierRegistry (O(K)), not TActorIterator (O(N))
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SquadTypes.h"
#include "DifficultySubsystem.generated.h"

UCLASS()
class SQUADAI_API UDifficultySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// THE one-call difficulty setter
	UFUNCTION(BlueprintCallable, Category = "Difficulty")
	void SetDifficulty(EGameDifficulty NewDifficulty);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Difficulty")
	EGameDifficulty GetCurrentDifficulty() const { return CurrentDifficulty; }

	// Static accessor
	static UDifficultySubsystem* Get(const UObject* WorldContext);

private:
	EGameDifficulty CurrentDifficulty = EGameDifficulty::Normal;

	float GetAllyPower(EGameDifficulty Diff) const;
	float GetEnemyDamage(EGameDifficulty Diff) const;
	float GetEnemyAccuracy(EGameDifficulty Diff) const;
};
