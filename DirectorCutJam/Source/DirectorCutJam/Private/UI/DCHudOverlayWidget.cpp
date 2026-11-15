#include "UI/DCHudOverlayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Core/DCTakeSubsystem.h"
#include "Data/DCLevelConfigData.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Interaction/DCActionStateComponent.h"
#include "UI/DCPersonaPanelWidget.h"
#include "UI/DCStarPopupWidget.h"
#include "UI/DCTakeResultWidget.h"
#include "UI/DCWidgetTheme.h"

UDCHudOverlayWidget::UDCHudOverlayWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDCHudOverlayWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}
}

TSharedRef<SWidget> UDCHudOverlayWidget::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}
	return Super::RebuildWidget();
}

void UDCHudOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!bBindingsWired)
	{
		bBindingsWired = true;
		if (RetakeHintButton)
		{
			RetakeHintButton->OnClicked.AddDynamic(this, &UDCHudOverlayWidget::HandleRetakeHintClicked);
		}
		if (TabPeekHintButton)
		{
			TabPeekHintButton->OnClicked.AddDynamic(this, &UDCHudOverlayWidget::HandleTabPeekHintClicked);
		}
	}

	RefreshOverlay();
}

void UDCHudOverlayWidget::NativeDestruct()
{
	UnbindDelegates();
	if (bBindingsWired)
	{
		if (RetakeHintButton)
		{
			RetakeHintButton->OnClicked.RemoveDynamic(this, &UDCHudOverlayWidget::HandleRetakeHintClicked);
		}
		if (TabPeekHintButton)
		{
			TabPeekHintButton->OnClicked.RemoveDynamic(this, &UDCHudOverlayWidget::HandleTabPeekHintClicked);
		}
		bBindingsWired = false;
	}
	Super::NativeDestruct();
}

