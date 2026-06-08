// SoldierAIController.h — Base AI controller
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SquadTypes.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "GameplayTasksComponent.h"
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
	ASoldierAIController(const FObjectInitializer& OI = FObjectInitializer::Get());

	// ---- Mission Data (Direct Drive) ----
	UPROPERTY(BlueprintReadWrite, Category = "AI|Mission")
	FVector CurrentGoalLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "AI|Mission")
	bool bHasActiveGoal = false;

	// ---- Team Logic ----
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }
	virtual void SetGenericTeamId(const FGenericTeamId& InID) override { TeamId = InID; }
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FGenericTeamId TeamId;

	// ---- Components ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> PerceptionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UGameplayTasksComponent> TasksComp;

	// ---- Suppression ----
	UPROPERTY(BlueprintReadOnly, Category = "AI|Suppression")
	float Suppression = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Suppression")
	float SuppressionDecayPerSec = 0.4f;

	UFUNCTION(BlueprintCallable, Category = "AI|Suppression")
	void AddSuppression(float Amount);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Suppression")
	float GetSuppression() const { return Suppression; }

	// ---- Threat Memory ----
	UPROPERTY(BlueprintReadOnly, Category = "AI|Memory")
	TArray<FPerceivedTarget> Memory;

	UFUNCTION(BlueprintCallable, Category = "AI|Memory")
	AActor* GetPrimaryThreat(float& OutConfidence) const;

	UFUNCTION(BlueprintCallable, Category = "AI|Memory")
	TArray<AActor*> GetCurrentlyVisibleHostiles() const;

	// ---- Cover ----
	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	FCoverPoint PendingCoverResult;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	bool bHasPendingCover = false;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	FCoverPoint CurrentCover;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Cover")
	bool bHasCover = false;

	UFUNCTION(BlueprintCallable, Category = "AI|Cover")
	void RequestCover(FVector ThreatLocation, float SearchRadius = 2000.f);

	// ---- LOD & Performance ----
	UPROPERTY(BlueprintReadOnly, Category = "AI|LOD")
	int32 CurrentLODBucket = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AI|LOD")
	bool bForceFullTick = false;

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
