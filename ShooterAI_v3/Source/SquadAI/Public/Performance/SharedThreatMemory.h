// SharedThreatMemory.h — Squad-wide threat awareness subsystem
// When ONE AI sees a threat, ALL nearby AI know about it within 1 tick.
// Fixes the "deaf AI" bug where enemies with their back turned don't know about threats.
// O(K) reads via SoldierRegistry spatial hash.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "SquadTypes.h"
#include "GenericTeamAgentInterface.h"
#include "SharedThreatMemory.generated.h"

USTRUCT()
struct FSharedThreatInfo
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr<AActor> Threat = nullptr;
	UPROPERTY() FVector LastKnownLocation = FVector::ZeroVector;
	UPROPERTY() FVector PredictedVelocity = FVector::ZeroVector;
	UPROPERTY() float Confidence = 0.f;
	UPROPERTY() float LastUpdateTime = 0.f;
	uint8 ReporterTeamId = 0; // Store as uint8 instead of FGenericTeamId (not UPROPERTY-safe)
};

UCLASS()
class SQUADAI_API USharedThreatMemorySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(USharedThreatMemorySubsystem, STATGROUP_Tickables);
	}

	// Report a threat sighting (called by any AI controller on perception update)
	void ReportThreat(AActor* Threat, FVector Location, FVector Velocity, float Confidence, uint8 ReporterTeamId);

	// Query known threats for a specific team
	TArray<FSharedThreatInfo> GetKnownThreatsForTeam(uint8 QuerierTeamId) const;

	// Get the highest-confidence known threat for a team
	bool GetBestKnownThreat(uint8 QuerierTeamId, FSharedThreatInfo& OutThreat) const;

	// Decay rate for shared confidence
	UPROPERTY(EditAnywhere, Category = "Shared Memory")
	float ConfidenceDecayRate = 0.15f;

	// How long before a shared threat is forgotten entirely
	UPROPERTY(EditAnywhere, Category = "Shared Memory")
	float ForgetAfterSeconds = 10.f;

private:
	// Key: threat actor, Value: shared info
	TMap<TWeakObjectPtr<AActor>, FSharedThreatInfo> KnownThreats;
	float ThrottleTimer = 0.f;

	void DecayAndCleanup(float DeltaTime, float CurrentTime);
};
