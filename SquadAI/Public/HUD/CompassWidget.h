// CompassWidget.h — Horizontal compass bar with cardinal directions + objective marker
// Player yaw drives UV scroll. Quest marker shows direction to active zone.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CompassWidget.generated.h"

class UImage;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class SQUADAI_API UCompassWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ---- Art ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compass|Art")
	TObjectPtr<UTexture2D> CompassStripTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compass|Art")
	TObjectPtr<UTexture2D> CenterMarkerTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compass|Art")
	TObjectPtr<UTexture2D> ObjectiveMarkerTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compass|Art")
	FLinearColor ObjectiveMarkerColor = FLinearColor(1.f, 0.8f, 0.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compass|Art")
	FLinearColor CardinalTextColor = FLinearColor::White;

	// ---- Bound Widgets (all optional) ----

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_Strip;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_Center;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_Objective;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Cardinal;

	// ---- API ----

	UFUNCTION(BlueprintCallable, Category = "Compass")
	void SetObjectiveLocation(FVector WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Compass")
	void ClearObjective();

	// Use Farsi cardinal directions?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compass")
	bool bUseFarsiCardinals = true;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	bool bHasObjective = false;
	FVector ObjectiveWorldLoc = FVector::ZeroVector;

	void ApplyArt();
};
