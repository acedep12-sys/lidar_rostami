// AISignificanceManager.h — Simple distance-based LOD for AI soldiers
// Evaluates significance per-soldier, maps to 6 buckets, adjusts tick rates
// Uses a lightweight tick-based approach compatible with installed UE builds
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AISignificanceManager.generated.h"

class ASoldierCharacter;

UENUM(BlueprintType)
enum class EAISignificance : uint8
{
	Critical,   // Full fidelity — closest/most visible
	High,
	Medium,
	Low,
	VeryLow,
	Dormant     // Minimal tick, mesh URO max
};

UCLASS()
class SQUADAI_API UAISignificanceManager : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UAISignificanceManager, STATGROUP_Tickables);
	}

	void RegisterSoldier(ASoldierCharacter* Soldier);
	void UnregisterSoldier(ASoldierCharacter* Soldier);

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<ASoldierCharacter>> RegisteredSoldiers;

	float TimeSinceLastUpdate = 0.f;
	float UpdateInterval = 0.25f; // 4 Hz

	float EvaluateSignificance(ASoldierCharacter* Soldier) const;
	void ApplySignificanceBucket(ASoldierCharacter* Soldier, EAISignificance Bucket);
	EAISignificance SignificanceToBucket(float Significance) const;
};
