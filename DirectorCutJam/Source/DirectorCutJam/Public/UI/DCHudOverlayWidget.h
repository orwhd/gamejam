// Retro Cute Journal HUD orchestrator: full-screen PreTake Persona sheet, minimal ActiveTake [R] Retake pill + transient StarPopup, TakeResult modal.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/DCTypes.h"
#include "DCHudOverlayWidget.generated.h"

class APawn;
class UBorder;
class UButton;
class UDCActionStateComponent;
class UDCPersonaPanelWidget;
class UDCStarPopupWidget;
class UDCTakeResultWidget;
class UDCTakeSubsystem;
class UOverlay;
class UTextBlock;

/**
 * Root HUD overlay managing PreTake (opaque Desk + Persona sheet), ActiveTake (minimal top-right [R] Retake pill
 * and transient top-center StarPopup), and TakeResult modal. Zero movie-studio clapperboard elements.
 */
UCLASS(BlueprintType, Blueprintable)
class DIRECTORCUTJAM_API UDCHudOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UDCHudOverlayWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "DirectorCut|UI")
	void InitializeForPlayer(APawn* PlayerPawn);

	UFUNCTION(BlueprintCallable, Category = "DirectorCut|UI")
	void RefreshOverlay();

	UFUNCTION(BlueprintPure, Category = "DirectorCut|UI")
	UDCPersonaPanelWidget* GetPersonaPanel() const { return PersonaPanelWidget.Get(); }

	UFUNCTION(BlueprintPure, Category = "DirectorCut|UI")
	UDCStarPopupWidget* GetStarPopup() const { return StarPopupWidget.Get(); }

	UFUNCTION(BlueprintPure, Category = "DirectorCut|UI")
	UDCTakeResultWidget* GetResultWidget() const { return ResultWidget.Get(); }

	UFUNCTION(BlueprintImplementableEvent, Category = "DirectorCut|UI")
	void BP_OnOverlayRefreshed(EDCTakePhase NewPhase, int32 EarnedStars, int32 MaxContainers);

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDCPersonaPanelWidget> PersonaPanelWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDCStarPopupWidget> StarPopupWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UDCTakeResultWidget> ResultWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RetakeHintContainer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> TabPeekHintContainer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> TabPeekHintButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TabPeekHintLabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RetakeHintButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RetakeHintLabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> HintToastBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HintToastText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> PageFlipGhostBorder;

private:
	void EnsureDefaultLayout();
	void ApplyConfigWidgetOverrides();
	void UnbindDelegates();
	void StartPageFlipGhost();
	void StopPageFlipGhost();

	UFUNCTION()
	void HandleTakePhaseChanged(EDCTakePhase NewPhase);

	UFUNCTION()
	void HandleTakeReset();

	UFUNCTION()
	void HandlePersonaPanelToggled(bool bIsOpen);

	UFUNCTION()
	void HandleHudHintRequested(FText HintText, float DurationSeconds);

	UFUNCTION()
	void HandleRetakeHintClicked();

	UFUNCTION()
	void HandleTabPeekHintClicked();

	TWeakObjectPtr<APawn> BoundPawn;
	TWeakObjectPtr<UDCActionStateComponent> ActionStateComponent;
	TWeakObjectPtr<UDCTakeSubsystem> TakeSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> RootOverlay;

	EDCTakePhase LastKnownPhase = EDCTakePhase::PreTake;
	float HintRemainingSeconds = 0.0f;
	bool bPlayingPageFlipGhost = false;
	float PageFlipGhostElapsed = 0.0f;
	bool bBindingsWired = false;
	bool bDefaultLayoutBuilt = false;
};
