#include "UI/DCTakeResultWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/DCTakeSubsystem.h"
#include "Data/DCLevelConfigData.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Persona/DCPersonaComponent.h"
#include "UI/DCWidgetTheme.h"

namespace
{
	FText ResolveResultText(const FText& Candidate, const FString& Fallback)
	{
		return Candidate.IsEmpty() ? FText::FromString(Fallback) : Candidate;
	}

	void BuildFactRow(
		UWidgetTree* Tree,
		UVerticalBox* ParentBox,
		const FString& RowLabel,
		const FString& InitialValue,
		const FMargin& RowPadding,
		TObjectPtr<UTextBlock>& OutRowLabelText,
		TObjectPtr<UBorder>& OutValuePillBorder,
		TObjectPtr<UTextBlock>& OutValueText)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>();
		DCTheme::AddVBoxChild(ParentBox, Row, RowPadding, HAlign_Fill, VAlign_Center);

		OutRowLabelText = DCTheme::MakeText(Tree, RowLabel, 17, DCTheme::InkMuted(), true, false);
		DCTheme::AddHBoxChild(Row, OutRowLabelText.Get(), FMargin(0.0f), HAlign_Left, VAlign_Center);

		USpacer* RowPush = Tree->ConstructWidget<USpacer>();
		DCTheme::AddHBoxChild(Row, RowPush, FMargin(0.0f), HAlign_Fill, VAlign_Center, ESlateSizeRule::Fill, 1.0f);

		UTextBlock* BuiltValueText = nullptr;
		OutValuePillBorder = DCTheme::MakeBadgePill(
			Tree,
			InitialValue,
			DCTheme::Highlighter(),
			DCTheme::Ink(),
			16,
			DCTheme::Ink(),
			2.5f,
			FMargin(16.0f, 5.0f),
			&BuiltValueText);
		OutValueText = BuiltValueText;
		DCTheme::AddHBoxChild(Row, OutValuePillBorder.Get(), FMargin(16.0f, 0.0f, 0.0f, 0.0f), HAlign_Right, VAlign_Center);
	}
}

UDCTakeResultWidget::UDCTakeResultWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDCTakeResultWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}
}

TSharedRef<SWidget> UDCTakeResultWidget::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}
	return Super::RebuildWidget();
}

void UDCTakeResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RestartButton && !bBindingsWired)
	{
		bBindingsWired = true;
		RestartButton->OnClicked.AddDynamic(this, &UDCTakeResultWidget::HandleRestartClicked);
	}

	RefreshResult();
}

void UDCTakeResultWidget::NativeDestruct()
{
	UnbindSubsystem();
	if (RestartButton && bBindingsWired)
	{
		RestartButton->OnClicked.RemoveDynamic(this, &UDCTakeResultWidget::HandleRestartClicked);
		bBindingsWired = false;
	}
	Super::NativeDestruct();
}

