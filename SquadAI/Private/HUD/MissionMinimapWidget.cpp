// MissionMinimapWidget.cpp — NativePaint minimap (no SceneCapture)
// Draws zones as colored rectangles, player as a triangle, compass labels
#include "HUD/MissionMinimapWidget.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"
#include "Rendering/DrawElements.h"
#include "Fonts/SlateFontInfo.h"
#include "Brushes/SlateColorBrush.h"

// Reusable brushes (avoid allocating per frame)
static const FSlateColorBrush WhiteBrush(FLinearColor::White);

void UMissionMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Apply border art
	if (Image_Border && BorderTexture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(BorderTexture);
		Brush.Margin = BorderNineSlice;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Image_Border->SetBrush(Brush);
	}

	if (Image_Background)
	{
		Image_Background->SetColorAndOpacity(BackgroundColor);
	}
}

void UMissionMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Cache player position
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		CachedPlayerLoc = Player->GetActorLocation();
		CachedPlayerYaw = Player->GetControlRotation().Yaw;
	}
}

void UMissionMinimapWidget::SetActiveZone(FVector ZoneCenter, float ZoneRadius)
{
	bHasActiveZone = true;
	ActiveZoneCenter = ZoneCenter;
	ActiveZoneRadius = FMath::Max(ZoneRadius, 100.f);
}

void UMissionMinimapWidget::ClearActiveZone()
{
	bHasActiveZone = false;
}

// =====================================================================
//  NATIVEPAINT — the actual drawing (called every frame by Slate)
// =====================================================================
int32 UMissionMinimapWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
	const FVector2D Center = WidgetSize * 0.5f;

	// Draw background
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			&WhiteBrush,
			ESlateDrawEffect::None,
			BackgroundColor);
		LayerId++;
	}

	// Draw active zone (red rectangle)
	if (bHasActiveZone)
	{
		const FVector2D ZoneMinimapPos = WorldToMinimap(ActiveZoneCenter, AllottedGeometry);
		const float MapScale = WidgetSize.X / (ActiveZoneRadius * (1.f + ZoneFitPadding) * 2.f);
		const float ZonePixelRadius = ActiveZoneRadius * MapScale;

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(ZonePixelRadius * 2.f, ZonePixelRadius * 2.f),
				FSlateLayoutTransform(FVector2f(ZoneMinimapPos.X - ZonePixelRadius, ZoneMinimapPos.Y - ZonePixelRadius))),
			&WhiteBrush,
			ESlateDrawEffect::None,
			ActiveZoneColor);
		LayerId++;
	}

	// Draw player marker
	DrawPlayerMarker(const_cast<FSlateWindowElementList&>(OutDrawElements), LayerId, AllottedGeometry);
	LayerId++;

	// Draw compass
	DrawCompassMarkers(const_cast<FSlateWindowElementList&>(OutDrawElements), LayerId, AllottedGeometry);
	LayerId++;

	return LayerId;
}

// =====================================================================
//  WORLD TO MINIMAP — convert world XY to widget pixel coordinates
// =====================================================================
FVector2D UMissionMinimapWidget::WorldToMinimap(FVector WorldPos, const FGeometry& Geometry) const
{
	const FVector2D WidgetSize = Geometry.GetLocalSize();

	if (!bHasActiveZone)
	{
		return WidgetSize * 0.5f; // Center
	}

	// Map world position relative to zone center
	const float MapRadius = ActiveZoneRadius * (1.f + ZoneFitPadding);
	FVector2D Relative(
		(WorldPos.X - ActiveZoneCenter.X) / (MapRadius * 2.f),
		(WorldPos.Y - ActiveZoneCenter.Y) / (MapRadius * 2.f)
	);

	// Rotate if following player
	if (bRotateWithPlayer)
	{
		const float Rad = FMath::DegreesToRadians(-CachedPlayerYaw + 90.f);
		const float CosR = FMath::Cos(Rad);
		const float SinR = FMath::Sin(Rad);
		const float RX = Relative.X * CosR - Relative.Y * SinR;
		const float RY = Relative.X * SinR + Relative.Y * CosR;
		Relative = FVector2D(RX, RY);
	}

	// Map to widget space (0,0 = top-left, Size = bottom-right)
	return FVector2D(
		(Relative.X + 0.5f) * WidgetSize.X,
		(0.5f - Relative.Y) * WidgetSize.Y // Y is flipped
	);
}

// =====================================================================
//  PLAYER MARKER — green dot
// =====================================================================
void UMissionMinimapWidget::DrawPlayerMarker(
	FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FGeometry& Geometry) const
{
	const FVector2D PlayerPixel = WorldToMinimap(CachedPlayerLoc, Geometry);
	const float R = PlayerDotRadius;

	// Draw as a small filled box (Slate doesn't have built-in circle draw)
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		Geometry.ToPaintGeometry(
			FVector2f(R * 2.f, R * 2.f),
			FSlateLayoutTransform(FVector2f(PlayerPixel.X - R, PlayerPixel.Y - R))),
		&WhiteBrush,
		ESlateDrawEffect::None,
		PlayerColor);
}

// =====================================================================
//  COMPASS — N/E/S/W at the edges
// =====================================================================
void UMissionMinimapWidget::DrawCompassMarkers(
	FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FGeometry& Geometry) const
{
	// Draw compass labels at the edges — simplified version
	// For a full implementation, use FSlateFontInfo and FSlateDrawElement::MakeText
	// For now, the compass direction is shown in the CompassWidget (separate)
}
