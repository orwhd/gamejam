// Retro Cute Journal top-center transient 3-star popup pill (Empty / Filled / Burnt with crack & smoke).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/DCTypes.h"
#include "DCStarPopupWidget.generated.h"

class UBorder;
class UDCJournalStarWidget;
class UDCTakeSubsystem;
class UHorizontalBox;
class UTextBlock;

/**
 * Top-center compact journal pill that pops in for ~2s when EarnedStars increase
 * or MaxStarContainers drop, displaying chubby vector stars (Filled / Empty / Burnt).
 */
UCLASS(BlueprintType, Blueprintable)
class DIRECTORCUTJAM_API UDCStarPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UDCStarPopupWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "DirectorCut|UI")
	void BindToTakeSubsystem(UDCTakeSubsystem* InSubsystem);

	UFUNCTION(BlueprintCallable, Category = "DirectorCut|UI")
	void TriggerPopup(int32 EarnedStars, int32 MaxContainers, EDCStarChangeReason Reason, float DurationSeconds = 2.0f);

	UFUNCTION(BlueprintCallable, Category = "DirectorCut|UI")
	void DismissPopup();

	UFUNCTION(BlueprintPure, Category = "DirectorCut|UI")
	bool IsPopupVisible() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "DirectorCut|UI")
	void BP_OnStarPopupTriggered(int32 EarnedStars, int32 MaxContainers, EDCStarChangeReason Reason);

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> PopupAccentBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> StarsGlyphRow;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StarsGlyphText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EventCaptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ContainerStatusText;

private:
	void EnsureDefaultLayout();
	void UnbindSubsystem();
	void RebuildStarIcons(int32 EarnedStars, int32 MaxContainers, EDCStarChangeReason Reason);

	UFUNCTION()
	void HandleStarStateChanged(int32 EarnedStars, int32 MaxContainers, EDCStarChangeReason Reason);

	UFUNCTION()
	void HandleTakePhaseChanged(EDCTakePhase NewPhase);

	UFUNCTION()
	void HandleTakeReset();

	TWeakObjectPtr<UDCTakeSubsystem> BoundSubsystem;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDCJournalStarWidget>> StarWidgets;

	int32 CachedEarnedStars = 0;
	int32 CachedMaxContainers = 3;
	int32 NewlyBurntIndex = INDEX_NONE;
	int32 NewlyFilledIndex = INDEX_NONE;
	float DisplayDurationSeconds = 2.0f;
	float ElapsedDisplaySeconds = 0.0f;
	float RemainingDisplaySeconds = 0.0f;
	float PopAnimSeconds = 0.35f;
	bool bDefaultLayoutBuilt = false;
};
