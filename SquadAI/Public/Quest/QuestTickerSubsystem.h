// QuestTickerSubsystem.h — Single 4Hz tick over ONLY active missions
// Inactive missions cost ZERO. Returns false from IsTickable() when nothing active.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "QuestTickerSubsystem.generated.h"

class AQuestMission;
class AQuestDirector;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHudUpdated, const FQuestHudSnapshot&, Snapshot);

UCLASS()
class SQUADAI_API UQuestTickerSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UQuestTickerSubsystem, STATGROUP_Tickables);
	}

	// Returns false when no missions active → engine skips our tick entirely
	virtual bool IsTickable() const override;

	// HUD event — bind your widget to this
	UPROPERTY(BlueprintAssignable, Category = "Quest")
	FOnHudUpdated OnHudUpdated;

	// Register the director (auto-found if only one in level)
	void SetDirector(AQuestDirector* Dir);

private:
	TWeakObjectPtr<AQuestDirector> Director;

	void FindDirector();
	void BroadcastHUD(AQuestMission* Mission);
};
