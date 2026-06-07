// MissionBriefingWidget.h — Top-left mission card with NineSlice border + RTL support
// Subclass this in a Widget Blueprint. Name your widgets to auto-bind.
// Art slots in Details panel — drag your border texture, set your Persian font.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Quest/QuestTypes.h"
#include "MissionBriefingWidget.generated.h"

class UTextBlock;
class UImage;
class UProgressBar;
class UPanelWidget;

UCLASS(BlueprintType, Blueprintable)
class SQUADAI_API UMissionBriefingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ---- Art Slots ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Briefing|Art")
	TObjectPtr<UTexture2D> BorderTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Briefing|Art")
	FMargin BorderMargin = FMargin(0.25f); // NineSlice corners

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Briefing|Art")
	FLinearColor BorderTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Briefing|Art")
	FLinearColor BackgroundColor = FLinearColor(0.02f, 0.02f, 0.05f, 0.85f);

	// ---- Text Style ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Briefing|Text")
	FSlateFontInfo BriefingFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Briefing|Text")
	FSlateFontInfo TitleFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Briefing|Text")
	FLinearColor TitleColor = FLinearColor(1.f, 0.85f, 0.3f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Briefing|Text")
	FLinearColor SubtitleColor = FLinearColor(0.8f, 0.8f, 0.7f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Briefing|Text")
	FLinearColor BodyColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Briefing|Text")
	FLinearColor CounterColor = FLinearColor(0.9f, 0.9f, 0.4f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Briefing|Text")
	FLinearColor TimerColor = FLinearColor(0.8f, 0.95f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Briefing|Text")
	bool bApplyFonts = false; // Apply BriefingFont/TitleFont to all text blocks

	// ---- Bound Widgets (all optional) ----

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_Border;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_Background;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Title;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Subtitle;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Briefing;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Objective;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_WaveCounter;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_KillCounter;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Remaining;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Timer;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> Progress_Enemies;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> Panel_WaveInfo;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> Panel_Timer;

	// ---- API ----

	UFUNCTION(BlueprintCallable, Category = "Briefing")
	void UpdateFromSnapshot(const FQuestHudSnapshot& Snapshot);

	// BlueprintImplementable events for intro/completion animations
	UFUNCTION(BlueprintImplementableEvent, Category = "Briefing")
	void OnMissionStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Briefing")
	void OnMissionCompleted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Briefing")
	void OnMissionFailed();

protected:
	virtual void NativeConstruct() override;

private:
	void ApplyArt();
	void SetText(UTextBlock* Block, const FText& Text, FLinearColor Color, bool bRTL = false);
	void SetVisible(UWidget* W, bool bShow);
	bool bLastMissionActive = false;
};
