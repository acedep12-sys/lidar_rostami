// AimComponent.h — Natural aim rotation with critically damped spring
// Body yaw only rotates past spine cone. Spine uses semi-implicit Euler.
// The single most important "feels right" piece.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AimComponent.generated.h"

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent, DisplayName = "Aim Component"))
class SQUADAI_API UAimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAimComponent();

	// ---- Tuning (defaults from USquadAITuning) ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|Body")
	float BodyYawRate = 360.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|Spine")
	float SpringStiffness = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|Spine")
	float SpringDampingRatio = 1.f;   // 1.0 = critically damped (no overshoot)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|Spine")
	float MaxAimOffsetYaw = 75.f;     // Spine cone before body must rotate

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|Spine")
	float MaxAimOffsetPitch = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|Accuracy")
	float MaxAimErrorDegrees = 2.5f;  // Random error on final aim direction

	// ---- API ----

	// Set the world-space target to aim at
	UFUNCTION(BlueprintCallable, Category = "Aim")
	void SetAimTarget(FVector WorldTarget);

	// Clear aim target (return to forward)
	UFUNCTION(BlueprintCallable, Category = "Aim")
	void ClearAimTarget();

	// Get the current aimed direction (with spring + error applied)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Aim")
	FRotator GetAimedRotation() const;

	// Get the spine-only aim offset (for AnimBP aim offset node)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Aim")
	FRotator GetSpineAimOffset() const;

	// Is the aim within error tolerance of the target?
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Aim")
	bool IsAimedAtTarget(float ToleranceDegrees = 5.f) const;

	UPROPERTY(BlueprintReadOnly, Category = "Aim")
	bool bHasTarget = false;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	FVector AimTarget = FVector::ZeroVector;
	FVector DesiredAimDir = FVector::ForwardVector;

	UPROPERTY()
	TWeakObjectPtr<class ASoldierAIController> CachedAIController;

	// Spring state (semi-implicit Euler)
	FVector SpringPos = FVector::ForwardVector;   // Current spine aim direction
	FVector SpringVel = FVector::ZeroVector;       // Angular velocity

	// Cached body yaw (the character's actual rotation)
	float CurrentBodyYaw = 0.f;

	void UpdateBodyYaw(float DeltaTime);
	void UpdateSpineSpring(float DeltaTime);
};
