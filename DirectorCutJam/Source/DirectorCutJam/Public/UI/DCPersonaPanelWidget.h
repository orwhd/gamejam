// Retro Cute Journal full-screen Pre-Take Persona sheet & 3-state (Fixed / Cuttable / Cut) interactive rows.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/DCPersonaData.h"
#include "DCPersonaPanelWidget.generated.h"

class APawn;
class UBorder;
class UButton;
class UDCJournalScissorsWidget;
class UDCPersonaComponent;
class UDCTakeSubsystem;
class UImage;
class UPanelWidget;
class UTextBlock;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCBlockCutRequestedDelegate, FName, CutBlockID);

/**
 * Single journal sentence block supporting Fixed, Cuttable (Highlighter + Blush hover + scissors),
 * and Cut (muted original text preserved + thick Ink marker strikethrough) states.
 */
UCLASS(BlueprintType, Blueprintable)
class DIRECTORCUTJAM_API UDCBlockCutButton : public UUserWidget
{
	GENERATED_BODY()

public:
	UDCBlockCutButton(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "DirectorCut|UI")
	void SetupBlockButton(
		const FDCPersonaBlock& InBlock,
		bool bCanEditNow,
		UDCPersonaComponent* InOwnerComponent,
		UTexture2D* InScissorsTexture = nullptr,
		float InCutAnimSeconds = 0.24f,
		bool bPlayJustCutStrikeAnim = false);

	UFUNCTION(BlueprintPure, Category = "DirectorCut|UI")
	FName GetBlockID() const { return CachedBlock.BlockID; }

	UPROPERTY(BlueprintAssignable, Category = "DirectorCut|UI")
	FDCBlockCutRequestedDelegate OnBlockCutTriggered;

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CutButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ButtonCaptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BlockHighlightBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDCJournalScissorsWidget> HoverScissorsIcon;

private:
	void EnsureDefaultLayout();
	void ApplyVisualState();

	UFUNCTION()
	void HandleButtonClicked();

	UFUNCTION()
	void HandleButtonHovered();

	UFUNCTION()
	void HandleButtonUnhovered();

	FDCPersonaBlock CachedBlock;
	TWeakObjectPtr<UDCPersonaComponent> OwnerComponent;
	bool bEditableNow = false;
	bool bIsHovered = false;
	bool bStrikeAnimPlaying = false;
	float StrikeProgress = 1.0f;
	float CutAnimDuration = 0.24f;
	bool bBindingsWired = false;
	bool bDefaultLayoutBuilt = false;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> CuttableUnderlineWidget;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> CutStrikeBarContainer;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> StrikeThroughBar;
};

/**
 * Full-screen Pre-Take Persona journal sheet over Desk backdrop (hiding 3D room),
 * with 4 angled corner tapes, 3-state sentence blocks, footer legend, and chubby ACTION! button.
 */
UCLASS(BlueprintType, Blueprintable)
class DIRECTORCUTJAM_API UDCPersonaPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UDCPersonaPanelWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "DirectorCut|UI")
	void BindToPlayer(APawn* PlayerPawn);

	UFUNCTION(BlueprintCallable, Category = "DirectorCut|UI")
	void RefreshPanel();

	UFUNCTION(BlueprintCallable, Category = "DirectorCut|UI")
	void PlayPaperDropIn();

	UFUNCTION(BlueprintImplementableEvent, Category = "DirectorCut|UI")
	void BP_OnPersonaPanelRefreshed(bool bCanEditNow, int32 PresentCount, int32 TotalCount);

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditDefaultsOnly, Category = "DirectorCut|UI")
	TSubclassOf<UDCBlockCutButton> BlockRowClass;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BackdropBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> JournalSheetBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RoleTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> PhaseBadgeBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PhaseBadgeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> DirectorsCutBadgeBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DirectorsCutBadgeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDCJournalScissorsWidget> DirectorsCutScissorsIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ReadOnlyBadgeBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ReadOnlyBadgeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> AngelDogStickerImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> BlocksContainer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> StartTakeButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StartTakeButtonLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RestartTakeButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FooterRuleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LegendFixedLabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LegendCuttableLabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LegendCutLabelText;

private:
	void EnsureDefaultLayout();
	void UnbindDelegates();
	void UpdateSheetTransform();

	UFUNCTION()
	void HandlePersonaChanged();

	UFUNCTION()
	void HandleBlockCutTriggered(FName CutBlockID);

	UFUNCTION()
	void HandleTakePhaseChanged(EDCTakePhase NewPhase);

	UFUNCTION()
	void HandleStartTakeClicked();

	UFUNCTION()
	void HandleRestartTakeClicked();

	TWeakObjectPtr<APawn> BoundPawn;
	TWeakObjectPtr<UDCPersonaComponent> PersonaComponent;
	TWeakObjectPtr<UDCTakeSubsystem> TakeSubsystem;

	TSet<FName> PreviouslyPresentBlockIDs;
	FName PendingJustCutBlockID = NAME_None;
	bool bPlayingPaperDropIn = false;
	float PaperDropInElapsed = 0.0f;
	bool bPlayingSheetShake = false;
	float SheetShakeElapsed = 0.0f;
	bool bBindingsWired = false;
	bool bDefaultLayoutBuilt = false;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ResetButtonLipContainer;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResetTakeButtonLabel;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ActionButtonLipContainer;
};
