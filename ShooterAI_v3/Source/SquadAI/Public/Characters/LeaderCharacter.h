// LeaderCharacter.h — Squad leader: carries NavigationInvoker, follows quest waypoints
// Paces speed based on player proximity (Half-Life / TLOU style)
#pragma once

#include "CoreMinimal.h"
#include "Characters/SoldierCharacter.h"
#include "SquadTypes.h"
#include "LeaderCharacter.generated.h"

class UNavigationInvokerComponent;

UENUM(BlueprintType)
enum class ELeaderWaypointState : uint8
{
	NotReached,
	Moving,
	Arrived,
	WaitingForClear,
	Completed
};

UCLASS(BlueprintType, Blueprintable)
class SQUADAI_API ALeaderCharacter : public ASoldierCharacter
{
	GENERATED_BODY()

public:
	ALeaderCharacter();

	// ---- Components ----

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Leader")
	TObjectPtr<UNavigationInvokerComponent> NavInvokerComp;

	// ---- Pacing ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leader|Pacing")
	float WaitForPlayerRadius = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leader|Pacing")
	float FullSpeedRadius = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leader|Pacing")
	float NormalSpeed = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leader|Pacing")
	float WaitingSpeed = 120.f;

	// ---- Waypoints ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leader|Quest")
	TArray<FQuestWaypoint> QuestWaypoints;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Quest")
	int32 CurrentWaypointIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Leader|Quest")
	ELeaderWaypointState WaypointState = ELeaderWaypointState::NotReached;

	// ---- API ----

	UFUNCTION(BlueprintCallable, Category = "Leader")
	void SetQuestWaypoints(const TArray<FQuestWaypoint>& Waypoints);

	UFUNCTION(BlueprintCallable, Category = "Leader")
	void BeginQuest();

	UFUNCTION(BlueprintCallable, Category = "Leader")
	bool AdvanceToNextWaypoint();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader")
	bool HasReachedCurrentWaypoint() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader")
	bool IsCurrentAreaClear() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader")
	FVector GetCurrentWaypointLocation() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader")
	float GetPlayerDistance() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Leader")
	bool IsQuestComplete() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	float LingerTimer = 0.f;
	void UpdatePacing();
	void UpdateWaypointState(float DeltaTime);
};
