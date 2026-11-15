#include "UI/DCPersonaPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/DCTakeSubsystem.h"
#include "Data/DCLevelConfigData.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Persona/DCPersonaComponent.h"
#include "Sound/SoundBase.h"
#include "UI/DCWidgetTheme.h"

namespace
{
	FText ResolveTextOrFallback(const FText& Candidate, const FString& Fallback)
	{
		return Candidate.IsEmpty() ? FText::FromString(Fallback) : Candidate;
	}

	UWidget* MakeLegendItem(
		UWidgetTree* Tree,
		UWidget* Swatch,
		const FString& Label,
		UTextBlock*& OutLabel)
	{
		UHorizontalBox* Pair = Tree->ConstructWidget<UHorizontalBox>();
		DCTheme::AddHBoxChild(Pair, Swatch, FMargin(0.0f, 0.0f, 8.0f, 0.0f), HAlign_Left, VAlign_Center);

		OutLabel = DCTheme::MakeText(Tree, Label, 14, DCTheme::InkMuted(), true, false);
		DCTheme::AddHBoxChild(Pair, OutLabel, FMargin(0.0f), HAlign_Left, VAlign_Center);
		return Pair;
	}
}

// -----------------------------------------------------------------------------
// UDCBlockCutButton
// -----------------------------------------------------------------------------

UDCBlockCutButton::UDCBlockCutButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDCBlockCutButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}
}

TSharedRef<SWidget> UDCBlockCutButton::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}
	return Super::RebuildWidget();
}

void UDCBlockCutButton::NativeConstruct()
{
	Super::NativeConstruct();

	if (CutButton && !bBindingsWired)
	{
		bBindingsWired = true;
		CutButton->OnClicked.AddDynamic(this, &UDCBlockCutButton::HandleButtonClicked);
		CutButton->OnHovered.AddDynamic(this, &UDCBlockCutButton::HandleButtonHovered);
		CutButton->OnUnhovered.AddDynamic(this, &UDCBlockCutButton::HandleButtonUnhovered);
	}

	ApplyVisualState();
}

void UDCBlockCutButton::NativeDestruct()
{
	if (CutButton && bBindingsWired)
	{
		CutButton->OnClicked.RemoveDynamic(this, &UDCBlockCutButton::HandleButtonClicked);
		CutButton->OnHovered.RemoveDynamic(this, &UDCBlockCutButton::HandleButtonHovered);
		CutButton->OnUnhovered.RemoveDynamic(this, &UDCBlockCutButton::HandleButtonUnhovered);
		bBindingsWired = false;
	}
	Super::NativeDestruct();
}

void UDCBlockCutButton::EnsureDefaultLayout()
{
	if (bDefaultLayoutBuilt || !WidgetTree)
	{
		return;
	}
	bDefaultLayoutBuilt = true;

	UBorder* RowMargin = WidgetTree->ConstructWidget<UBorder>();
	RowMargin->SetBrushColor(FLinearColor::Transparent);
	RowMargin->SetPadding(FMargin(0.0f, 2.0f));
	WidgetTree->RootWidget = RowMargin;

	BlockHighlightBorder = WidgetTree->ConstructWidget<UBorder>();
	BlockHighlightBorder->SetBrushColor(FLinearColor::Transparent);
	BlockHighlightBorder->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	RowMargin->SetContent(BlockHighlightBorder);

	CutButton = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle TransparentStyle;
	TransparentStyle.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
	TransparentStyle.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
	TransparentStyle.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
	TransparentStyle.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
	TransparentStyle.NormalPadding = FMargin(0.0f);
	TransparentStyle.PressedPadding = FMargin(0.0f);
	CutButton->SetStyle(TransparentStyle);
	BlockHighlightBorder->SetContent(CutButton);

	UBorder* ContentPad = WidgetTree->ConstructWidget<UBorder>();
	ContentPad->SetBrushColor(FLinearColor::Transparent);
	ContentPad->SetPadding(FMargin(14.0f, 7.0f));
	CutButton->SetContent(ContentPad);

	UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>();
	ContentPad->SetContent(RowBox);

	UVerticalBox* SentenceStack = WidgetTree->ConstructWidget<UVerticalBox>();
	DCTheme::AddHBoxChild(
		RowBox,
		SentenceStack,
		FMargin(0.0f),
		HAlign_Fill,
		VAlign_Center,
		ESlateSizeRule::Fill,
		1.0f);

	UOverlay* TextOverlay = WidgetTree->ConstructWidget<UOverlay>();
	DCTheme::AddVBoxChild(SentenceStack, TextOverlay, FMargin(0.0f), HAlign_Left, VAlign_Center);

	ButtonCaptionText = DCTheme::MakeText(WidgetTree, TEXT(""), 19, DCTheme::Ink(), false, true);
	if (UOverlaySlot* TextSlot = TextOverlay->AddChildToOverlay(ButtonCaptionText))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Left);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetPadding(FMargin(4.0f, 0.0f));
	}

	USizeBox* StrikeBarSize = WidgetTree->ConstructWidget<USizeBox>();
	StrikeBarSize->SetHeightOverride(4.2f);
	StrikeBarSize->SetVisibility(ESlateVisibility::Collapsed);
	CutStrikeBarContainer = StrikeBarSize;

	StrikeThroughBar = DCTheme::MakeJournalBorder(
		WidgetTree, nullptr, DCTheme::Ink(), 2.0f, FLinearColor::Transparent, 0.0f, FMargin(0.0f));
	StrikeThroughBar->SetRenderTransformPivot(FVector2D(0.0f, 0.5f));
	StrikeThroughBar->SetRenderTransformAngle(-0.35f);
	StrikeThroughBar->SetRenderScale(FVector2D(1.0f, 1.0f));
	StrikeBarSize->SetContent(StrikeThroughBar);

	if (UOverlaySlot* StrikeOverlaySlot = TextOverlay->AddChildToOverlay(StrikeBarSize))
	{
		StrikeOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		StrikeOverlaySlot->SetVerticalAlignment(VAlign_Center);
	}

	UDCDashedRuleWidget* UnderlineRule = WidgetTree->ConstructWidget<UDCDashedRuleWidget>();
	UnderlineRule->ConfigureRule(DCTheme::CuttableUnderline(), 2.2f, 7.0f, 5.0f);
	CuttableUnderlineWidget = UnderlineRule;
	DCTheme::AddVBoxChild(SentenceStack, UnderlineRule, FMargin(0.0f, 2.0f, 0.0f, 0.0f), HAlign_Fill, VAlign_Top);

	// Always reserve fixed icon width so hovering cuttable rows never wraps/shifts words horizontally.
	USizeBox* ScissorsSlot = WidgetTree->ConstructWidget<USizeBox>();
	ScissorsSlot->SetMinDesiredWidth(28.0f);
	ScissorsSlot->SetMinDesiredHeight(28.0f);

	HoverScissorsIcon = WidgetTree->ConstructWidget<UDCJournalScissorsWidget>();
	HoverScissorsIcon->SetIconSize(26.0f);
	HoverScissorsIcon->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	HoverScissorsIcon->SetRenderTransformAngle(0.0f);
	HoverScissorsIcon->SetRenderOpacity(0.0f);
	HoverScissorsIcon->SetVisibility(ESlateVisibility::Hidden);
	ScissorsSlot->SetContent(HoverScissorsIcon);

	DCTheme::AddHBoxChild(RowBox, ScissorsSlot, FMargin(12.0f, 0.0f, 0.0f, 0.0f), HAlign_Right, VAlign_Center);
}

