// QuestObjective.h — Base class + 4 built-in objective types
// Instanced UObjects owned by QuestMission. Designer picks type, sets tag, done.
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Quest/QuestTypes.h"
#include "QuestObjective.generated.h"

class AQuestZone;
class AWaveSpawner;

// =====================================================================
//  BASE CLASS
// =====================================================================

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class SQUADAI_API UQuestObjective : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FText DisplayText;

	UPROPERTY(BlueprintReadOnly, Category = "Objective")
	EObjectiveState State = EObjectiveState::Inactive;

	// Called by QuestMission when this objective becomes active
	virtual void OnStart(UWorld* World) { State = EObjectiveState::Active; }

	// Called at the mission's tick rate (4 Hz)
	virtual void Tick(float DeltaTime, UWorld* World) {}

	// Called when the objective should clean up
	virtual void OnFinish() { State = EObjectiveState::Inactive; }

	bool IsComplete() const { return State == EObjectiveState::Succeeded; }
	bool IsFailed() const { return State == EObjectiveState::Failed; }
};

// =====================================================================
//  AUTO ZONE OBJECTIVE — the "one-field" objective
//  Designer picks a Tag and Mode. System auto-resolves the zone + spawner.
// =====================================================================

UCLASS(BlueprintType, Blueprintable, EditInlineNew, meta = (DisplayName = "Auto Zone Objective"))
class SQUADAI_API UQuestObjective_AutoZone : public UQuestObjective
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FQuestTag ZoneTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	EObjectiveMode Mode = EObjectiveMode::Auto;

	// For Defend: max seconds player can be outside zone before failure (-1 = no limit)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective",
		meta = (EditCondition = "Mode == EObjectiveMode::Defend"))
	float MaxSecondsOutsideZone = -1.f;

	virtual void OnStart(UWorld* World) override;
	virtual void Tick(float DeltaTime, UWorld* World) override;
	virtual void OnFinish() override;

private:
	TWeakObjectPtr<AQuestZone> CachedZone;
	TWeakObjectPtr<AWaveSpawner> CachedSpawner;
	EObjectiveMode ResolvedMode = EObjectiveMode::Reach;
	float OutsideTimer = 0.f;
	int32 InitialHostileCount = 0;

	void ResolveMode(UWorld* World);
};
