// SoldierCharacter.h — Base character for all AI soldiers
// Owns AimComponent, WeaponComponent, HealthComponent, PerceptionStimuli
// Faction-driven: team ID, accuracy, invincibility all set from faction
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "SquadTypes.h"
#include "SoldierCharacter.generated.h"

class UAimComponent;
class UWeaponComponent;
class UHealthComponent;
class UAIPerceptionStimuliSourceComponent;

UCLASS(Abstract, BlueprintType, Blueprintable)
class SQUADAI_API ASoldierCharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ASoldierCharacter();

	// ---- Components ----

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soldier|Components")
	TObjectPtr<UAimComponent> AimComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soldier|Components")
	TObjectPtr<UWeaponComponent> WeaponComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soldier|Components")
	TObjectPtr<UHealthComponent> HealthComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soldier|Components")
	TObjectPtr<UAIPerceptionStimuliSourceComponent> PerceptionStimuli;

	// ---- Faction ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Faction")
	ESquadFaction Faction = ESquadFaction::Enemy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Faction")
	float AccuracyMultiplier = 0.65f;

	// ---- Team Interface ----

	virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }
	virtual void SetGenericTeamId(const FGenericTeamId& InTeamId) override { TeamId = InTeamId; }

	// ---- Cover State ----

	UPROPERTY(BlueprintReadOnly, Category = "Soldier|Cover")
	bool bIsInCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier|Cover")
	bool bIsCrouchingInCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "Soldier|Cover")
	FCoverPoint CurrentCoverPoint;

	UFUNCTION(BlueprintCallable, Category = "Soldier|Cover")
	void SetCoverState(bool bInCover, bool bCrouching);

	UFUNCTION(BlueprintCallable, Category = "Soldier|Cover")
	void SetCurrentCover(const FCoverPoint& Cover);

	UFUNCTION(BlueprintCallable, Category = "Soldier|Cover")
	void ClearCover();

	// ---- Convenience ----

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier")
	bool IsAlive() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier")
	bool IsEnemy() const { return Faction == ESquadFaction::Enemy; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier")
	bool IsAlly() const { return Faction == ESquadFaction::Ally; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldier")
	float GetHealthPercent() const;

	// ---- Animation hooks (assign in Blueprint subclass) ----

	// Played on death BEFORE ragdoll (e.g. a short 0.5s death animation)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	// Played on taking damage (flinch)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation")
	TObjectPtr<UAnimMontage> HitReactMontage;

	// Time to wait after death montage before enabling ragdoll
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier|Animation")
	float DeathMontageToRagdollDelay = 0.3f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION()
	void OnHealthDied(AActor* OwnerActor);

	virtual void HandleDeath();

	void RegisterWithSystems();
	void UnregisterFromSystems();

	FGenericTeamId TeamId;
};
