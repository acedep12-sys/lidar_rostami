// QuestZone.h — Trigger volume that self-registers with the QuestRegistry
// Supports box/sphere/capsule. Tracks actors inside via overlap events.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Quest/QuestTypes.h"
#include "QuestZone.generated.h"

class UBoxComponent;
class USphereComponent;

UENUM(BlueprintType)
enum class EZoneShape : uint8 { Box, Sphere };

UCLASS(BlueprintType, Blueprintable)
class SQUADAI_API AQuestZone : public AActor
{
	GENERATED_BODY()

public:
	AQuestZone();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	FQuestTag ZoneTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	EZoneShape Shape = EZoneShape::Box;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone",
		meta = (EditCondition = "Shape == EZoneShape::Box"))
	FVector BoxExtent = FVector(1500.f, 1500.f, 400.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone",
		meta = (EditCondition = "Shape == EZoneShape::Sphere"))
	float SphereRadius = 1500.f;

	// ---- API ----
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zone")
	bool IsPlayerInZone() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zone")
	int32 CountHostilesInZone() const;

	UFUNCTION(BlueprintCallable, Category = "Zone")
	TArray<AActor*> GetHostilesInZone() const;

	// Zone center and radius for minimap
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zone")
	FVector GetZoneCenter() const { return GetActorLocation(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Zone")
	float GetZoneRadius() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> BoxComp;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> SphereComp;

private:
	UPROPERTY() TArray<TWeakObjectPtr<AActor>> ActorsInZone;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* Overlapped, AActor* Other,
		UPrimitiveComponent* OtherComp, int32 OtherIdx, bool bSweep, const FHitResult& Hit);
	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* Overlapped, AActor* Other,
		UPrimitiveComponent* OtherComp, int32 OtherIdx);

	void ConfigureShape();
};