void UDCHudOverlayWidget::EnsureDefaultLayout()
{
	if (bDefaultLayoutBuilt || !WidgetTree)
	{
		return;
	}
	bDefaultLayoutBuilt = true;

	RootOverlay = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = RootOverlay;

	// Layer 1 (ActiveTake ONLY): Top-right compact low-opacity [Tab] Persona + [R] Retake pill cluster
	UHorizontalBox* TopRightHintRow = WidgetTree->ConstructWidget<UHorizontalBox>();

	UTextBlock* BuiltTabPeekLabel = nullptr;
	TabPeekHintButton = DCTheme::MakeKeycapPillButton(
		WidgetTree,
		TEXT("Tab"),
		TEXT("Persona"),
		DCTheme::Paper(),
		DCTheme::Blush(),
		3.0f,
		26.0f,
		13,
		16,
		FMargin(14.0f, 7.0f),
		&BuiltTabPeekLabel);
	TabPeekHintLabelText = BuiltTabPeekLabel;

	TabPeekHintContainer = WidgetTree->ConstructWidget<UBorder>();
	TabPeekHintContainer->SetBrushColor(FLinearColor::Transparent);
	TabPeekHintContainer->SetContent(TabPeekHintButton);
	TabPeekHintContainer->SetVisibility(ESlateVisibility::Collapsed);
	DCTheme::AddHBoxChild(TopRightHintRow, TabPeekHintContainer, FMargin(0.0f, 0.0f, 10.0f, 0.0f), HAlign_Left, VAlign_Center);

	UTextBlock* BuiltRetakeHintLabel = nullptr;
	RetakeHintButton = DCTheme::MakeKeycapPillButton(
		WidgetTree,
		TEXT("R"),
		TEXT("Retake"),
		DCTheme::Paper(),
		DCTheme::Blush(),
		3.0f,
		26.0f,
		14,
		16,
		FMargin(14.0f, 7.0f),
		&BuiltRetakeHintLabel);
	RetakeHintLabelText = BuiltRetakeHintLabel;
	DCTheme::AddHBoxChild(TopRightHintRow, RetakeHintButton, FMargin(0.0f), HAlign_Left, VAlign_Center);

	RetakeHintContainer = WidgetTree->ConstructWidget<UBorder>();
	RetakeHintContainer->SetBrushColor(FLinearColor::Transparent);
	RetakeHintContainer->SetRenderOpacity(0.85f);
	RetakeHintContainer->SetContent(TopRightHintRow);
	RetakeHintContainer->SetVisibility(ESlateVisibility::Collapsed);

	if (UOverlaySlot* RetakeSlot = RootOverlay->AddChildToOverlay(RetakeHintContainer))
	{
		RetakeSlot->SetHorizontalAlignment(HAlign_Right);
		RetakeSlot->SetVerticalAlignment(VAlign_Top);
		RetakeSlot->SetPadding(FMargin(0.0f, 22.0f, 26.0f, 0.0f));
	}

	// Layer 2 (Transient Hint Toast ONLY if explicitly requested)
	HintToastText = DCTheme::MakeText(WidgetTree, TEXT(""), 16, DCTheme::Ink(), true, false);
	HintToastBorder = DCTheme::MakeJournalBorder(
		WidgetTree,
		HintToastText,
		DCTheme::Paper(),
		999.0f,
		DCTheme::Ink(),
		3.0f,
		FMargin(22.0f, 9.0f));
	HintToastBorder->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* ToastSlot = RootOverlay->AddChildToOverlay(HintToastBorder))
	{
		ToastSlot->SetHorizontalAlignment(HAlign_Center);
		ToastSlot->SetVerticalAlignment(VAlign_Bottom);
		ToastSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 28.0f));
	}

	// Layer 3: Full-screen TakeResult modal (collapsed unless TakeResult)
	ResultWidget = WidgetTree->ConstructWidget<UDCTakeResultWidget>(UDCTakeResultWidget::StaticClass());
	if (ResultWidget)
	{
		ResultWidget->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* ResultSlot = RootOverlay->AddChildToOverlay(ResultWidget))
		{
			ResultSlot->SetHorizontalAlignment(HAlign_Fill);
			ResultSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	// Layer 4: Top-center transient StarPopup pill
	StarPopupWidget = WidgetTree->ConstructWidget<UDCStarPopupWidget>(UDCStarPopupWidget::StaticClass());
	if (StarPopupWidget)
	{
		StarPopupWidget->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* PopupSlot = RootOverlay->AddChildToOverlay(StarPopupWidget))
		{
			PopupSlot->SetHorizontalAlignment(HAlign_Center);
			PopupSlot->SetVerticalAlignment(VAlign_Top);
		}
	}

	// Layer 5: Full-screen PreTake Persona journal sheet (or Tab Read-Only Peek)
	PersonaPanelWidget = WidgetTree->ConstructWidget<UDCPersonaPanelWidget>(UDCPersonaPanelWidget::StaticClass());
	if (PersonaPanelWidget)
	{
		PersonaPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* PersonaSlot = RootOverlay->AddChildToOverlay(PersonaPanelWidget))
		{
			PersonaSlot->SetHorizontalAlignment(HAlign_Fill);
			PersonaSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	// Layer 6: Shot 01 -> 02 HitTestInvisible cream journal sheet page flip-up ghost
	PageFlipGhostBorder = DCTheme::MakeJournalBorder(
		WidgetTree,
		nullptr,
		DCTheme::Paper(),
		24.0f,
		DCTheme::Ink(),
		4.0f,
		FMargin(0.0f));
	if (PageFlipGhostBorder)
	{
		PageFlipGhostBorder->SetVisibility(ESlateVisibility::Collapsed);
		PageFlipGhostBorder->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		if (UOverlaySlot* GhostSlot = RootOverlay->AddChildToOverlay(PageFlipGhostBorder))
		{
			GhostSlot->SetHorizontalAlignment(HAlign_Fill);
			GhostSlot->SetVerticalAlignment(VAlign_Fill);
			GhostSlot->SetPadding(FMargin(140.0f, 44.0f));
		}
	}
}