void UDCBlockCutButton::SetupBlockButton(
	const FDCPersonaBlock& InBlock,
	bool bCanEditNow,
	UDCPersonaComponent* InOwnerComponent,
	UTexture2D* InScissorsTexture,
	float InCutAnimSeconds,
	bool bPlayJustCutStrikeAnim)
{
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}

	CachedBlock = InBlock;
	bEditableNow = bCanEditNow;
	OwnerComponent = InOwnerComponent;
	CutAnimDuration = FMath::Max(0.08f, InCutAnimSeconds);

	if (HoverScissorsIcon)
	{
		HoverScissorsIcon->SetTextureOverride(InScissorsTexture);
	}

	if (!InBlock.bIsPresent && bPlayJustCutStrikeAnim)
	{
		bStrikeAnimPlaying = true;
		StrikeProgress = 0.0f;
	}
	else
	{
		bStrikeAnimPlaying = false;
		StrikeProgress = 1.0f;
	}

	bIsHovered = false;
	ApplyVisualState();
}

void UDCBlockCutButton::ApplyVisualState()
{
	const bool bIsCut = CachedBlock.bCanDelete && !CachedBlock.bIsPresent;
	const bool bIsCuttable = CachedBlock.bCanDelete && CachedBlock.bIsPresent;
	const bool bCanClick = bIsCuttable && bEditableNow;
	const bool bShowHover = bCanClick && bIsHovered;

	if (ButtonCaptionText)
	{
		// Always preserve the original block copy verbatim even after being cut.
		ButtonCaptionText->SetText(CachedBlock.DisplayText);
		ButtonCaptionText->SetColorAndOpacity(FSlateColor(bIsCut ? DCTheme::CutTextMuted() : DCTheme::Ink()));
	}

	if (BlockHighlightBorder)
	{
		if (bIsCuttable)
		{
			const FLinearColor BandFill = bShowHover ? DCTheme::Blush() : DCTheme::Highlighter();
			DCTheme::ApplyRoundedBorderStyle(BlockHighlightBorder, BandFill, 10.0f, FLinearColor::Transparent, 0.0f);
			BlockHighlightBorder->SetRenderTranslation(bShowHover ? FVector2D(0.0f, -2.0f) : FVector2D::ZeroVector);
			BlockHighlightBorder->SetRenderScale(bShowHover ? FVector2D(1.01f, 1.01f) : FVector2D(1.0f, 1.0f));
		}
		else
		{
			BlockHighlightBorder->SetBrush(FSlateNoResource());
			BlockHighlightBorder->SetBrushColor(FLinearColor::Transparent);
			BlockHighlightBorder->SetRenderTranslation(FVector2D::ZeroVector);
			BlockHighlightBorder->SetRenderScale(FVector2D(1.0f, 1.0f));
		}
	}

	if (CuttableUnderlineWidget)
	{
		CuttableUnderlineWidget->SetVisibility(
			(bIsCuttable && !bShowHover) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (HoverScissorsIcon)
	{
		if (bShowHover)
		{
			HoverScissorsIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
			HoverScissorsIcon->SetRenderOpacity(1.0f);
			HoverScissorsIcon->SetRenderTransformAngle(-10.0f);
		}
		else if (bIsCuttable)
		{
			HoverScissorsIcon->SetVisibility(ESlateVisibility::Hidden);
			HoverScissorsIcon->SetRenderOpacity(0.0f);
			HoverScissorsIcon->SetRenderTransformAngle(0.0f);
		}
		else
		{
			HoverScissorsIcon->SetVisibility(ESlateVisibility::Collapsed);
			HoverScissorsIcon->SetRenderOpacity(0.0f);
			HoverScissorsIcon->SetRenderTransformAngle(0.0f);
		}
	}

	if (CutStrikeBarContainer && StrikeThroughBar)
	{
		if (bIsCut)
		{
			CutStrikeBarContainer->SetVisibility(ESlateVisibility::HitTestInvisible);
			const float Eased = 1.0f - FMath::Pow(1.0f - FMath::Clamp(StrikeProgress, 0.0f, 1.0f), 2.5f);
			StrikeThroughBar->SetRenderScale(FVector2D(Eased, 1.0f));
		}
		else
		{
			CutStrikeBarContainer->SetVisibility(ESlateVisibility::Collapsed);
			StrikeThroughBar->SetRenderScale(FVector2D(1.0f, 1.0f));
		}
	}

	if (CutButton)
	{
		// Use HitTestInvisible instead of SetIsEnabled(false) so Fixed/Cut lines do not suffer UE's 0.45 disabled dim.
		CutButton->SetIsEnabled(true);
		CutButton->SetVisibility(bCanClick ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);
		if (!bCanClick)
		{
			bIsHovered = false;
		}
	}

	if (bShowHover)
	{
		SetCursor(EMouseCursor::Hand);
		if (CutButton)
		{
			CutButton->SetCursor(EMouseCursor::Hand);
		}
	}
	else
	{
		SetCursor(EMouseCursor::Default);
		ResetCursor();
		if (CutButton)
		{
			CutButton->SetCursor(EMouseCursor::Default);
			CutButton->ResetCursor();
		}
	}

	Invalidate(EInvalidateWidgetReason::Paint);
}

void UDCBlockCutButton::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (CutButton)
	{
		const bool bCanClick = CachedBlock.bCanDelete && CachedBlock.bIsPresent && bEditableNow;
		const bool bNowHovered = bCanClick && CutButton->IsHovered();
		if (bNowHovered != bIsHovered)
		{
			bIsHovered = bNowHovered;
			ApplyVisualState();
		}
	}

	if (bStrikeAnimPlaying)
	{
		StrikeProgress = FMath::Clamp(StrikeProgress + InDeltaTime / FMath::Max(0.05f, CutAnimDuration), 0.0f, 1.0f);
		if (StrikeThroughBar)
		{
			const float Eased = 1.0f - FMath::Pow(1.0f - StrikeProgress, 2.5f);
			StrikeThroughBar->SetRenderScale(FVector2D(Eased, 1.0f));
		}
		if (StrikeProgress >= 1.0f)
		{
			bStrikeAnimPlaying = false;
			StrikeProgress = 1.0f;
		}
	}
}

void UDCBlockCutButton::HandleButtonClicked()
{
	if (!bEditableNow || !CachedBlock.bCanDelete || !CachedBlock.bIsPresent || !OwnerComponent.IsValid())
	{
		return;
	}

	bIsHovered = false;
	SetCursor(EMouseCursor::Default);
	ResetCursor();
	if (CutButton)
	{
		CutButton->SetCursor(EMouseCursor::Default);
		CutButton->ResetCursor();
	}

	const FName TargetID = CachedBlock.BlockID;
	OnBlockCutTriggered.Broadcast(TargetID);
	OwnerComponent->TryDeleteBlock(TargetID);
}

void UDCBlockCutButton::HandleButtonHovered()
{
	if (!bEditableNow || !CachedBlock.bCanDelete || !CachedBlock.bIsPresent)
	{
		return;
	}
	bIsHovered = true;
	ApplyVisualState();
}

void UDCBlockCutButton::HandleButtonUnhovered()
{
	if (bIsHovered)
	{
		bIsHovered = false;
		ApplyVisualState();
	}
}

// -----------------------------------------------------------------------------
// UDCPersonaPanelWidget
// -----------------------------------------------------------------------------

UDCPersonaPanelWidget::UDCPersonaPanelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDCPersonaPanelWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}
}

