// AutoSpawnZone.h — ONE actor drop = full encounter
// Creates: QuestZone + WaveSpawner + EnemyAttackCoordinator + N spawn points
// All auto-tagged, auto-registered, auto-wired. Editor previews with arrows.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Quest/QuestTypes.h"
#include "AutoSpawnZone.generated.h"

class AQuestZone;
class AWaveSpawner;
class AEnemyAttackCoordinator;
class UWaveTemplate;
class UBillboardComponent;
class UArrowComponent;

UCLASS(BlueprintType, Blueprintable)
class SQUADAI_API AAutoSpawnZone : public AActor
{
	GENERATED_BODY()

public:
	AAutoSpawnZone();

	// ---- Configuration ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoZone")
	FQuestTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoZone")
	FVector ZoneExtents = FVector(1500.f, 1500.f, 400.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoZone")
	TSubclassOf<APawn> DefaultEnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoZone")
	TObjectPtr<UWaveTemplate> WaveTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoZone",
		meta = (ClampMin = "3", ClampMax = "12"))
	int32 NumSpawnPoints = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoZone")
	float SpawnPointRadius = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoZone")
	bool bUseAttackCoordinator = true;

	// ---- Runtime refs ----
	UPROPERTY(BlueprintReadOnly, Category = "AutoZone|Runtime")
	TObjectPtr<AQuestZone> SpawnedZone;

	UPROPERTY(BlueprintReadOnly, Category = "AutoZone|Runtime")
	TObjectPtr<AWaveSpawner> SpawnedSpawner;

	UPROPERTY(BlueprintReadOnly, Category = "AutoZone|Runtime")
	TObjectPtr<AEnemyAttackCoordinator> SpawnedCoordinator;

	// ---- API ----
	UFUNCTION(BlueprintCallable, Category = "AutoZone")
	AWaveSpawner* GetSpawner() const { return SpawnedSpawner; }

	UFUNCTION(BlueprintCallable, Category = "AutoZone")
	AQuestZone* GetZone() const { return SpawnedZone; }

protected:
	virtual void BeginPlay() override;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "AutoZone")
	TObjectPtr<UBillboardComponent> EditorIcon;

	TArray<TObjectPtr<UArrowComponent>> EditorArrows;

	virtual void OnConstruction(const FTransform& Transform) override;
#endif

private:
	void SpawnChildActors();
	TArray<FVector> GenerateSpawnPointLocations() const;
};
