// QuestHUDWidget.cpp — Master HUD, auto-binds to QuestTickerSubsystem
#include "HUD/QuestHUDWidget.h"
#include "HUD/MissionBriefingWidget.h"
#include "HUD/MissionMinimapWidget.h"
#include "HUD/CompassWidget.h"
#include "Quest/QuestTickerSubsystem.h"
#include "Quest/QuestTypes.h"
#include "Components/TextBlock.h"

void UQuestHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Auto-bind to quest ticker
	UWorld* World = GetWorld();
	if (World)
	{
		UQuestTickerSubsystem* Ticker = World->GetSubsystem<UQuestTickerSubsystem>();
		if (Ticker)
		{
			Ticker->OnHudUpdated.AddDynamic(this, &UQuestHUDWidget::OnHudUpdated);
		}
	}

	SetVisible(Text_Announcement.Get(), false);
	SetVisible(Text_MissionStatus.Get(), false);
}

void UQuestHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Announcement fade
	if (AnnouncementTimer > 0.f)
	{
		AnnouncementTimer -= InDeltaTime;
		if (AnnouncementTimer <= 0.f) SetVisible(Text_Announcement.Get(), false);
	}

	// Status fade
	if (StatusTimer > 0.f)
	{
		StatusTimer -= InDeltaTime;
		if (StatusTimer <= 0.f) SetVisible(Text_MissionStatus.Get(), false);
	}
}

void UQuestHUDWidget::OnHudUpdated(const FQuestHudSnapshot& Snapshot)
{
	LastSnapshot = Snapshot;

	// Update briefing card
	if (Widget_Briefing)
	{
		Widget_Briefing->UpdateFromSnapshot(Snapshot);
	}

	// Update minimap zone highlight
	if (Widget_Minimap && Snapshot.bMissionActive)
	{
		// Zone info comes from the active objective's zone
		// This would be extended to read from the QuestMission → current objective → zone
		// For now, the minimap can be set manually via Blueprint or
		// the QuestTickerSubsystem can broadcast zone info
	}

	// Update compass objective marker
	if (Widget_Compass && Snapshot.bMissionActive)
	{
		// Similarly, objective location comes from the quest system
		// Extension point: add zone center to the snapshot
	}
}

void UQuestHUDWidget::ShowAnnouncement(const FText& Text, FLinearColor Color)
{
	if (Text_Announcement.Get())
	{
		Text_Announcement->SetText(Text);
		Text_Announcement->SetColorAndOpacity(FSlateColor(Color));
		SetVisible(Text_Announcement.Get(), true);
		AnnouncementTimer = AnnouncementDuration;
	}
}

void UQuestHUDWidget::SetVisible(UWidget* W, bool bShow)
{
	if (W) W->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
