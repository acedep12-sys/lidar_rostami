// MissionMinimapWidget.h — Minimap drawn with NativePaint (zero SceneCapture cost)
// Paints: zone as red rectangle, player as green triangle, compass markers
// ~0.05ms/frame — a handful of FSlateDrawElement quads, no extra render pass
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MissionMinimapWidget.generated.h"

class UImage;

UCLASS(BlueprintType, Blueprintable)
class SQUADAI_API UMissionMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ---- Style ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Style")
	FLinearColor ActiveZoneColor = FLinearColor(1.f, 0.f, 0.f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Style")
	FLinearColor InactiveZoneColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Style")
	FLinearColor PlayerColor = FLinearColor(0.2f, 1.f, 0.2f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Style")
	FLinearColor CompassColor = FLinearColor(1.f, 0.95f, 0.7f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Style")
	FLinearColor BackgroundColor = FLinearColor(0.05f, 0.05f, 0.08f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Style")
	float PlayerDotRadius = 5.f;

	// ---- Behavior ----

	// If false: zone fills the minimap, player dot moves.
	// If true: player stays centered, world scrolls under.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Behavior")
	bool bFollowPlayer = false;

	// Padding around the zone when fitting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Behavior")
	float ZoneFitPadding = 0.15f;

	// Rotate with player facing?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Behavior")
	bool bRotateWithPlayer = true;

	// ---- Art (optional) ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Art")
	TObjectPtr<UTexture2D> BackgroundTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Art")
	TObjectPtr<UTexture2D> BorderTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Art")
	FMargin BorderNineSlice = FMargin(0.25f);

	// ---- Bound widgets (optional) ----
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_Border;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_Background;

	// ---- API ----

	// Set the active quest zone to highlight
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetActiveZone(FVector ZoneCenter, float ZoneRadius);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void ClearActiveZone();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	bool bHasActiveZone = false;
	FVector ActiveZoneCenter = FVector::ZeroVector;
	float ActiveZoneRadius = 0.f;

	// Cached player state
	FVector CachedPlayerLoc = FVector::ZeroVector;
	float CachedPlayerYaw = 0.f;

	// Map world position to minimap widget pixel coordinates
	FVector2D WorldToMinimap(FVector WorldPos, const FGeometry& Geometry) const;

	// Draw a filled rectangle on the minimap
	void DrawFilledRect(FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FGeometry& Geometry, FVector2D TopLeft, FVector2D Size, FLinearColor Color) const;

	// Draw the player triangle
	void DrawPlayerMarker(FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FGeometry& Geometry) const;

	// Draw compass N/E/S/W labels
	void DrawCompassMarkers(FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FGeometry& Geometry) const;
};
