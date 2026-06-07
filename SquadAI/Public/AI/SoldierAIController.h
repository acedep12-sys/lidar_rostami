// SoldierAIController.h — Base AI controller
// Detour Crowd (RVO), 3 perception senses, fading memory, suppression, StateTree
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SquadTypes.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "SoldierAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAISenseConfig_Damage;
class UStateTreeAIComponent;
class ASoldierCharacter;

UCLASS(BlueprintType, Blueprintable)
class SQUADAI_API ASoldierAIController : public AAIController
{
	GENERATED_BODY()

public:
	// Use CrowdFollowingComponent for RVO avoidance between all AI
	ASoldierAIController(const FObjectInitializer& OI = FObjectInitializer::Get());

	// ---- Components ----

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> PerceptionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComp;

	// ---- Suppression ----

	UPROPERTY(BlueprintReadOnly, Category = "AI|Suppression")
	float Suppression = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Suppression")
	float SuppressionDecayPerSec = 0.4f;

	// Add suppression (called by WeaponComponent via SoldierRegistry)
	UFUNCTION(BlueprintCallable, Category = "AI|Suppression")
	void AddSuppression(float Amount);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Suppression")
	float GetSuppression() const { return Suppression; }

	// ---- Threat Memory ----

	UPROPERTY(BlueprintReadOnly, Category = "AI|Memory")
	TArray<FPerceivedTarget> Memory;

	// Get the highest-priority threat (scored by confidence + distance)
	UFUNCTION(BlueprintCallable, Category = "AI|Memory")
	AActor* GetPrimaryThreat(float& OutConfidence) const;

	// Get all currently VISIBLE hostile actors (engine-maintained cache)
	UFUNCTION(BlueprintCallable, Category = "AI|Memory")
	TArray<AActor*> GetCurrentlyVisibleHostiles() const;

	// ---- Cover ----

	// Pending cover result — StateTree tasks poll this
	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	FCoverPoint PendingCoverResult;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	bool bHasPendingCover = false;

	// Current cover being used
	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	FCoverPoint CurrentCover;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	bool bHasCover = false;

	// Request cover from the subsystem — result stored in PendingCoverResult
	UFUNCTION(BlueprintCallable, Category = "AI|Cover")
	void RequestCover(FVector ThreatLocation, float SearchRadius = 2000.f);

	// ---- LOD & Performance ----

	// Current LOD bucket (0=Critical, 5=Dormant, set by AISignificanceManager)
	UPROPERTY(BlueprintReadOnly, Category = "AI|LOD")
	int32 CurrentLODBucket = 0;

	// Force a full-fidelity tick on next update (set by reactive events)
	UPROPERTY(BlueprintReadWrite, Category = "AI|LOD")
	bool bForceFullTick = false;

	// Time of last forced tick (to decay back to normal after 1 second)
	float ForceFullTickUntil = 0.f;

	// ---- Helpers ----

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI")
	ASoldierCharacter* GetSoldierCharacter() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI")
	bool HasLineOfSightTo(AActor* Target) const;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void SetupPerception();
	void UpdateMemory(float DeltaTime);

private:
	UPROPERTY() TObjectPtr<UAISenseConfig_Sight>   SightConfig;
	UPROPERTY() TObjectPtr<UAISenseConfig_Hearing>  HearingConfig;
	UPROPERTY() TObjectPtr<UAISenseConfig_Damage>   DamageConfig;

	float MemoryDecayRate = 0.2f;
};