void UDCTakeResultWidget::EnsureDefaultLayout()
{
	if (bDefaultLayoutBuilt || !WidgetTree)
	{
		return;
	}
	bDefaultLayoutBuilt = true;

	// Full-screen dim backdrop (#2B2724 @ 58%)
	DimScrimBorder = WidgetTree->ConstructWidget<UBorder>();
	DimScrimBorder->SetBrushColor(DCTheme::DimScrim());
	DimScrimBorder->SetHorizontalAlignment(HAlign_Center);
	DimScrimBorder->SetVerticalAlignment(VAlign_Center);
	DimScrimBorder->SetPadding(FMargin(32.0f));
	WidgetTree->RootWidget = DimScrimBorder;

	USizeBox* CardWidthFrame = WidgetTree->ConstructWidget<USizeBox>();
	CardWidthFrame->SetMinDesiredWidth(580.0f);
	CardWidthFrame->SetMaxDesiredWidth(640.0f);
	DimScrimBorder->SetContent(CardWidthFrame);

	ResultCardOverlay = WidgetTree->ConstructWidget<UOverlay>();
	ResultCardOverlay->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	CardWidthFrame->SetContent(ResultCardOverlay);

	UVerticalBox* CardStack = WidgetTree->ConstructWidget<UVerticalBox>();
	UBorder* CardSheet = DCTheme::MakeJournalBorder(
		WidgetTree,
		CardStack,
		DCTheme::Paper(),
		28.0f,
		DCTheme::Ink(),
		4.0f,
		FMargin(48.0f, 44.0f, 48.0f, 36.0f));

	UOverlaySlot* CardLayer = ResultCardOverlay->AddChildToOverlay(CardSheet);
	if (CardLayer)
	{
		CardLayer->SetHorizontalAlignment(HAlign_Fill);
		CardLayer->SetVerticalAlignment(VAlign_Fill);
	}

	// Two angled cream corner tapes at top corners
	USizeBox* LeftTape = DCTheme::MakeCornerTape(WidgetTree, DCTheme::TapeCream(), -24.0f, 128.0f, 38.0f);
	LeftTape->SetRenderTranslation(FVector2D(-22.0f, -14.0f));
	if (UOverlaySlot* LeftTapeSlot = ResultCardOverlay->AddChildToOverlay(LeftTape))
	{
		LeftTapeSlot->SetHorizontalAlignment(HAlign_Left);
		LeftTapeSlot->SetVerticalAlignment(VAlign_Top);
	}

	USizeBox* RightTape = DCTheme::MakeCornerTape(WidgetTree, DCTheme::TapeCream(), +24.0f, 128.0f, 38.0f);
	RightTape->SetRenderTranslation(FVector2D(22.0f, -14.0f));
	if (UOverlaySlot* RightTapeSlot = ResultCardOverlay->AddChildToOverlay(RightTape))
	{
		RightTapeSlot->SetHorizontalAlignment(HAlign_Right);
		RightTapeSlot->SetVerticalAlignment(VAlign_Top);
	}

	// Row 1: Large journal stars row (preserving burnt cracked remnants) + optional cheering Angel Pup sticker on 3-star clear
	UHorizontalBox* StarsHeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	DCTheme::AddVBoxChild(CardStack, StarsHeaderRow, FMargin(0.0f, 4.0f, 0.0f, 16.0f), HAlign_Center, VAlign_Center);

	StarsGlyphRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	DCTheme::AddHBoxChild(StarsHeaderRow, StarsGlyphRow, FMargin(0.0f), HAlign_Center, VAlign_Center);

	PerfectStickerImage = WidgetTree->ConstructWidget<UImage>();
	PerfectStickerImage->SetDesiredSizeOverride(FVector2D(72.0f, 72.0f));
	PerfectStickerImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	PerfectStickerImage->SetRenderTransformAngle(9.0f);
	PerfectStickerImage->SetVisibility(ESlateVisibility::Collapsed);
	DCTheme::AddHBoxChild(StarsHeaderRow, PerfectStickerImage, FMargin(14.0f, 0.0f, 0.0f, 0.0f), HAlign_Center, VAlign_Center);

	// Top dashed divider
	UDCDashedRuleWidget* TopRule = WidgetTree->ConstructWidget<UDCDashedRuleWidget>();
	TopRule->ConfigureRule(DCTheme::DashedRule(), 3.0f, 9.0f, 12.0f);
	DCTheme::AddVBoxChild(CardStack, TopRule, FMargin(0.0f, 0.0f, 0.0f, 12.0f), HAlign_Fill, VAlign_Center);

	// Row 2..4: Objective fact rows (FIRES / CIVILIAN / STARS)
	BuildFactRow(
		WidgetTree,
		CardStack,
		TEXT("FIRES"),
		TEXT("ALL OUT"),
		FMargin(0.0f, 4.0f, 0.0f, 10.0f),
		FiresRowLabelText,
		FiresValuePillBorder,
		FiresValueText);

	BuildFactRow(
		WidgetTree,
		CardStack,
		TEXT("CIVILIAN"),
		TEXT("SAVED UNINJURED"),
		FMargin(0.0f, 4.0f, 0.0f, 10.0f),
		CivilianRowLabelText,
		CivilianValuePillBorder,
		CivilianValueText);

	BuildFactRow(
		WidgetTree,
		CardStack,
		TEXT("STARS"),
		TEXT("0 / 3"),
		FMargin(0.0f, 4.0f, 0.0f, 14.0f),
		StarsRowLabelText,
		StarsValuePillBorder,
		StarsSummaryText);

	// Bottom dashed divider before Director's Note + [R] Retake
	UDCDashedRuleWidget* BottomRule = WidgetTree->ConstructWidget<UDCDashedRuleWidget>();
	BottomRule->ConfigureRule(DCTheme::DashedRule(), 3.0f, 9.0f, 12.0f);
	DCTheme::AddVBoxChild(CardStack, BottomRule, FMargin(0.0f, 0.0f, 0.0f, 14.0f), HAlign_Fill, VAlign_Center);

	// Warm cream-bordered Director's Note / Review Quip box right above [R] Retake (zero buttons inside)
	USizeBox* ReviewNoteWidthBox = WidgetTree->ConstructWidget<USizeBox>();
	ReviewNoteWidthBox->SetMaxDesiredWidth(480.0f);

	ReviewNoteText = DCTheme::MakeText(WidgetTree, TEXT(""), 15, DCTheme::Ink(), false, true);
	ReviewNoteText->SetJustification(ETextJustify::Center);
	ReviewNoteText->SetWrapTextAt(444.0f);

	ReviewNoteBorder = DCTheme::MakeJournalBorder(
		WidgetTree,
		ReviewNoteText,
		DCTheme::ReviewNoteFill(),
		14.0f,
		DCTheme::ReviewNoteBorder(),
		2.5f,
		FMargin(18.0f, 11.0f));
	ReviewNoteWidthBox->SetContent(ReviewNoteBorder);
	DCTheme::AddVBoxChild(CardStack, ReviewNoteWidthBox, FMargin(0.0f, 0.0f, 0.0f, 18.0f), HAlign_Center, VAlign_Center);

	// Centered chubby Butter [R] Retake pill button
	RestartButton = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle RetakeStyle;
	RetakeStyle.Normal = DCTheme::MakeRoundedBoxBrush(DCTheme::Butter(), 999.0f, DCTheme::Ink(), 4.0f);
	RetakeStyle.Hovered = DCTheme::MakeRoundedBoxBrush(DCTheme::Blush(), 999.0f, DCTheme::Ink(), 4.0f);
	RetakeStyle.Pressed = DCTheme::MakeRoundedBoxBrush(DCTheme::Blush(), 999.0f, DCTheme::Ink(), 4.0f);
	RetakeStyle.Disabled = DCTheme::MakeRoundedBoxBrush(DCTheme::Paper(), 999.0f, DCTheme::CutTextMuted(), 4.0f);
	RetakeStyle.NormalPadding = FMargin(0.0f);
	RetakeStyle.PressedPadding = FMargin(0.0f, 2.0f, 0.0f, 0.0f);
	RestartButton->SetStyle(RetakeStyle);

	UHorizontalBox* RetakeContentRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	UBorder* Keycap = DCTheme::MakeChubbyKeycap(WidgetTree, TEXT("R"), 30.0f, 15, 8.0f, 2.5f);
	DCTheme::AddHBoxChild(RetakeContentRow, Keycap, FMargin(0.0f, 0.0f, 12.0f, 0.0f), HAlign_Left, VAlign_Center);

	RetakeButtonText = DCTheme::MakeText(WidgetTree, TEXT("Retake"), 22, DCTheme::Ink(), true, false);
	DCTheme::AddHBoxChild(RetakeContentRow, RetakeButtonText, FMargin(0.0f), HAlign_Left, VAlign_Center);

	UBorder* RetakePadding = WidgetTree->ConstructWidget<UBorder>();
	RetakePadding->SetBrushColor(FLinearColor::Transparent);
	RetakePadding->SetPadding(FMargin(28.0f, 10.0f));
	RetakePadding->SetHorizontalAlignment(HAlign_Center);
	RetakePadding->SetVerticalAlignment(VAlign_Center);
	RetakePadding->SetContent(RetakeContentRow);
	RestartButton->SetContent(RetakePadding);

	UBorder* RetakeLip = DCTheme::MakeJournalBorder(
		WidgetTree,
		RestartButton,
		DCTheme::ButterShadow(),
		999.0f,
		DCTheme::Ink(),
		4.0f,
		FMargin(0.0f, 0.0f, 0.0f, 5.0f));

	DCTheme::AddVBoxChild(CardStack, RetakeLip, FMargin(0.0f), HAlign_Center, VAlign_Center);
}

