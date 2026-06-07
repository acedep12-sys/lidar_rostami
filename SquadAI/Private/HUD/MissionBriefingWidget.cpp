// MissionBriefingWidget.cpp — Mission card with RTL, NineSlice, art slots
#include "HUD/MissionBriefingWidget.h"
#include "HUD/FarsiTextHelper.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/PanelWidget.h"

void UMissionBriefingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyArt();
	SetVisible(this, false); // Hidden until mission starts
}

void UMissionBriefingWidget::UpdateFromSnapshot(const FQuestHudSnapshot& S)
{
	// Show/hide based on mission active state
	if (S.bMissionActive && !bLastMissionActive)
	{
		SetVisible(this, true);
		OnMissionStarted(); // BP event for intro animation
	}
	bLastMissionActive = S.bMissionActive;

	if (!S.bMissionActive)
	{
		SetVisible(this, false);
		return;
	}

	const bool bRTL = S.bRightToLeft;

	// Title
	SetText(Text_Title, S.MissionTitle, TitleColor, bRTL);

	// Objective
	SetText(Text_Objective, S.ObjectiveText, BodyColor, bRTL);

	// Wave counter
	if (S.WavesCurrent >= 0 && S.WavesTotal > 0)
	{
		SetVisible(Panel_WaveInfo, true);
		SetText(Text_WaveCounter,
			bRTL ? UFarsiTextHelper::FormatWavePersian(S.WavesCurrent + 1, S.WavesTotal)
			     : FText::Format(NSLOCTEXT("HUD", "Wave", "Wave {0}/{1}"),
			       FText::AsNumber(S.WavesCurrent + 1), FText::AsNumber(S.WavesTotal)),
			CounterColor, bRTL);
	}
	else
	{
		SetVisible(Panel_WaveInfo, false);
	}

	// Kill counter
	if (S.EnemiesKilled > 0)
	{
		SetText(Text_KillCounter,
			bRTL ? UFarsiTextHelper::FormatKillsPersian(S.EnemiesKilled)
			     : FText::Format(NSLOCTEXT("HUD", "Kills", "Kills: {0}"), FText::AsNumber(S.EnemiesKilled)),
			CounterColor, bRTL);
	}

	// Remaining
	if (S.EnemiesRemaining >= 0)
	{
		SetText(Text_Remaining,
			bRTL ? UFarsiTextHelper::FormatRemainingPersian(S.EnemiesRemaining)
			     : FText::Format(NSLOCTEXT("HUD", "Remain", "{0} remaining"), FText::AsNumber(S.EnemiesRemaining)),
			CounterColor, bRTL);
	}

	// Timer
	if (S.TimerSeconds >= 0.f)
	{
		SetVisible(Panel_Timer, true);
		FString TimerStr = bRTL
			? UFarsiTextHelper::FormatTimerPersian(S.TimerSeconds)
			: FString::Printf(TEXT("%d:%02d"),
				FMath::FloorToInt(S.TimerSeconds / 60.f),
				FMath::FloorToInt(FMath::Fmod(S.TimerSeconds, 60.f)));
		SetText(Text_Timer, FText::FromString(TimerStr), TimerColor, bRTL);
	}
	else
	{
		SetVisible(Panel_Timer, false);
	}

	// Enemy progress bar
	if (Progress_Enemies && S.EnemiesRemaining >= 0)
	{
		const int32 Total = S.EnemiesKilled + S.EnemiesRemaining;
		const float Pct = (Total > 0) ? (float)S.EnemiesKilled / (float)Total : 0.f;
		Progress_Enemies->SetPercent(Pct);
		if (bRTL) Progress_Enemies->SetBarFillType(EProgressBarFillType::RightToLeft);
	}
}

void UMissionBriefingWidget::ApplyArt()
{
	if (Image_Border && BorderTexture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(BorderTexture);
		Brush.Margin = BorderMargin;
		Brush.DrawAs = ESlateBrushDrawType::Box; // NineSlice
		Image_Border->SetBrush(Brush);
		Image_Border->SetColorAndOpacity(BorderTint);
	}

	if (Image_Background)
	{
		Image_Background->SetColorAndOpacity(BackgroundColor);
	}
}

void UMissionBriefingWidget::SetText(UTextBlock* Block, const FText& Text, FLinearColor Color, bool bRTL)
{
	if (!Block) return;
	Block->SetText(Text);
	Block->SetColorAndOpacity(FSlateColor(Color));
	Block->SetJustification(bRTL ? ETextJustify::Right : ETextJustify::Left);

	if (bApplyFonts)
	{
		if (Block == Text_Title && TitleFont.HasValidFont())
			Block->SetFont(TitleFont);
		else if (BriefingFont.HasValidFont())
			Block->SetFont(BriefingFont);
	}
}

void UMissionBriefingWidget::SetVisible(UWidget* W, bool bShow)
{
	if (W) W->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
