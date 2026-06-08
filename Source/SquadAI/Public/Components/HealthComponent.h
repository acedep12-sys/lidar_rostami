// HealthComponent.h — Single chokepoint for all damage
// bInvincible clamps HP but still fires damage events (flinch animations work)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthDamaged, float, Damage, AActor*, DamageCauser, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthDied, AActor*, OwnerActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, NewHealthPercent);

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent, DisplayName = "Health"))
class SQUADAI_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float CurrentHealth = 100.f;

	// If true, health never reaches 0 — allies use this
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	bool bInvincible = false;

	// Minimum health when invincible (so allies can still "look hurt")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health",
		meta = (EditCondition = "bInvincible"))
	float InvincibleMinHealth = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	bool bIsDead = false;

	// ---- Events ----
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnHealthDamaged OnDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnHealthDied OnDeath;

	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnHealthChanged OnHealthChanged;

	// ---- API ----

	// THE single damage entry point. Everything goes through here.
	UFUNCTION(BlueprintCallable, Category = "Health")
	float ApplyDamage(float DamageAmount, AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	float GetHealthPercent() const { return MaxHealth > 0.f ? CurrentHealth / MaxHealth : 0.f; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Health")
	bool IsAlive() const { return !bIsDead; }

protected:
	virtual void BeginPlay() override;
};