void UDCTakeResultWidget::UnbindSubsystem()
{
	if (BoundSubsystem.IsValid())
	{
		BoundSubsystem->OnTakeFinished.RemoveDynamic(this, &UDCTakeResultWidget::HandleTakeFinished);
		BoundSubsystem->OnTakeReset.RemoveDynamic(this, &UDCTakeResultWidget::HandleTakeReset);
		BoundSubsystem.Reset();
	}
}

void UDCTakeResultWidget::BindToTakeSubsystem(UDCTakeSubsystem* InSubsystem)
{
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}

	UnbindSubsystem();
	BoundSubsystem = InSubsystem;
	if (!InSubsystem)
	{
		return;
	}

	InSubsystem->OnTakeFinished.AddDynamic(this, &UDCTakeResultWidget::HandleTakeFinished);
	InSubsystem->OnTakeReset.AddDynamic(this, &UDCTakeResultWidget::HandleTakeReset);
	RefreshResult();
}

void UDCTakeResultWidget::RefreshResult()
{
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}

	if (!BoundSubsystem.IsValid() && GetWorld())
	{
		BoundSubsystem = GetWorld()->GetSubsystem<UDCTakeSubsystem>();
	}
	if (!BoundSubsystem.IsValid())
	{
		return;
	}

	if (BoundSubsystem->IsTakeResult())
	{
		if (!bHasPoppedForCurrentResult)
		{
			bHasPoppedForCurrentResult = true;
			PopDuration = 0.30f;
			bPlayingPop = true;
			PopElapsed = 0.0f;
			if (ResultCardOverlay)
			{
				ResultCardOverlay->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
				ResultCardOverlay->SetRenderScale(FVector2D(0.90f, 0.90f));
			}
		}
	}
	else
	{
		bHasPoppedForCurrentResult = false;
		bPlayingPop = false;
		PopElapsed = 0.0f;
		if (ResultCardOverlay)
		{
			ResultCardOverlay->SetRenderScale(FVector2D(1.0f, 1.0f));
		}
	}

	const UDCLevelConfigData* Config = BoundSubsystem->GetActiveConfig();
	const int32 InitialContainers = Config ? FMath::Max(1, Config->InitialStarContainers) : 3;
	const int32 MaxContainers = FMath::Clamp(BoundSubsystem->GetMaxStarContainers(), 0, InitialContainers);
	const int32 Earned = FMath::Clamp(BoundSubsystem->GetEarnedStars(), 0, MaxContainers);
	const EDCTakeEndReason EndReason = BoundSubsystem->GetLastEndReason();
	const EDCCivilianState CivState = BoundSubsystem->GetCivilianState();
	const int32 RemainingFires = BoundSubsystem->GetRemainingFireSpotCount();

	if (StarsGlyphRow && WidgetTree)
	{
		StarsGlyphRow->ClearChildren();
		StarWidgets.Reset();

		UTexture2D* EmptyTex = Config ? Config->StarEmptyTexture.Get() : nullptr;
		UTexture2D* FilledTex = Config ? Config->StarFilledTexture.Get() : nullptr;
		UTexture2D* BurntTex = Config ? Config->StarBurntTexture.Get() : nullptr;

		for (int32 Index = 0; Index < InitialContainers; ++Index)
		{
			UDCJournalStarWidget* Star = WidgetTree->ConstructWidget<UDCJournalStarWidget>();
			if (!Star)
			{
				continue;
			}

			EDCJournalStarState State = EDCJournalStarState::Empty;
			float StarOpacity = 1.0f;
			if (Index < Earned)
			{
				State = EDCJournalStarState::Filled;
			}
			else if (Index >= MaxContainers)
			{
				State = EDCJournalStarState::Burnt;
				StarOpacity = 0.68f;
			}

			Star->SetStarDiameter(96.0f);
			Star->SetStarState(State);
			Star->SetVisualOpacity(StarOpacity);
			Star->SetBurntProgress(1.0f);
			Star->SetTextureOverrides(EmptyTex, FilledTex, BurntTex);

			StarWidgets.Add(Star);
			DCTheme::AddHBoxChild(
				StarsGlyphRow,
				Star,
				FMargin(Index > 0 ? 18.0f : 0.0f, 0.0f, 0.0f, 0.0f),
				HAlign_Center,
				VAlign_Center);
		}
	}

	if (FiresRowLabelText)
	{
		const FText FiresLabel = Config
			? ResolveResultText(Config->ResultFiresRowLabel, TEXT("FIRES"))
			: FText::FromString(TEXT("FIRES"));
		FiresRowLabelText->SetText(FiresLabel);
	}
	if (FiresValueText)
	{
		FText FiresValue;
		if (RemainingFires <= 0)
		{
			FiresValue = Config
				? ResolveResultText(Config->ResultFiresAllOutValue, TEXT("ALL OUT"))
				: FText::FromString(TEXT("ALL OUT"));
		}
		else
		{
			const FText FormatText = Config
				? ResolveResultText(Config->ResultFiresRemainingFormat, TEXT("{0} REMAINING"))
				: FText::FromString(TEXT("{0} REMAINING"));
			FiresValue = FText::Format(FormatText, FText::AsNumber(RemainingFires));
		}
		FiresValueText->SetText(FiresValue);
	}
	if (FiresValuePillBorder)
	{
		const FLinearColor FiresFill = (RemainingFires <= 0) ? DCTheme::TapeBlue() : DCTheme::CalmResultTag();
		DCTheme::ApplyRoundedBorderStyle(
			FiresValuePillBorder, FiresFill, 999.0f, DCTheme::Ink(), 2.5f, FMargin(16.0f, 5.0f));
	}

	const bool bCivilianSaved = (CivState == EDCCivilianState::RescuedUninjured || CivState == EDCCivilianState::RescuedInjured);
	if (CivilianRowLabelText)
	{
		const FText CivLabel = Config
			? ResolveResultText(Config->ResultCivilianRowLabel, TEXT("CIVILIAN"))
			: FText::FromString(TEXT("CIVILIAN"));
		CivilianRowLabelText->SetText(CivLabel);
	}
	if (CivilianValueText)
	{
		FText CivValue;
		if (CivState == EDCCivilianState::RescuedUninjured)
		{
			CivValue = Config
				? ResolveResultText(Config->ResultCivilianUninjuredValue, TEXT("SAVED UNINJURED"))
				: FText::FromString(TEXT("SAVED UNINJURED"));
		}
		else if (CivState == EDCCivilianState::RescuedInjured)
		{
			CivValue = Config
				? ResolveResultText(Config->ResultCivilianInjuredValue, TEXT("SAVED INJURED"))
				: FText::FromString(TEXT("SAVED INJURED"));
		}
		else
		{
			CivValue = Config
				? ResolveResultText(Config->ResultCivilianLostValue, TEXT("LOST IN FIRE"))
				: FText::FromString(TEXT("LOST IN FIRE"));
		}
		CivilianValueText->SetText(CivValue);
	}
	if (CivilianValuePillBorder)
	{
		const FLinearColor CivFill = (CivState == EDCCivilianState::RescuedUninjured)
			? DCTheme::Highlighter()
			: (bCivilianSaved ? DCTheme::TapeCream() : DCTheme::CalmResultTag());
		DCTheme::ApplyRoundedBorderStyle(
			CivilianValuePillBorder, CivFill, 999.0f, DCTheme::Ink(), 2.5f, FMargin(16.0f, 5.0f));
	}

	if (StarsSummaryText)
	{
		StarsSummaryText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Earned, InitialContainers)));
	}
	if (StarsValuePillBorder)
	{
		const FLinearColor StarsFill = (Earned > 0) ? DCTheme::Butter() : DCTheme::Paper();
		DCTheme::ApplyRoundedBorderStyle(
			StarsValuePillBorder, StarsFill, 999.0f, DCTheme::Ink(), 2.5f, FMargin(16.0f, 5.0f));
	}

	if (RetakeButtonText && Config)
	{
		RetakeButtonText->SetText(ResolveResultText(Config->RetakeHintLabel, TEXT("Retake")));
	}

	const FString ReviewNote = BuildReviewNoteText(Earned, MaxContainers, CivState, RemainingFires);
	if (ReviewNoteText)
	{
		ReviewNoteText->SetText(FText::FromString(ReviewNote));
	}
	if (ReviewNoteBorder)
	{
		ReviewNoteBorder->SetVisibility(ReviewNote.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	if (PerfectStickerImage)
	{
		DCTheme::ApplyOptionalStickerImage(PerfectStickerImage.Get(), Earned == 3 && MaxContainers == 3);
	}

	// Optional BindWidgetOptional hooks for custom Blueprint Designer overrides (zero clapperboard strings)
	if (HeaderTitleText)
	{
		const FText SceneName = Config
			? ResolveResultText(Config->LevelDisplayName, TEXT("Fire Call"))
			: FText::FromString(TEXT("Fire Call"));
		HeaderTitleText->SetText(SceneName);
	}
	if (TakeMetaText)
	{
		TakeMetaText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Earned, MaxContainers)));
	}
	if (StarsGlyphText)
	{
		StarsGlyphText->SetText(FText::FromString(DCTheme::BuildStarGlyphText(Earned, MaxContainers)));
	}
	if (EndReasonText && FiresValueText)
	{
		EndReasonText->SetText(FiresValueText->GetText());
	}
	if (OutcomeSummaryText && CivilianValueText)
	{
		OutcomeSummaryText->SetText(CivilianValueText->GetText());
	}

	BP_OnResultUpdated(EndReason, Earned, MaxContainers);
}

