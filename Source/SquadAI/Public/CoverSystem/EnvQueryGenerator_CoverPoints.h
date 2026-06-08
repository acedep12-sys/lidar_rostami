// EnvQueryGenerator_CoverPoints.h — EQS Generator: produces cover points as query items
// Optional visual workflow — AI can also use RequestCoverAsync() directly.
#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "EnvQueryGenerator_CoverPoints.generated.h"

UCLASS(meta = (DisplayName = "Cover Points (Squad AI)"))
class SQUADAI_API UEnvQueryGenerator_CoverPoints : public UEnvQueryGenerator
{
	GENERATED_BODY()

public:
	UEnvQueryGenerator_CoverPoints();

	UPROPERTY(EditDefaultsOnly, Category = "Generator")
	float SearchRadius = 2000.f;

	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};
