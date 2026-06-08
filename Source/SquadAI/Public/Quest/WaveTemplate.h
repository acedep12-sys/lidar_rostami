// WaveTemplate.h — Reusable DataAsset: define waves once, use everywhere
// Edit once in Content Browser → all quests using it update automatically
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SquadTypes.h"
#include "WaveTemplate.generated.h"

UCLASS(BlueprintType)
class SQUADAI_API UWaveTemplate : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waves")
	TArray<FWaveDefinition> Waves;

	// Default enemy class (can be overridden per wave)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waves")
	TSubclassOf<APawn> DefaultEnemyClass;

	int32 GetTotalEnemyCount() const
	{
		int32 Total = 0;
		for (const FWaveDefinition& W : Waves) Total += W.NumSoldiers;
		return Total;
	}
};
