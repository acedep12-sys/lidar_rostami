// EnemySoldier.h — Enemy: mortal, aggressive, takes cover, suppression-aware
#pragma once

#include "CoreMinimal.h"
#include "Characters/SoldierCharacter.h"
#include "EnemySoldier.generated.h"

UCLASS(BlueprintType, Blueprintable)
class SQUADAI_API AEnemySoldier : public ASoldierCharacter
{
	GENERATED_BODY()

public:
	AEnemySoldier();

	// How willingly they advance under fire (0=never, 1=always)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Behavior",
		meta = (ClampMin = "0", ClampMax = "1"))
	float Aggression = 0.4f;

	// At this suppression value, refuse to peek from cover
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Behavior",
		meta = (ClampMin = "0", ClampMax = "1"))
	float SuppressionThreshold = 0.6f;

	// Attack coordinator assigns this — the world location this enemy attacks toward
	UPROPERTY(BlueprintReadWrite, Category = "Enemy|Behavior")
	FVector AssignedAttackPoint = FVector::ZeroVector;

	// Max distance from the attack point before being pulled back
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Behavior")
	float AttackPointLeash = 2500.f;

	// Tactical role assigned by the coordinator
	UPROPERTY(BlueprintReadWrite, Category = "Enemy|Behavior")
	EAttackPhase CurrentPhase = EAttackPhase::Idle;

protected:
	virtual void BeginPlay() override;
};
