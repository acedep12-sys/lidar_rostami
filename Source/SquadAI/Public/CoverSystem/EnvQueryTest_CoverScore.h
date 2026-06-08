// EnvQueryTest_CoverScore.h — EQS Test: scores cover points against a threat context
// Uses the same scoring algorithm as the async query path
#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvQueryTest_CoverScore.generated.h"

UCLASS(meta = (DisplayName = "Cover Score vs Threat (Squad AI)"))
class SQUADAI_API UEnvQueryTest_CoverScore : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UEnvQueryTest_CoverScore();

	UPROPERTY(EditDefaultsOnly, Category = "Test")
	TSubclassOf<UEnvQueryContext> ThreatContext;

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};
