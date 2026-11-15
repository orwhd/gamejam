// World-projected Retro Cute Journal [E] interact prompt with clockwise Sky water hold arc.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCInteractPromptWidget.generated.h"

class UBorder;
class UTextBlock;
class UWidget;

/**
 * Displays the world-projected [E] chubby keycap, verb pill, and clockwise Sky water progress ring
 * when holding [E] to extinguish fire spots.
 */
UCLASS(BlueprintType, Blueprintable)
class DIRECTORCUTJAM_API UDCInteractPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UDCInteractPromptWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "DirectorCut|UI")
	void SetPromptState(const FText& LabelText, float HoldProgress01, bool bIsHoldMode, bool bIsSpraying);

	UFUNCTION(BlueprintPure, Category = "DirectorCut|UI")
	float GetHoldProgress01() const { return CurrentHoldProgress; }

	UFUNCTION(BlueprintPure, Category = "DirectorCut|UI")
	bool IsHoldMode() const { return bHoldModeActive; }

	UFUNCTION(BlueprintPure, Category = "DirectorCut|UI")
	bool IsSpraying() const { return bSprayingActive; }

	UFUNCTION(BlueprintPure, Category = "DirectorCut|UI")
	FText GetPromptLabelText() const { return CurrentLabelText; }

	UFUNCTION(BlueprintImplementableEvent, Category = "DirectorCut|UI")
	void BP_OnPromptStateUpdated(const FText& LabelText, float HoldProgress01, bool bIsHoldMode, bool bIsSpraying);

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
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
	TObjectPtr<UBorder> KeycapBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KeycapText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> PromptPillBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptLabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> RingAnchorWidget;

private:
	void EnsureDefaultLayout();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> BounceContainer;

	FText CurrentLabelText;
	float CurrentHoldProgress = 0.0f;
	bool bHoldModeActive = false;
	bool bSprayingActive = false;
	float BounceTime = 0.0f;
	bool bDefaultLayoutBuilt = false;
};
