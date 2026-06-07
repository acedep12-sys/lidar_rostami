// QuestHUDWidget.h — Master HUD container: mission briefing + minimap + compass
// Auto-binds to QuestTickerSubsystem. Subclass in Widget Blueprint.
// All child widgets are optional — only create what you want.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Quest/QuestTypes.h"
#include "QuestHUDWidget.generated.h"

class UMissionBriefingWidget;
class UMissionMinimapWidget;
class UCompassWidget;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class SQUADAI_API UQuestHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ---- Sub-widgets (placed in your Designer layout, bound by name) ----

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMissionBriefingWidget> Widget_Briefing;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UMissionMinimapWidget> Widget_Minimap;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCompassWidget> Widget_Compass;

	// ---- Announcement (center of screen) ----

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Announcement;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MissionStatus;

	// ---- Config ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	float AnnouncementDuration = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	float StatusDuration = 4.f;

	// ---- API ----

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowAnnouncement(const FText& Text, FLinearColor Color = FLinearColor::White);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void OnHudUpdated(const FQuestHudSnapshot& Snapshot);

	float AnnouncementTimer = 0.f;
	float StatusTimer = 0.f;
	FQuestHudSnapshot LastSnapshot;

	void SetVisible(UWidget* W, bool bShow);
};