TSharedRef<SWidget> UDCPersonaPanelWidget::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}
	return Super::RebuildWidget();
}

void UDCPersonaPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!bBindingsWired)
	{
		bBindingsWired = true;
		if (StartTakeButton)
		{
			StartTakeButton->OnClicked.AddDynamic(this, &UDCPersonaPanelWidget::HandleStartTakeClicked);
		}
		if (RestartTakeButton)
		{
			RestartTakeButton->OnClicked.AddDynamic(this, &UDCPersonaPanelWidget::HandleRestartTakeClicked);
		}
	}

	RefreshPanel();
}

void UDCPersonaPanelWidget::NativeDestruct()
{
	UnbindDelegates();
	if (bBindingsWired)
	{
		if (StartTakeButton)
		{
			StartTakeButton->OnClicked.RemoveDynamic(this, &UDCPersonaPanelWidget::HandleStartTakeClicked);
		}
		if (RestartTakeButton)
		{
			RestartTakeButton->OnClicked.RemoveDynamic(this, &UDCPersonaPanelWidget::HandleRestartTakeClicked);
		}
		bBindingsWired = false;
	}
	Super::NativeDestruct();
}

void UDCPersonaPanelWidget::EnsureDefaultLayout()
{
	if (bDefaultLayoutBuilt || !WidgetTree)
	{
		return;
	}
	bDefaultLayoutBuilt = true;

	// Full-screen solid Desk backdrop covering 3D room completely during PreTake.
	BackdropBorder = WidgetTree->ConstructWidget<UBorder>();
	BackdropBorder->SetBrushColor(DCTheme::Desk());
	BackdropBorder->SetPadding(FMargin(68.0f, 44.0f));
	BackdropBorder->SetHorizontalAlignment(HAlign_Fill);
	BackdropBorder->SetVerticalAlignment(VAlign_Fill);
	WidgetTree->RootWidget = BackdropBorder;

	UOverlay* SheetOverlay = WidgetTree->ConstructWidget<UOverlay>();
	BackdropBorder->SetContent(SheetOverlay);

	// Centered journal page card
	USizeBox* SheetMaxFrame = WidgetTree->ConstructWidget<USizeBox>();
	SheetMaxFrame->SetMinDesiredWidth(1520.0f);
	SheetMaxFrame->SetMaxDesiredWidth(1620.0f);
	UOverlaySlot* SheetSlot = SheetOverlay->AddChildToOverlay(SheetMaxFrame);
	if (SheetSlot)
	{
		SheetSlot->SetHorizontalAlignment(HAlign_Center);
		SheetSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* SheetContent = WidgetTree->ConstructWidget<UVerticalBox>();
	JournalSheetBorder = DCTheme::MakeJournalBorder(
		WidgetTree,
		SheetContent,
		DCTheme::Paper(),
		24.0f,
		DCTheme::Ink(),
		4.0f,
		FMargin(64.0f, 36.0f, 64.0f, 32.0f));
	JournalSheetBorder->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	SheetMaxFrame->SetContent(JournalSheetBorder);

	// Wrap sheet card and corner tapes inside a shared overlay so tapes always hug card corners cleanly
	SheetMaxFrame->SetContent(nullptr);
	UOverlay* CardWithTapesOverlay = WidgetTree->ConstructWidget<UOverlay>();
	SheetMaxFrame->SetContent(CardWithTapesOverlay);

	UOverlaySlot* CardLayer = CardWithTapesOverlay->AddChildToOverlay(JournalSheetBorder);
	if (CardLayer)
	{
		CardLayer->SetHorizontalAlignment(HAlign_Fill);
		CardLayer->SetVerticalAlignment(VAlign_Fill);
	}

	const auto AttachCornerTape = [&](const FLinearColor& TapeColor, float AngleDeg, EHorizontalAlignment HAlign, EVerticalAlignment VAlign, const FVector2D& Offset)
	{
		USizeBox* Tape = DCTheme::MakeCornerTape(WidgetTree, TapeColor, AngleDeg, 142.0f, 40.0f);
		Tape->SetRenderTranslation(Offset);
		UOverlaySlot* TapeSlot = CardWithTapesOverlay->AddChildToOverlay(Tape);
		if (TapeSlot)
		{
			TapeSlot->SetHorizontalAlignment(HAlign);
			TapeSlot->SetVerticalAlignment(VAlign);
		}
	};

	AttachCornerTape(DCTheme::TapeCream(), -11.0f, HAlign_Left, VAlign_Top, FVector2D(-26.0f, -12.0f));
	AttachCornerTape(DCTheme::TapeCream(), +10.0f, HAlign_Right, VAlign_Top, FVector2D(26.0f, -12.0f));
	AttachCornerTape(DCTheme::TapePink(), +9.0f, HAlign_Left, VAlign_Bottom, FVector2D(-26.0f, 12.0f));
	AttachCornerTape(DCTheme::TapeBlue(), -10.0f, HAlign_Right, VAlign_Bottom, FVector2D(26.0f, 12.0f));

	// Optional decorative tilted Angel Pup sticker in the lower-right journal margin (collapsed if texture absent)
	AngelDogStickerImage = WidgetTree->ConstructWidget<UImage>();
	AngelDogStickerImage->SetDesiredSizeOverride(FVector2D(108.0f, 108.0f));
	AngelDogStickerImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	AngelDogStickerImage->SetRenderTransformAngle(9.0f);
	AngelDogStickerImage->SetRenderOpacity(0.94f);
	AngelDogStickerImage->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* StickerSlot = CardWithTapesOverlay->AddChildToOverlay(AngelDogStickerImage))
	{
		StickerSlot->SetHorizontalAlignment(HAlign_Right);
		StickerSlot->SetVerticalAlignment(VAlign_Bottom);
		StickerSlot->SetPadding(FMargin(0.0f, 0.0f, 52.0f, 108.0f));
	}

	// Header row: PERSONA + Fire Call pill + tilted Blush Director's Cut tape badge + optional READ-ONLY pill
	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	DCTheme::AddVBoxChild(SheetContent, HeaderRow, FMargin(0.0f, 0.0f, 0.0f, 12.0f), HAlign_Fill, VAlign_Top);

	RoleTitleText = DCTheme::MakeText(WidgetTree, TEXT("PERSONA"), 34, DCTheme::Ink(), true, false);
	DCTheme::AddHBoxChild(HeaderRow, RoleTitleText, FMargin(0.0f), HAlign_Left, VAlign_Center);

	UTextBlock* BuiltBadgeText = nullptr;
	PhaseBadgeBorder = DCTheme::MakeBadgePill(
		WidgetTree,
		TEXT("Fire Call"),
		DCTheme::Highlighter(),
		DCTheme::Ink(),
		18,
		DCTheme::Ink(),
		3.0f,
		FMargin(18.0f, 5.0f),
		&BuiltBadgeText);
	PhaseBadgeText = BuiltBadgeText;
	DCTheme::AddHBoxChild(HeaderRow, PhaseBadgeBorder, FMargin(18.0f, 0.0f, 0.0f, 0.0f), HAlign_Left, VAlign_Center);

	UHorizontalBox* DirectorsCutRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	UDCJournalScissorsWidget* BuiltTapeScissors = WidgetTree->ConstructWidget<UDCJournalScissorsWidget>();
	BuiltTapeScissors->SetIconSize(19.0f);
	BuiltTapeScissors->SetTint(DCTheme::Ink());
	BuiltTapeScissors->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	BuiltTapeScissors->SetRenderTransformAngle(-14.0f);
	DirectorsCutScissorsIcon = BuiltTapeScissors;
	DCTheme::AddHBoxChild(DirectorsCutRow, BuiltTapeScissors, FMargin(0.0f, 0.0f, 7.0f, 0.0f), HAlign_Left, VAlign_Center);

	UTextBlock* BuiltDirectorsCutText = DCTheme::MakeText(WidgetTree, TEXT("Director's Cut"), 15, DCTheme::Ink(), true, false);
	DirectorsCutBadgeText = BuiltDirectorsCutText;
	DCTheme::AddHBoxChild(DirectorsCutRow, BuiltDirectorsCutText, FMargin(0.0f), HAlign_Left, VAlign_Center);

	DirectorsCutBadgeBorder = DCTheme::MakeJournalBorder(
		WidgetTree,
		DirectorsCutRow,
		DCTheme::Blush(),
		8.0f,
		DCTheme::Ink(),
		2.5f,
		FMargin(12.0f, 4.0f));
	DirectorsCutBadgeBorder->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	DirectorsCutBadgeBorder->SetRenderTransformAngle(-3.2f);
	DCTheme::AddHBoxChild(HeaderRow, DirectorsCutBadgeBorder, FMargin(14.0f, 0.0f, 0.0f, 0.0f), HAlign_Left, VAlign_Center);

	USpacer* HeaderPush = WidgetTree->ConstructWidget<USpacer>();
	DCTheme::AddHBoxChild(HeaderRow, HeaderPush, FMargin(0.0f), HAlign_Fill, VAlign_Center, ESlateSizeRule::Fill, 1.0f);

	UTextBlock* BuiltReadOnlyText = nullptr;
	ReadOnlyBadgeBorder = DCTheme::MakeBadgePill(
		WidgetTree,
		TEXT("READ-ONLY"),
		DCTheme::CalmResultTag(),
		DCTheme::Ink(),
		14,
		DCTheme::Ink(),
		2.5f,
		FMargin(14.0f, 4.0f),
		&BuiltReadOnlyText);
	ReadOnlyBadgeText = BuiltReadOnlyText;
	ReadOnlyBadgeBorder->SetVisibility(ESlateVisibility::Collapsed);
	DCTheme::AddHBoxChild(HeaderRow, ReadOnlyBadgeBorder, FMargin(12.0f, 0.0f, 0.0f, 0.0f), HAlign_Right, VAlign_Center);

	// Dashed horizontal divider
	UDCDashedRuleWidget* HeaderDivider = WidgetTree->ConstructWidget<UDCDashedRuleWidget>();
	HeaderDivider->ConfigureRule(DCTheme::DashedRule(), 3.0f, 9.0f, 12.0f);
	DCTheme::AddVBoxChild(SheetContent, HeaderDivider, FMargin(0.0f, 0.0f, 0.0f, 8.0f), HAlign_Fill, VAlign_Top);

	// 1-line subtitle directly under header divider
	SubtitleText = DCTheme::MakeText(
		WidgetTree,
		TEXT("Click yellow lines to snip memories before ACTION! (Careful\u2014some memories help!)"),
		16,
		DCTheme::InkMuted(),
		false,
		true);
	DCTheme::AddVBoxChild(SheetContent, SubtitleText, FMargin(2.0f, 0.0f, 2.0f, 10.0f), HAlign_Fill, VAlign_Top);

	// Journal prose body container
	UVerticalBox* BlocksVBox = WidgetTree->ConstructWidget<UVerticalBox>();
	BlocksContainer = BlocksVBox;
	DCTheme::AddVBoxChild(SheetContent, BlocksVBox, FMargin(0.0f), HAlign_Fill, VAlign_Top);

	// Flexible spacer pushing footer legend & ACTION! button neatly to bottom of journal card
	USpacer* BodyFlexSpacer = WidgetTree->ConstructWidget<USpacer>();
	DCTheme::AddVBoxChild(SheetContent, BodyFlexSpacer, FMargin(0.0f), HAlign_Fill, VAlign_Fill, ESlateSizeRule::Fill, 1.0f);

	// Footer row: Legend swatches on left, chubby ACTION! pill button on right
	UHorizontalBox* FooterRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	DCTheme::AddVBoxChild(SheetContent, FooterRow, FMargin(0.0f, 18.0f, 0.0f, 0.0f), HAlign_Fill, VAlign_Bottom);

	UHorizontalBox* LegendRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	DCTheme::AddHBoxChild(FooterRow, LegendRow, FMargin(0.0f), HAlign_Left, VAlign_Center, ESlateSizeRule::Fill, 1.0f);

	// FIXED swatch
	USizeBox* FixedSwatchBox = WidgetTree->ConstructWidget<USizeBox>();
	FixedSwatchBox->SetWidthOverride(24.0f);
	FixedSwatchBox->SetHeightOverride(16.0f);
	UTextBlock* FixedIconText = DCTheme::MakeText(WidgetTree, TEXT("A"), 12, DCTheme::Ink(), true, false);
	FixedIconText->SetJustification(ETextJustify::Center);
	UBorder* FixedSwatchBorder = DCTheme::MakeJournalBorder(
		WidgetTree, FixedIconText, DCTheme::Paper(), 4.0f, DCTheme::DashedRule(), 2.0f, FMargin(0.0f));
	FixedSwatchBorder->SetHorizontalAlignment(HAlign_Center);
	FixedSwatchBorder->SetVerticalAlignment(VAlign_Center);
	FixedSwatchBox->SetContent(FixedSwatchBorder);
	UTextBlock* FixedLabelPtr = nullptr;
	DCTheme::AddHBoxChild(
		LegendRow,
		MakeLegendItem(WidgetTree, FixedSwatchBox, TEXT("FIXED"), FixedLabelPtr),
		FMargin(0.0f, 0.0f, 22.0f, 0.0f),
		HAlign_Left,
		VAlign_Center);
	LegendFixedLabelText = FixedLabelPtr;

	// CUTTABLE swatch
	USizeBox* CuttableSwatchBox = WidgetTree->ConstructWidget<USizeBox>();
	CuttableSwatchBox->SetWidthOverride(26.0f);
	CuttableSwatchBox->SetHeightOverride(16.0f);
	UBorder* CuttableSwatchBorder = DCTheme::MakeJournalBorder(
		WidgetTree, nullptr, DCTheme::Highlighter(), 5.0f, DCTheme::CuttableUnderline(), 2.0f, FMargin(0.0f));
	CuttableSwatchBox->SetContent(CuttableSwatchBorder);
	UTextBlock* CuttableLabelPtr = nullptr;
	DCTheme::AddHBoxChild(
		LegendRow,
		MakeLegendItem(WidgetTree, CuttableSwatchBox, TEXT("CUTTABLE"), CuttableLabelPtr),
		FMargin(0.0f, 0.0f, 22.0f, 0.0f),
		HAlign_Left,
		VAlign_Center);
	LegendCuttableLabelText = CuttableLabelPtr;

	// CUT swatch
	USizeBox* CutSwatchBox = WidgetTree->ConstructWidget<USizeBox>();
	CutSwatchBox->SetWidthOverride(24.0f);
	CutSwatchBox->SetHeightOverride(16.0f);
	UOverlay* CutStrikeOverlay = WidgetTree->ConstructWidget<UOverlay>();
	USizeBox* StrikeBarSize = WidgetTree->ConstructWidget<USizeBox>();
	StrikeBarSize->SetWidthOverride(16.0f);
	StrikeBarSize->SetHeightOverride(3.0f);
	UBorder* StrikeBar = DCTheme::MakeJournalBorder(WidgetTree, nullptr, DCTheme::Ink(), 1.0f, FLinearColor::Transparent, 0.0f, FMargin(0.0f));
	StrikeBarSize->SetContent(StrikeBar);
	if (UOverlaySlot* StrikeSlot = CutStrikeOverlay->AddChildToOverlay(StrikeBarSize))
	{
		StrikeSlot->SetHorizontalAlignment(HAlign_Center);
		StrikeSlot->SetVerticalAlignment(VAlign_Center);
	}
	UBorder* CutSwatchBorder = DCTheme::MakeJournalBorder(
		WidgetTree, CutStrikeOverlay, DCTheme::Paper(), 4.0f, DCTheme::CutTextMuted(), 2.0f, FMargin(0.0f));
	CutSwatchBorder->SetHorizontalAlignment(HAlign_Center);
	CutSwatchBorder->SetVerticalAlignment(VAlign_Center);
	CutSwatchBox->SetContent(CutSwatchBorder);
	UTextBlock* CutLabelPtr = nullptr;
	DCTheme::AddHBoxChild(
		LegendRow,
		MakeLegendItem(WidgetTree, CutSwatchBox, TEXT("CUT"), CutLabelPtr),
		FMargin(0.0f),
		HAlign_Left,
		VAlign_Center);
	LegendCutLabelText = CutLabelPtr;

	// Secondary compact [R] Reset chubby pill button beside ACTION! (PreTake only)
	UTextBlock* BuiltResetLabel = nullptr;
	UButton* BuiltResetBtn = DCTheme::MakeKeycapPillButton(
		WidgetTree,
		TEXT("R"),
		TEXT("Reset"),
		DCTheme::Paper(),
		DCTheme::TapeCream(),
		3.0f,
		28.0f,
		14,
		17,
		FMargin(16.0f, 9.0f),
		&BuiltResetLabel);
	RestartTakeButton = BuiltResetBtn;
	ResetTakeButtonLabel = BuiltResetLabel;
	ResetButtonLipContainer = DCTheme::MakeJournalBorder(
		WidgetTree,
		BuiltResetBtn,
		DCTheme::KeycapShadow(),
		999.0f,
		DCTheme::Ink(),
		3.0f,
		FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	DCTheme::AddHBoxChild(FooterRow, ResetButtonLipContainer, FMargin(16.0f, 0.0f, 0.0f, 0.0f), HAlign_Right, VAlign_Center);

	// ACTION! chubby yellow pill button
	UButton* BuiltStartBtn = nullptr;
	UTextBlock* BuiltStartLabel = nullptr;
	ActionButtonLipContainer = DCTheme::MakeChubbyButton(
		WidgetTree,
		BuiltStartBtn,
		FText::FromString(TEXT("ACTION!")),
		DCTheme::Butter(),
		DCTheme::Blush(),
		DCTheme::ButterShadow(),
		DCTheme::Ink(),
		24,
		999.0f,
		4.0f,
		FMargin(36.0f, 11.0f),
		FDCOptionalTextBlockOut(BuiltStartLabel));
	StartTakeButton = BuiltStartBtn;
	StartTakeButtonLabel = BuiltStartLabel;
	DCTheme::AddHBoxChild(FooterRow, ActionButtonLipContainer, FMargin(14.0f, 0.0f, 0.0f, 0.0f), HAlign_Right, VAlign_Center);
}

void UDCPersonaPanelWidget::UnbindDelegates()
{
	if (PersonaComponent.IsValid())
	{
		PersonaComponent->OnPersonaChanged.RemoveDynamic(this, &UDCPersonaPanelWidget::HandlePersonaChanged);
	}
	if (TakeSubsystem.IsValid())
	{
		TakeSubsystem->OnTakePhaseChanged.RemoveDynamic(this, &UDCPersonaPanelWidget::HandleTakePhaseChanged);
	}
}

void UDCPersonaPanelWidget::BindToPlayer(APawn* PlayerPawn)
{
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}

	UnbindDelegates();

	BoundPawn = PlayerPawn;
	PersonaComponent = PlayerPawn ? PlayerPawn->FindComponentByClass<UDCPersonaComponent>() : nullptr;
	TakeSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UDCTakeSubsystem>() : nullptr;

	PreviouslyPresentBlockIDs.Reset();
	if (PersonaComponent.IsValid())
	{
		for (const FDCPersonaBlock& Block : PersonaComponent->GetBlocks())
		{
			if (Block.bEnabledInLevel && Block.bIsPresent)
			{
				PreviouslyPresentBlockIDs.Add(Block.BlockID);
			}
		}
		PersonaComponent->OnPersonaChanged.AddDynamic(this, &UDCPersonaPanelWidget::HandlePersonaChanged);
	}
	if (TakeSubsystem.IsValid())
	{
		TakeSubsystem->OnTakePhaseChanged.AddDynamic(this, &UDCPersonaPanelWidget::HandleTakePhaseChanged);
	}

	RefreshPanel();
}