UDCPersonaComponent* UDCTakeResultWidget::FindPlayerPersonaComponent() const
{
	if (const APawn* OwningPawn = GetOwningPlayerPawn())
	{
		if (UDCPersonaComponent* Found = OwningPawn->FindComponentByClass<UDCPersonaComponent>())
		{
			return Found;
		}
	}

	if (const UWorld* World = GetWorld())
	{
		if (const APlayerController* PC = World->GetFirstPlayerController())
		{
			if (const APawn* Pawn = PC->GetPawn())
			{
				return Pawn->FindComponentByClass<UDCPersonaComponent>();
			}
		}
	}

	return nullptr;
}

FString UDCTakeResultWidget::BuildReviewNoteText(
	int32 Earned,
	int32 MaxContainers,
	EDCCivilianState CivState,
	int32 RemainingFires) const
{
	if (Earned == 3 && MaxContainers == 3)
	{
		return TEXT("PERFECT DIRECTOR'S CUT! One flip-flop, zero burns, all flames out!");
	}

	const UDCPersonaComponent* PersonaComp = FindPlayerPersonaComponent();
	const bool bWitchTraumaPresent = PersonaComp && PersonaComp->IsBlockPresent(FName(TEXT("WitchCaptainTrauma")));
	if (Earned == 0 && bWitchTraumaPresent)
	{
		return TEXT("Chief Witch's red heels won. Pipo stayed frog-free on the porch.");
	}

	if (CivState == EDCCivilianState::Dead && RemainingFires <= 0)
	{
		return TEXT("Fires out! Sadly, heart-boxers guy screamed his last scream.");
	}

	if (CivState == EDCCivilianState::RescuedInjured && RemainingFires <= 0)
	{
		return TEXT("Textbook Rule #1! Too bad smoke doesn't wait for protocol...");
	}

	const bool bCivilianRescued = (CivState == EDCCivilianState::RescuedUninjured)
		|| (CivState == EDCCivilianState::RescuedInjured);
	if (bCivilianRescued && RemainingFires > 0 && PersonaComp)
	{
		if (!PersonaComp->IsBlockPresent(FName(TEXT("ExtinguisherTraining"))))
		{
			return TEXT("Heart-boxers guy is safe! Shame Pipo thought the extinguisher was roast-chicken ketchup.");
		}
		if (!PersonaComp->IsBlockPresent(FName(TEXT("AngelPupWorkout"))))
		{
			return TEXT("How could you snip out Angel Pup Coco?! Skipping leg day cost the room!");
		}
	}

	return TEXT("Chief Witch's red heels won, or the flames took over—snip sharper and [R] Retake!");
}