void UDCHudOverlayWidget::ApplyConfigWidgetOverrides()
{
	if (!bDefaultLayoutBuilt || !RootOverlay || !WidgetTree || !TakeSubsystem.IsValid())
	{
		return;
	}

	const UDCLevelConfigData* Config = TakeSubsystem->GetActiveConfig();
	if (!Config)
	{
		return;
	}

	bool bSwapped = false;
	if (Config->TakeResultWidgetClass
		&& Config->TakeResultWidgetClass->IsChildOf(UDCTakeResultWidget::StaticClass())
		&& ResultWidget
		&& ResultWidget->GetClass() != Config->TakeResultWidgetClass.Get())
	{
		ResultWidget->RemoveFromParent();
		ResultWidget = WidgetTree->ConstructWidget<UDCTakeResultWidget>(Config->TakeResultWidgetClass.Get());
		bSwapped = true;
	}

	if (Config->PreTakeScriptWidgetClass
		&& Config->PreTakeScriptWidgetClass->IsChildOf(UDCPersonaPanelWidget::StaticClass())
		&& PersonaPanelWidget
		&& PersonaPanelWidget->GetClass() != Config->PreTakeScriptWidgetClass.Get())
	{
		PersonaPanelWidget->RemoveFromParent();
		PersonaPanelWidget = WidgetTree->ConstructWidget<UDCPersonaPanelWidget>(Config->PreTakeScriptWidgetClass.Get());
		bSwapped = true;
	}

	// Re-establish z-order (Result -> StarPopup -> PersonaPanel -> PageFlipGhost) whenever an override class was swapped
	if (bSwapped)
	{
		if (ResultWidget)
		{
			ResultWidget->RemoveFromParent();
			ResultWidget->SetVisibility(ESlateVisibility::Collapsed);
			if (UOverlaySlot* ResultSlot = RootOverlay->AddChildToOverlay(ResultWidget))
			{
				ResultSlot->SetHorizontalAlignment(HAlign_Fill);
				ResultSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
		if (StarPopupWidget)
		{
			StarPopupWidget->RemoveFromParent();
			if (UOverlaySlot* PopupSlot = RootOverlay->AddChildToOverlay(StarPopupWidget))
			{
				PopupSlot->SetHorizontalAlignment(HAlign_Center);
				PopupSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
		if (PersonaPanelWidget)
		{
			PersonaPanelWidget->RemoveFromParent();
			PersonaPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
			if (UOverlaySlot* PersonaSlot = RootOverlay->AddChildToOverlay(PersonaPanelWidget))
			{
				PersonaSlot->SetHorizontalAlignment(HAlign_Fill);
				PersonaSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
		if (PageFlipGhostBorder)
		{
			PageFlipGhostBorder->RemoveFromParent();
			if (UOverlaySlot* GhostSlot = RootOverlay->AddChildToOverlay(PageFlipGhostBorder))
			{
				GhostSlot->SetHorizontalAlignment(HAlign_Fill);
				GhostSlot->SetVerticalAlignment(VAlign_Fill);
				GhostSlot->SetPadding(FMargin(140.0f, 44.0f));
			}
		}
	}
}

void UDCHudOverlayWidget::UnbindDelegates()
{
	if (TakeSubsystem.IsValid())
	{
		TakeSubsystem->OnTakePhaseChanged.RemoveDynamic(this, &UDCHudOverlayWidget::HandleTakePhaseChanged);
		TakeSubsystem->OnTakeReset.RemoveDynamic(this, &UDCHudOverlayWidget::HandleTakeReset);
	}
	if (ActionStateComponent.IsValid())
	{
		ActionStateComponent->OnPersonaPanelToggled.RemoveDynamic(this, &UDCHudOverlayWidget::HandlePersonaPanelToggled);
		ActionStateComponent->OnHudHintRequested.RemoveDynamic(this, &UDCHudOverlayWidget::HandleHudHintRequested);
	}
}

void UDCHudOverlayWidget::InitializeForPlayer(APawn* PlayerPawn)
{
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}

	UnbindDelegates();

	BoundPawn = PlayerPawn;
	ActionStateComponent = PlayerPawn ? PlayerPawn->FindComponentByClass<UDCActionStateComponent>() : nullptr;
	TakeSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UDCTakeSubsystem>() : nullptr;

	ApplyConfigWidgetOverrides();

	if (TakeSubsystem.IsValid())
	{
		TakeSubsystem->OnTakePhaseChanged.AddDynamic(this, &UDCHudOverlayWidget::HandleTakePhaseChanged);
		TakeSubsystem->OnTakeReset.AddDynamic(this, &UDCHudOverlayWidget::HandleTakeReset);
	}
	if (ActionStateComponent.IsValid())
	{
		ActionStateComponent->OnPersonaPanelToggled.AddDynamic(this, &UDCHudOverlayWidget::HandlePersonaPanelToggled);
		ActionStateComponent->OnHudHintRequested.AddDynamic(this, &UDCHudOverlayWidget::HandleHudHintRequested);
	}

	if (PersonaPanelWidget)
	{
		PersonaPanelWidget->BindToPlayer(PlayerPawn);
	}
	if (StarPopupWidget)
	{
		StarPopupWidget->BindToTakeSubsystem(TakeSubsystem.Get());
	}
	if (ResultWidget)
	{
		ResultWidget->BindToTakeSubsystem(TakeSubsystem.Get());
	}

	LastKnownPhase = TakeSubsystem.IsValid() ? TakeSubsystem->GetPhase() : EDCTakePhase::PreTake;
	StopPageFlipGhost();

	RefreshOverlay();
}

void UDCHudOverlayWidget::RefreshOverlay()
{
	if (!TakeSubsystem.IsValid() && GetWorld())
	{
		TakeSubsystem = GetWorld()->GetSubsystem<UDCTakeSubsystem>();
	}
	if (!ActionStateComponent.IsValid() && BoundPawn.IsValid())
	{
		ActionStateComponent = BoundPawn->FindComponentByClass<UDCActionStateComponent>();
	}

	const EDCTakePhase Phase = TakeSubsystem.IsValid() ? TakeSubsystem->GetPhase() : EDCTakePhase::PreTake;
	const UDCLevelConfigData* Config = TakeSubsystem.IsValid() ? TakeSubsystem->GetActiveConfig() : nullptr;
	const int32 EarnedStars = TakeSubsystem.IsValid() ? TakeSubsystem->GetEarnedStars() : 0;
	const int32 MaxContainers = TakeSubsystem.IsValid() ? TakeSubsystem->GetMaxStarContainers() : 3;

	const bool bAllowActivePeek = Config
		&& (Config->ActiveTakePersonaViewPolicy == EDCActiveTakePersonaViewPolicy::AllowTabReadOnlyPeek);
	const bool bActionPanelOpen = ActionStateComponent.IsValid() && ActionStateComponent->IsPersonaPanelOpen();

	const bool bShowPersonaPanel = (Phase == EDCTakePhase::PreTake)
		|| (Phase == EDCTakePhase::ActiveTake && bAllowActivePeek && bActionPanelOpen);
	const bool bShowResultModal = (Phase == EDCTakePhase::TakeResult);
	const bool bShowRetakeHint = (Phase == EDCTakePhase::ActiveTake) && !bShowPersonaPanel;

	if (PersonaPanelWidget)
	{
		const bool bWasVisible = (PersonaPanelWidget->GetVisibility() == ESlateVisibility::Visible);
		PersonaPanelWidget->SetVisibility(bShowPersonaPanel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bShowPersonaPanel)
		{
			PersonaPanelWidget->RefreshPanel();
			if (!bWasVisible && Phase == EDCTakePhase::PreTake)
			{
				PersonaPanelWidget->PlayPaperDropIn();
			}
		}
	}

	if (ResultWidget)
	{
		ResultWidget->SetVisibility(bShowResultModal ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bShowResultModal)
		{
			ResultWidget->RefreshResult();
		}
	}

	if (RetakeHintLabelText && Config && !Config->RetakeHintLabel.IsEmpty())
	{
		RetakeHintLabelText->SetText(Config->RetakeHintLabel);
	}
	if (TabPeekHintContainer)
	{
		const bool bShowTabPeekHint = bShowRetakeHint && bAllowActivePeek;
		TabPeekHintContainer->SetVisibility(bShowTabPeekHint ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (RetakeHintContainer)
	{
		RetakeHintContainer->SetVisibility(bShowRetakeHint ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (StarPopupWidget && Phase != EDCTakePhase::ActiveTake)
	{
		StarPopupWidget->DismissPopup();
	}

	if (HintToastBorder && Phase != EDCTakePhase::ActiveTake)
	{
		HintRemainingSeconds = 0.0f;
		HintToastBorder->SetVisibility(ESlateVisibility::Collapsed);
	}

	BP_OnOverlayRefreshed(Phase, EarnedStars, MaxContainers);
}

void UDCHudOverlayWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bPlayingPageFlipGhost && PageFlipGhostBorder)
	{
		constexpr float FlipDuration = 0.28f;
		PageFlipGhostElapsed += InDeltaTime;
		const float Alpha = FMath::Clamp(PageFlipGhostElapsed / FlipDuration, 0.0f, 1.0f);
		const float EaseOut = 1.0f - FMath::Square(1.0f - Alpha);
		PageFlipGhostBorder->SetRenderTranslation(FVector2D(0.0f, FMath::Lerp(0.0f, -900.0f, EaseOut)));
		PageFlipGhostBorder->SetRenderOpacity(FMath::Lerp(0.85f, 0.0f, Alpha));

		if (Alpha >= 1.0f)
		{
			StopPageFlipGhost();
		}
	}

	if (HintRemainingSeconds > 0.0f && HintToastBorder)
	{
		HintRemainingSeconds = FMath::Max(0.0f, HintRemainingSeconds - InDeltaTime);
		if (HintRemainingSeconds < 0.25f)
		{
			HintToastBorder->SetRenderOpacity(FMath::Clamp(HintRemainingSeconds / 0.25f, 0.0f, 1.0f));
		}
		if (HintRemainingSeconds <= 0.0f)
		{
			HintToastBorder->SetRenderOpacity(1.0f);
			HintToastBorder->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

FReply UDCHudOverlayWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (!TakeSubsystem.IsValid() && GetWorld())
	{
		TakeSubsystem = GetWorld()->GetSubsystem<UDCTakeSubsystem>();
	}
	if (!ActionStateComponent.IsValid() && BoundPawn.IsValid())
	{
		ActionStateComponent = BoundPawn->FindComponentByClass<UDCActionStateComponent>();
	}

	if (Key == EKeys::R && TakeSubsystem.IsValid())
	{
		TakeSubsystem->RestartTake();
		return FReply::Handled();
	}

	if (Key == EKeys::SpaceBar && TakeSubsystem.IsValid() && TakeSubsystem->IsPreTake())
	{
		TakeSubsystem->StartTake();
		return FReply::Handled();
	}

	if (Key == EKeys::Tab && TakeSubsystem.IsValid() && TakeSubsystem->IsActiveTake())
	{
		const UDCLevelConfigData* Config = TakeSubsystem->GetActiveConfig();
		const bool bAllowPeek = Config
			&& (Config->ActiveTakePersonaViewPolicy == EDCActiveTakePersonaViewPolicy::AllowTabReadOnlyPeek);
		if (ActionStateComponent.IsValid())
		{
			if (bAllowPeek)
			{
				ActionStateComponent->SetPersonaPanelOpen(!ActionStateComponent->IsPersonaPanelOpen());
			}
			else if (Config && !Config->PersonaViewDisallowedHintText.IsEmpty())
			{
				ActionStateComponent->RequestHudHint(Config->PersonaViewDisallowedHintText, 2.2f);
			}
		}
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void UDCHudOverlayWidget::StartPageFlipGhost()
{
	bPlayingPageFlipGhost = true;
	PageFlipGhostElapsed = 0.0f;

	if (PageFlipGhostBorder)
	{
		PageFlipGhostBorder->SetRenderTranslation(FVector2D::ZeroVector);
		PageFlipGhostBorder->SetRenderOpacity(0.85f);
		PageFlipGhostBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UDCHudOverlayWidget::StopPageFlipGhost()
{
	bPlayingPageFlipGhost = false;
	PageFlipGhostElapsed = 0.0f;

	if (PageFlipGhostBorder)
	{
		PageFlipGhostBorder->SetRenderTranslation(FVector2D::ZeroVector);
		PageFlipGhostBorder->SetRenderOpacity(1.0f);
		PageFlipGhostBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UDCHudOverlayWidget::HandleTakePhaseChanged(EDCTakePhase NewPhase)
{
	const EDCTakePhase PrevPhase = LastKnownPhase;
	LastKnownPhase = NewPhase;

	if (PrevPhase == EDCTakePhase::PreTake && NewPhase == EDCTakePhase::ActiveTake)
	{
		StartPageFlipGhost();
	}
	else if (NewPhase != EDCTakePhase::ActiveTake)
	{
		StopPageFlipGhost();
	}

	RefreshOverlay();
}

void UDCHudOverlayWidget::HandleTakeReset()
{
	LastKnownPhase = TakeSubsystem.IsValid() ? TakeSubsystem->GetPhase() : EDCTakePhase::PreTake;
	StopPageFlipGhost();

	HintRemainingSeconds = 0.0f;
	if (HintToastBorder)
	{
		HintToastBorder->SetRenderOpacity(1.0f);
		HintToastBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
	RefreshOverlay();
}

void UDCHudOverlayWidget::HandlePersonaPanelToggled(bool bIsOpen)
{
	RefreshOverlay();
}

void UDCHudOverlayWidget::HandleHudHintRequested(FText HintText, float DurationSeconds)
{
	if (!HintToastBorder || !HintToastText || HintText.IsEmpty())
	{
		return;
	}
	HintToastText->SetText(HintText);
	HintRemainingSeconds = DurationSeconds > 0.0f ? DurationSeconds : 2.4f;
	HintToastBorder->SetRenderOpacity(1.0f);
	HintToastBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UDCHudOverlayWidget::HandleRetakeHintClicked()
{
	if (!TakeSubsystem.IsValid() && GetWorld())
	{
		TakeSubsystem = GetWorld()->GetSubsystem<UDCTakeSubsystem>();
	}
	if (TakeSubsystem.IsValid())
	{
		TakeSubsystem->RestartTake();
	}
}

void UDCHudOverlayWidget::HandleTabPeekHintClicked()
{
	if (!TakeSubsystem.IsValid() && GetWorld())
	{
		TakeSubsystem = GetWorld()->GetSubsystem<UDCTakeSubsystem>();
	}
	if (!ActionStateComponent.IsValid() && BoundPawn.IsValid())
	{
		ActionStateComponent = BoundPawn->FindComponentByClass<UDCActionStateComponent>();
	}

	if (TakeSubsystem.IsValid() && TakeSubsystem->IsActiveTake() && ActionStateComponent.IsValid())
	{
		const UDCLevelConfigData* Config = TakeSubsystem->GetActiveConfig();
		const bool bAllowPeek = Config
			&& (Config->ActiveTakePersonaViewPolicy == EDCActiveTakePersonaViewPolicy::AllowTabReadOnlyPeek);
		if (bAllowPeek)
		{
			ActionStateComponent->SetPersonaPanelOpen(!ActionStateComponent->IsPersonaPanelOpen());
		}
	}
}
