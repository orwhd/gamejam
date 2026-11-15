#include "UI/DCInteractPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "UI/DCWidgetTheme.h"

UDCInteractPromptWidget::UDCInteractPromptWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDCInteractPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}
}

TSharedRef<SWidget> UDCInteractPromptWidget::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}
	return Super::RebuildWidget();
}

void UDCInteractPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (KeycapText)
	{
		KeycapText->SetText(FText::FromString(TEXT("E")));
	}
	if (PromptLabelText)
	{
		PromptLabelText->SetText(CurrentLabelText);
	}
	if (PromptPillBorder)
	{
		PromptPillBorder->SetVisibility(CurrentLabelText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UDCInteractPromptWidget::EnsureDefaultLayout()
{
	if (bDefaultLayoutBuilt || !WidgetTree)
	{
		return;
	}
	bDefaultLayoutBuilt = true;

	UBorder* Root = WidgetTree->ConstructWidget<UBorder>();
	Root->SetBrushColor(FLinearColor::Transparent);
	Root->SetPadding(FMargin(10.0f, 8.0f));
	WidgetTree->RootWidget = Root;

	BounceContainer = WidgetTree->ConstructWidget<UBorder>();
	BounceContainer->SetBrushColor(FLinearColor::Transparent);
	Root->SetContent(BounceContainer);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	BounceContainer->SetContent(Row);

	// Pad around the [E] keycap so the concentric hold ring never clips against widget edges.
	USizeBox* RingHostBox = WidgetTree->ConstructWidget<USizeBox>();
	RingHostBox->SetMinDesiredWidth(98.0f);
	RingHostBox->SetMinDesiredHeight(98.0f);

	UBorder* CenterCell = WidgetTree->ConstructWidget<UBorder>();
	CenterCell->SetBrushColor(FLinearColor::Transparent);
	CenterCell->SetHorizontalAlignment(HAlign_Center);
	CenterCell->SetVerticalAlignment(VAlign_Center);

	UTextBlock* BuiltKeyText = nullptr;
	KeycapBorder = DCTheme::MakeChubbyKeycap(WidgetTree, TEXT("E"), 40.0f, 20, 11.0f, 3.5f, &BuiltKeyText);
	KeycapText = BuiltKeyText;
	RingAnchorWidget = KeycapBorder;

	CenterCell->SetContent(KeycapBorder);
	RingHostBox->SetContent(CenterCell);
	DCTheme::AddHBoxChild(Row, RingHostBox, FMargin(0.0f), HAlign_Center, VAlign_Center);

	UTextBlock* BuiltLabelText = nullptr;
	PromptPillBorder = DCTheme::MakeBadgePill(
		WidgetTree,
		TEXT(""),
		DCTheme::Paper(),
		DCTheme::Ink(),
		18,
		DCTheme::Ink(),
		3.5f,
		FMargin(18.0f, 8.0f),
		&BuiltLabelText);
	PromptLabelText = BuiltLabelText;
	DCTheme::AddHBoxChild(Row, PromptPillBorder, FMargin(8.0f, 0.0f, 0.0f, 0.0f), HAlign_Left, VAlign_Center);
}

void UDCInteractPromptWidget::SetPromptState(const FText& LabelText, float HoldProgress01, bool bIsHoldMode, bool bIsSpraying)
{
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}

	CurrentLabelText = LabelText;
	CurrentHoldProgress = FMath::Clamp(HoldProgress01, 0.0f, 1.0f);
	bHoldModeActive = bIsHoldMode;
	bSprayingActive = bIsSpraying;

	if (PromptLabelText)
	{
		PromptLabelText->SetText(CurrentLabelText);
	}
	if (PromptPillBorder)
	{
		PromptPillBorder->SetVisibility(CurrentLabelText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	Invalidate(EInvalidateWidgetReason::Paint);
	BP_OnPromptStateUpdated(CurrentLabelText, CurrentHoldProgress, bHoldModeActive, bSprayingActive);
}

void UDCInteractPromptWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const float Speed = bSprayingActive ? 6.0f : 3.4f;
	const float Amplitude = bSprayingActive ? 1.8f : 3.2f;
	BounceTime += InDeltaTime * Speed;

	if (BounceContainer)
	{
		const float BobY = FMath::Sin(BounceTime) * Amplitude;
		BounceContainer->SetRenderTranslation(FVector2D(0.0f, BobY));
	}
}

int32 UDCInteractPromptWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	int32 MaxLayer = Super::NativePaint(
		Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (bHoldModeActive && (bSprayingActive || CurrentHoldProgress > KINDA_SMALL_NUMBER))
	{
		FVector2f RingCenter = FVector2f(AllottedGeometry.GetLocalSize()) * 0.5f;
		float RingRadius = 38.0f;

		UWidget* Anchor = RingAnchorWidget ? RingAnchorWidget.Get() : KeycapBorder.Get();
		if (Anchor)
		{
			const FGeometry AnchorGeom = Anchor->GetCachedGeometry();
			const FVector2f AnchorSize = FVector2f(AnchorGeom.GetLocalSize());
			if (AnchorSize.X > 1.0f && AnchorSize.Y > 1.0f)
			{
				const FVector2f AbsCenter = AnchorGeom.LocalToAbsolute(AnchorSize * 0.5f);
				RingCenter = AllottedGeometry.AbsoluteToLocal(AbsCenter);
				const float HalfExtent = FMath::Max(AnchorSize.X, AnchorSize.Y) * 0.5f;
				RingRadius = FMath::Clamp(HalfExtent + 13.0f, 34.0f, 44.0f);
			}
		}

		// Background track: Ink outer border ring + crisp Paper track
		DCTheme::PaintArcStroke(
			OutDrawElements,
			MaxLayer + 1,
			AllottedGeometry,
			RingCenter,
			RingRadius,
			0.0f,
			UE_TWO_PI,
			DCTheme::Ink(),
			13.0f,
			48,
			false);

		DCTheme::PaintArcStroke(
			OutDrawElements,
			MaxLayer + 2,
			AllottedGeometry,
			RingCenter,
			RingRadius,
			0.0f,
			UE_TWO_PI,
			DCTheme::Paper(),
			7.5f,
			48,
			false);

		if (CurrentHoldProgress > KINDA_SMALL_NUMBER)
		{
			const float StartRad = -UE_HALF_PI;
			const float EndRad = StartRad + UE_TWO_PI * CurrentHoldProgress;
			DCTheme::PaintArcStroke(
				OutDrawElements,
				MaxLayer + 3,
				AllottedGeometry,
				RingCenter,
				RingRadius,
				StartRad,
				EndRad,
				DCTheme::Sky(),
				7.5f,
				48,
				true);
		}

		MaxLayer += 3;
	}

	return MaxLayer;
}