void UDCTakeResultWidget::HandleTakeFinished(EDCTakeEndReason EndReason, int32 FinalEarnedStars, int32 FinalMaxContainers)
{
	RefreshResult();
}

void UDCTakeResultWidget::HandleTakeReset()
{
	bHasPoppedForCurrentResult = false;
	bPlayingPop = false;
	PopElapsed = 0.0f;
	if (ResultCardOverlay)
	{
		ResultCardOverlay->SetRenderScale(FVector2D(1.0f, 1.0f));
	}
}

void UDCTakeResultWidget::HandleRestartClicked()
{
	if (!BoundSubsystem.IsValid() && GetWorld())
	{
		BoundSubsystem = GetWorld()->GetSubsystem<UDCTakeSubsystem>();
	}
	if (BoundSubsystem.IsValid())
	{
		BoundSubsystem->RestartTake();
	}
}

void UDCTakeResultWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bPlayingPop && ResultCardOverlay)
	{
		PopElapsed += InDeltaTime;
		const float Alpha = FMath::Clamp(PopElapsed / FMath::Max(0.05f, PopDuration), 0.0f, 1.0f);
		const float Scale = DCTheme::EvaluateOvershootScale(Alpha, 0.90f, 1.03f, 1.0f, 0.65f);

		ResultCardOverlay->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		ResultCardOverlay->SetRenderScale(FVector2D(Scale, Scale));
		if (Alpha >= 1.0f)
		{
			bPlayingPop = false;
			ResultCardOverlay->SetRenderScale(FVector2D(1.0f, 1.0f));
		}
	}
}
