// AllySoldier.h — Ally: immortal, PowerLevel-driven accuracy, follows leader
// PowerLevel (0-1) is the single master dial that controls everything
#pragma once

#include "CoreMinimal.h"
#include "Characters/SoldierCharacter.h"
#include "AllySoldier.generated.h"

UCLASS(BlueprintType, Blueprintable)
class SQUADAI_API AAllySoldier : public ASoldierCharacter
{
	GENERATED_BODY()

public:
	AAllySoldier();

	// ---- The ONE knob that controls ally strength ----

	// 0 = barely hits, 1 = elite. Default 0.25 = shoots often, hits rarely
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally|Power",
		meta = (ClampMin = "0", ClampMax = "1"))
	float PowerLevel = 0.25f;

	// Per-stat overrides. -1 = derive from PowerLevel. Set >= 0 to override.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally|Power")
	float AccuracyOverride = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally|Power")
	float DamageMultiplierOverride = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally|Power")
	float FireRateMultiplierOverride = -1.f;

	// ---- Formation ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally|Formation")
	int32 FormationSlot = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ally|Formation")
	TWeakObjectPtr<AActor> LeaderOverride = nullptr;

	// ---- API ----

	// Applies PowerLevel to weapon + health. Called in BeginPlay and by DifficultySubsystem.
	UFUNCTION(BlueprintCallable, Category = "Ally")
	void ApplyPowerLevel();

protected:
	virtual void BeginPlay() override;
};
