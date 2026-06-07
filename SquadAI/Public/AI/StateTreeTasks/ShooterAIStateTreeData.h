// ShooterAIStateTreeData.h — StateTree evaluators that populate runtime data
// Three evaluators: Threat, Suppression, Health
// IMPORTANT: Instance data structs must be declared BEFORE their evaluators
#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "SquadTypes.h"
#include "ShooterAIStateTreeData.generated.h"

// =====================================================================
//  INSTANCE DATA (must be BEFORE the evaluator that references it)
// =====================================================================

USTRUCT()
struct FSTE_ThreatInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Output")
	TWeakObjectPtr<AActor> PrimaryThreat = nullptr;

	UPROPERTY(EditAnywhere, Category = "Output")
	float ThreatConfidence = 0.f;

	UPROPERTY(EditAnywhere, Category = "Output")
	bool bHasVisibleEnemy = false;

	UPROPERTY(EditAnywhere, Category = "Output")
	FVector LastKnownEnemyLoc = FVector::ZeroVector;
};

USTRUCT()
struct FSTE_SuppressionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Output")
	float Suppression = 0.f;
};

USTRUCT()
struct FSTE_HealthInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Output")
	float HealthPercent = 1.f;
};

// =====================================================================
//  EVALUATORS (reference the instance data types above)
// =====================================================================

USTRUCT(meta = (DisplayName = "Evaluate Threat"))
struct SQUADAI_API FSTE_Threat : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTE_ThreatInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT(meta = (DisplayName = "Evaluate Suppression"))
struct SQUADAI_API FSTE_Suppression : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTE_SuppressionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT(meta = (DisplayName = "Evaluate Health"))
struct SQUADAI_API FSTE_Health : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTE_HealthInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
