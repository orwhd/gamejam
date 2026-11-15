// Retro Cute Journal thought bubble: Paper sheet, Peach outline, 2-line clue formatting, and dot tail.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCThoughtBubbleWidget.generated.h"

class UBorder;
class USizeBox;
class UTextBlock;

/**
 * Floating above-head journal thought bubble with Peach border, 3-dot tail,
 * two-line memory/clue typography, and quick overshoot bounce-in.
 */
UCLASS(BlueprintType, Blueprintable)
class DIRECTORCUTJAM_API UDCThoughtBubbleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UDCThoughtBubbleWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "DirectorCut|UI")
	void SetThoughtText(const FText& NewThought);

	UFUNCTION(BlueprintPure, Category = "DirectorCut|UI")
	FText GetThoughtText() const { return CachedThoughtText; }

	UFUNCTION(BlueprintImplementableEvent, Category = "DirectorCut|UI")
	void BP_OnThoughtUpdated(const FText& NewThought);

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
	TObjectPtr<UBorder> BubbleBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ThoughtTextBlock;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SecondaryThoughtTextBlock;

private:
	void EnsureDefaultLayout();
	void ApplyPopScale(float Scale);

	UPROPERTY(Transient)
	TObjectPtr<UBorder> AnimRootBorder;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> TailSpacerBox;

	FText CachedThoughtText;
	float PopElapsed = 0.24f;
	float PopDuration = 0.24f;
	bool bPlayingPop = false;
	bool bDefaultLayoutBuilt = false;
};
