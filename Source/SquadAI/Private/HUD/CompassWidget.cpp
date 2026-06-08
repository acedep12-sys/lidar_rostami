// CompassWidget.cpp — Horizontal compass with objective marker
#include "HUD/CompassWidget.h"
#include "HUD/FarsiTextHelper.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UCompassWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyArt();
}

void UCompassWidget::ApplyArt()
{
	if (Image_Strip && CompassStripTexture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(CompassStripTexture);
		Brush.ImageSize = FVector2D(CompassStripTexture->GetSizeX(), CompassStripTexture->GetSizeY());
		Image_Strip->SetBrush(Brush);
	}

	if (Image_Center && CenterMarkerTexture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(CenterMarkerTexture);
		Image_Center->SetBrush(Brush);
	}

	if (Image_Objective && ObjectiveMarkerTexture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(ObjectiveMarkerTexture);
		Image_Objective->SetBrush(Brush);
		Image_Objective->SetColorAndOpacity(ObjectiveMarkerColor);
	}
}

void UCompassWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player) return;

	const float PlayerYaw = Player->GetControlRotation().Yaw;

	// Scroll the compass strip based on player yaw
	if (Image_Strip)
	{
		// Assuming compass texture is 2048px wide showing 360° of N-E-S-W-N
		// UV scroll = yaw / 360 mapped to pixel offset
		const float NormYaw = FMath::Fmod(PlayerYaw + 360.f, 360.f) / 360.f;
		const float StripWidth = 2048.f; // Match your compass texture width
		const float VisibleWidth = MyGeometry.GetLocalSize().X;
		const float Offset = NormYaw * StripWidth - (VisibleWidth * 0.5f);
		Image_Strip->SetRenderTranslation(FVector2D(-Offset, 0.f));
	}

	// Cardinal direction text
	if (Text_Cardinal)
	{
		FString Cardinal = bUseFarsiCardinals
			? UFarsiTextHelper::GetPersianCardinal(PlayerYaw)
			: [PlayerYaw]() -> FString {
				float Y = FMath::Fmod(PlayerYaw + 360.f, 360.f);
				if (Y >= 337.5f || Y < 22.5f) return TEXT("N");
				if (Y < 67.5f) return TEXT("NE");
				if (Y < 112.5f) return TEXT("E");
				if (Y < 157.5f) return TEXT("SE");
				if (Y < 202.5f) return TEXT("S");
				if (Y < 247.5f) return TEXT("SW");
				if (Y < 292.5f) return TEXT("W");
				return TEXT("NW");
			}();

		Text_Cardinal->SetText(FText::FromString(Cardinal));
		Text_Cardinal->SetColorAndOpacity(FSlateColor(CardinalTextColor));
	}

	// Objective marker
	if (Image_Objective)
	{
		if (bHasObjective && Player)
		{
			const FVector ToObj = ObjectiveWorldLoc - Player->GetActorLocation();
			const float AngleToObj = FMath::Atan2(ToObj.Y, ToObj.X) * (180.f / PI);
			const float RelAngle = FMath::FindDeltaAngleDegrees(PlayerYaw, AngleToObj);

			// Map ±90° to screen pixels on the compass bar
			const float VisibleWidth = MyGeometry.GetLocalSize().X;
			const float ScreenX = (RelAngle / 90.f) * (VisibleWidth * 0.5f);

			Image_Objective->SetRenderTranslation(FVector2D(ScreenX, 0.f));
			Image_Objective->SetVisibility(FMath::Abs(RelAngle) <= 90.f
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
		}
		else
		{
			Image_Objective->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UCompassWidget::SetObjectiveLocation(FVector WorldLocation)
{
	bHasObjective = true;
	ObjectiveWorldLoc = WorldLocation;
}

void UCompassWidget::ClearObjective()
{
	bHasObjective = false;
	if (Image_Objective)
	{
		Image_Objective->SetVisibility(ESlateVisibility::Collapsed);
	}
}
