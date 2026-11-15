// Retro Cute Journal end-of-take result modal: dim scrim, taped Paper card, burnt star remnants, objective rows, and [R] Retake.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/DCTypes.h"
#include "DCTakeResultWidget.generated.h"

class UBorder;
class UButton;
class UDCJournalStarWidget;
class UDCPersonaComponent;
class UDCTakeSubsystem;
class UHorizontalBox;
class UImage;
class UOverlay;
class UTextBlock;

/**
 * Centered journal result card displayed on TakeResult.
 * Preserves burnt cracked star remnants, objective outcomes (Fires / Civilian / Stars), and [R] Retake.
 */
UCLASS(BlueprintType, Blueprintable)
class DIRECTORCUTJAM_API UDCTakeResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UDCTakeResultWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "DirectorCut|UI")
	void BindToTakeSubsystem(UDCTakeSubsystem* InSubsystem);

	UFUNCTION(BlueprintCallable, Category = "DirectorCut|UI")
	void RefreshResult();

	UFUNCTION(BlueprintImplementableEvent, Category = "DirectorCut|UI")
	void BP_OnResultUpdated(EDCTakeEndReason EndReason, int32 EarnedStars, int32 MaxContainers);

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeaderTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TakeMetaText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> StarsGlyphRow;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StarsGlyphText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StarsSummaryText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EndReasonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OutcomeSummaryText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ReviewNoteBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ReviewNoteText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> PerfectStickerImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RestartButton;

private:
	void EnsureDefaultLayout();
	void UnbindSubsystem();
	UDCPersonaComponent* FindPlayerPersonaComponent() const;
	FString BuildReviewNoteText(int32 Earned, int32 MaxContainers, EDCCivilianState CivState, int32 RemainingFires) const;

	UFUNCTION()
	void HandleTakeFinished(EDCTakeEndReason EndReason, int32 FinalEarnedStars, int32 FinalMaxContainers);

	UFUNCTION()
	void HandleTakeReset();

	UFUNCTION()
	void HandleRestartClicked();

	TWeakObjectPtr<UDCTakeSubsystem> BoundSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> DimScrimBorder;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> ResultCardOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FiresRowLabelText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> FiresValuePillBorder;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FiresValueText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CivilianRowLabelText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> CivilianValuePillBorder;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CivilianValueText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StarsRowLabelText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> StarsValuePillBorder;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RetakeButtonText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDCJournalStarWidget>> StarWidgets;

	bool bPlayingPop = false;
	bool bHasPoppedForCurrentResult = false;
	float PopElapsed = 0.0f;
	float PopDuration = 0.30f;
	bool bBindingsWired = false;
	bool bDefaultLayoutBuilt = false;
};