void UDCPersonaPanelWidget::RefreshPanel()
{
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}

	if (!PersonaComponent.IsValid() && BoundPawn.IsValid())
	{
		PersonaComponent = BoundPawn->FindComponentByClass<UDCPersonaComponent>();
	}
	if (!TakeSubsystem.IsValid() && GetWorld())
	{
		TakeSubsystem = GetWorld()->GetSubsystem<UDCTakeSubsystem>();
	}

	const UDCLevelConfigData* Config = TakeSubsystem.IsValid() ? TakeSubsystem->GetActiveConfig() : nullptr;
	const bool bCanEditNow = PersonaComponent.IsValid() && PersonaComponent->CanEditNow();
	const bool bIsPreTake = !TakeSubsystem.IsValid() || TakeSubsystem->IsPreTake();

	if (BackdropBorder)
	{
		BackdropBorder->SetBrushColor(bIsPreTake ? DCTheme::Desk() : DCTheme::DimScrim());
	}

	if (RoleTitleText)
	{
		const FText HeaderTitle = PersonaComponent.IsValid()
			? ResolveTextOrFallback(PersonaComponent->GetPageHeaderTitle(), TEXT("PERSONA"))
			: FText::FromString(TEXT("PERSONA"));
		RoleTitleText->SetText(HeaderTitle);
	}

	if (PhaseBadgeText)
	{
		FText BadgeTitle = PersonaComponent.IsValid() ? PersonaComponent->GetLevelBadgeTitle() : FText::GetEmpty();
		if (BadgeTitle.IsEmpty() && Config)
		{
			BadgeTitle = Config->LevelDisplayName;
		}
		PhaseBadgeText->SetText(ResolveTextOrFallback(BadgeTitle, TEXT("Fire Call")));
	}

	if (ReadOnlyBadgeBorder)
	{
		ReadOnlyBadgeBorder->SetVisibility(!bIsPreTake ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (DirectorsCutScissorsIcon)
	{
		UTexture2D* ScissorsTexForTape = Config ? Config->ScissorsCursorTexture.Get() : nullptr;
		DirectorsCutScissorsIcon->SetTextureOverride(ScissorsTexForTape);
	}

	if (SubtitleText)
	{
		if (bIsPreTake)
		{
			SubtitleText->SetText(FText::FromString(TEXT("Click yellow lines to snip memories before ACTION! (Careful—some memories help!)")));
			SubtitleText->SetColorAndOpacity(FSlateColor(DCTheme::InkMuted()));
		}
		else
		{
			SubtitleText->SetText(FText::FromString(TEXT("READ-ONLY PEEK — Press [Tab] to Resume")));
			SubtitleText->SetColorAndOpacity(FSlateColor(DCTheme::Ink()));
		}
	}

	if (AngelDogStickerImage)
	{
		DCTheme::ApplyOptionalStickerImage(AngelDogStickerImage.Get(), true);
	}

	if (LegendFixedLabelText && Config)
	{
		LegendFixedLabelText->SetText(ResolveTextOrFallback(Config->LegendFixedLabel, TEXT("FIXED")));
	}
	if (LegendCuttableLabelText && Config)
	{
		LegendCuttableLabelText->SetText(ResolveTextOrFallback(Config->LegendCuttableLabel, TEXT("CUTTABLE")));
	}
	if (LegendCutLabelText && Config)
	{
		LegendCutLabelText->SetText(ResolveTextOrFallback(Config->LegendCutLabel, TEXT("CUT")));
	}
	if (StartTakeButtonLabel && Config)
	{
		StartTakeButtonLabel->SetText(ResolveTextOrFallback(Config->ActionButtonLabel, TEXT("ACTION!")));
	}

	if (ResetButtonLipContainer)
	{
		ResetButtonLipContainer->SetVisibility(bCanEditNow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (RestartTakeButton)
	{
		RestartTakeButton->SetIsEnabled(bCanEditNow);
		RestartTakeButton->SetVisibility(bCanEditNow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (ActionButtonLipContainer)
	{
		ActionButtonLipContainer->SetVisibility(bCanEditNow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (StartTakeButton)
	{
		StartTakeButton->SetIsEnabled(bCanEditNow);
		StartTakeButton->SetVisibility(bCanEditNow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (FooterRuleText)
	{
		FooterRuleText->SetVisibility(ESlateVisibility::Collapsed);
	}

	int32 PresentCount = 0;
	int32 TotalEnabledCount = 0;
	TSet<FName> CurrentPresentIDs;
	bool bAnyBlockJustCut = false;

	if (BlocksContainer && PersonaComponent.IsValid() && WidgetTree)
	{
		BlocksContainer->ClearChildren();

		UTexture2D* ScissorsTex = Config ? Config->ScissorsCursorTexture.Get() : nullptr;
		constexpr float MarkerSwipeSeconds = 0.24f;

		UClass* RowClass = BlockRowClass ? BlockRowClass.Get() : UDCBlockCutButton::StaticClass();
		for (const FDCPersonaBlock& Block : PersonaComponent->GetBlocks())
		{
			if (!Block.bEnabledInLevel)
			{
				continue;
			}

			++TotalEnabledCount;
			if (Block.bIsPresent)
			{
				++PresentCount;
				CurrentPresentIDs.Add(Block.BlockID);
			}

			const bool bJustCut = (!Block.bIsPresent)
				&& ((PendingJustCutBlockID == Block.BlockID) || PreviouslyPresentBlockIDs.Contains(Block.BlockID));
			if (bJustCut)
			{
				bAnyBlockJustCut = true;
			}

			UDCBlockCutButton* RowWidget = WidgetTree->ConstructWidget<UDCBlockCutButton>(RowClass);
			if (RowWidget)
			{
				RowWidget->OnBlockCutTriggered.AddDynamic(this, &UDCPersonaPanelWidget::HandleBlockCutTriggered);
				RowWidget->SetupBlockButton(Block, bCanEditNow, PersonaComponent.Get(), ScissorsTex, MarkerSwipeSeconds, bJustCut);

				if (UVerticalBox* VBox = Cast<UVerticalBox>(BlocksContainer))
				{
					DCTheme::AddVBoxChild(VBox, RowWidget, FMargin(0.0f, 2.0f), HAlign_Fill, VAlign_Top);
				}
				else
				{
					BlocksContainer->AddChild(RowWidget);
				}
			}
		}
	}

	if (bAnyBlockJustCut)
	{
		bPlayingSheetShake = true;
		SheetShakeElapsed = 0.0f;
		UpdateSheetTransform();
		DCTheme::PlayUISound(this, Config ? Config->MarkerSwipeSound.Get() : nullptr);
	}

	PreviouslyPresentBlockIDs = MoveTemp(CurrentPresentIDs);
	PendingJustCutBlockID = NAME_None;

	BP_OnPersonaPanelRefreshed(bCanEditNow, PresentCount, TotalEnabledCount);
}

void UDCPersonaPanelWidget::PlayPaperDropIn()
{
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}

	bPlayingPaperDropIn = true;
	PaperDropInElapsed = 0.0f;
	UpdateSheetTransform();

	if (!TakeSubsystem.IsValid() && GetWorld())
	{
		TakeSubsystem = GetWorld()->GetSubsystem<UDCTakeSubsystem>();
	}
	const UDCLevelConfigData* Config = TakeSubsystem.IsValid() ? TakeSubsystem->GetActiveConfig() : nullptr;
	DCTheme::PlayUISound(this, Config ? Config->PaperOpenSound.Get() : nullptr);
}

void UDCPersonaPanelWidget::UpdateSheetTransform()
{
	if (!JournalSheetBorder)
	{
		return;
	}

	float Scale = 1.0f;
	float OffsetY = 0.0f;
	if (bPlayingPaperDropIn)
	{
		constexpr float DropInDuration = 0.30f;
		const float Alpha = FMath::Clamp(PaperDropInElapsed / DropInDuration, 0.0f, 1.0f);
		Scale = DCTheme::EvaluateOvershootScale(Alpha, 0.95f, 1.015f, 1.0f, 0.65f);
		const float EaseOutY = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f);
		OffsetY = FMath::Lerp(-10.0f, 0.0f, EaseOutY);
	}

	float OffsetX = 0.0f;
	if (bPlayingSheetShake)
	{
		constexpr float ShakeDuration = 0.20f;
		const float Alpha = FMath::Clamp(SheetShakeElapsed / ShakeDuration, 0.0f, 1.0f);
		const float Decay = 1.0f - Alpha;
		const float Wave = FMath::Sin(Alpha * UE_TWO_PI * 3.0f) * Decay;
		OffsetX = Wave * 3.5f;
	}

	JournalSheetBorder->SetRenderScale(FVector2D(Scale, Scale));
	JournalSheetBorder->SetRenderTranslation(FVector2D(OffsetX, OffsetY));
}

void UDCPersonaPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	bool bTransformDirty = false;
	if (bPlayingPaperDropIn)
	{
		constexpr float DropInDuration = 0.30f;
		PaperDropInElapsed += InDeltaTime;
		if (PaperDropInElapsed >= DropInDuration)
		{
			bPlayingPaperDropIn = false;
			PaperDropInElapsed = DropInDuration;
		}
		bTransformDirty = true;
	}

	if (bPlayingSheetShake)
	{
		constexpr float ShakeDuration = 0.20f;
		SheetShakeElapsed += InDeltaTime;
		if (SheetShakeElapsed >= ShakeDuration)
		{
			bPlayingSheetShake = false;
			SheetShakeElapsed = ShakeDuration;
		}
		bTransformDirty = true;
	}

	if (bTransformDirty)
	{
		UpdateSheetTransform();
	}
}

void UDCPersonaPanelWidget::HandlePersonaChanged()
{
	RefreshPanel();
}

void UDCPersonaPanelWidget::HandleBlockCutTriggered(FName CutBlockID)
{
	PendingJustCutBlockID = CutBlockID;
}

void UDCPersonaPanelWidget::HandleTakePhaseChanged(EDCTakePhase NewPhase)
{
	RefreshPanel();
}

void UDCPersonaPanelWidget::HandleStartTakeClicked()
{
	if (!TakeSubsystem.IsValid() && GetWorld())
	{
		TakeSubsystem = GetWorld()->GetSubsystem<UDCTakeSubsystem>();
	}
	if (TakeSubsystem.IsValid())
	{
		if (TakeSubsystem->IsPreTake())
		{
			const UDCLevelConfigData* Config = TakeSubsystem->GetActiveConfig();
			DCTheme::PlayUISound(this, Config ? Config->ActionStartSound.Get() : nullptr);
		}
		TakeSubsystem->StartTake();
	}
}

void UDCPersonaPanelWidget::HandleRestartTakeClicked()
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
