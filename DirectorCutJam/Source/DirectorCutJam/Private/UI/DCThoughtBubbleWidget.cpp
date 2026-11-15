#include "UI/DCThoughtBubbleWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "UI/DCWidgetTheme.h"

UDCThoughtBubbleWidget::UDCThoughtBubbleWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDCThoughtBubbleWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}
}

TSharedRef<SWidget> UDCThoughtBubbleWidget::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}
	return Super::RebuildWidget();
}

void UDCThoughtBubbleWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (AnimRootBorder)
	{
		AnimRootBorder->SetRenderTransformPivot(FVector2D(0.5f, 1.0f));
	}
	else if (BubbleBorder)
	{
		BubbleBorder->SetRenderTransformPivot(FVector2D(0.5f, 1.0f));
	}
}

void UDCThoughtBubbleWidget::EnsureDefaultLayout()
{
	if (bDefaultLayoutBuilt || !WidgetTree)
	{
		return;
	}
	bDefaultLayoutBuilt = true;

	AnimRootBorder = WidgetTree->ConstructWidget<UBorder>();
	AnimRootBorder->SetBrushColor(FLinearColor::Transparent);
	AnimRootBorder->SetPadding(FMargin(8.0f, 8.0f, 8.0f, 4.0f));
	AnimRootBorder->SetRenderTransformPivot(FVector2D(0.5f, 1.0f));
	WidgetTree->RootWidget = AnimRootBorder;

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>();
	AnimRootBorder->SetContent(Stack);

	UVerticalBox* TextColumn = WidgetTree->ConstructWidget<UVerticalBox>();

	ThoughtTextBlock = DCTheme::MakeText(WidgetTree, TEXT(""), 22, DCTheme::Ink(), true, true);
	ThoughtTextBlock->SetJustification(ETextJustify::Center);
	DCTheme::AddVBoxChild(TextColumn, ThoughtTextBlock, FMargin(0.0f), HAlign_Center, VAlign_Center);

	SecondaryThoughtTextBlock = DCTheme::MakeText(WidgetTree, TEXT(""), 17, DCTheme::InkMuted(), false, true);
	SecondaryThoughtTextBlock->SetJustification(ETextJustify::Center);
	SecondaryThoughtTextBlock->SetVisibility(ESlateVisibility::Collapsed);
	DCTheme::AddVBoxChild(TextColumn, SecondaryThoughtTextBlock, FMargin(0.0f, 4.0f, 0.0f, 0.0f), HAlign_Center, VAlign_Center);

	USizeBox* BubbleWidthGuard = WidgetTree->ConstructWidget<USizeBox>();
	BubbleWidthGuard->SetMinDesiredWidth(220.0f);
	BubbleWidthGuard->SetMaxDesiredWidth(460.0f);
	BubbleWidthGuard->SetContent(TextColumn);

	BubbleBorder = DCTheme::MakeJournalBorder(
		WidgetTree,
		BubbleWidthGuard,
		DCTheme::Paper(),
		28.0f,
		DCTheme::Peach(),
		4.5f,
		FMargin(26.0f, 15.0f));

	DCTheme::AddVBoxChild(Stack, BubbleBorder, FMargin(0.0f), HAlign_Center, VAlign_Center);

	// Reserved breathing room below the rounded sheet for the 3-dot thought tail
	TailSpacerBox = WidgetTree->ConstructWidget<USizeBox>();
	TailSpacerBox->SetMinDesiredHeight(42.0f);
	DCTheme::AddVBoxChild(Stack, TailSpacerBox, FMargin(0.0f), HAlign_Center, VAlign_Top);
}

void UDCThoughtBubbleWidget::SetThoughtText(const FText& NewThought)
{
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}

	const bool bHasContent = !NewThought.IsEmpty();
	const bool bWasCollapsed = AnimRootBorder && (AnimRootBorder->GetVisibility() != ESlateVisibility::SelfHitTestInvisible);
	const bool bShouldBounce = bHasContent && (!NewThought.EqualTo(CachedThoughtText) || bWasCollapsed);

	CachedThoughtText = NewThought;

	if (AnimRootBorder)
	{
		AnimRootBorder->SetVisibility(bHasContent ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (ThoughtTextBlock)
	{
		const FString RawString = NewThought.ToString();
		FString PrimaryLine;
		FString SecondaryLine;
		if (SecondaryThoughtTextBlock && RawString.Split(TEXT("\n"), &PrimaryLine, &SecondaryLine))
		{
			PrimaryLine.TrimStartAndEndInline();
			SecondaryLine.TrimStartAndEndInline();

			ThoughtTextBlock->SetText(FText::FromString(PrimaryLine));
			if (!SecondaryLine.IsEmpty())
			{
				SecondaryThoughtTextBlock->SetText(FText::FromString(SecondaryLine));
				SecondaryThoughtTextBlock->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				SecondaryThoughtTextBlock->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		else
		{
			ThoughtTextBlock->SetText(NewThought);
			if (SecondaryThoughtTextBlock)
			{
				SecondaryThoughtTextBlock->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	if (bShouldBounce)
	{
		PopDuration = 0.24f;
		bPlayingPop = true;
		PopElapsed = 0.0f;
		ApplyPopScale(0.75f);
	}
	else if (!bHasContent)
	{
		bPlayingPop = false;
		ApplyPopScale(1.0f);
	}

	Invalidate(EInvalidateWidgetReason::Paint);
	BP_OnThoughtUpdated(NewThought);
}

void UDCThoughtBubbleWidget::ApplyPopScale(float Scale)
{
	UWidget* Target = AnimRootBorder ? static_cast<UWidget*>(AnimRootBorder.Get()) : static_cast<UWidget*>(BubbleBorder.Get());
	if (Target)
	{
		Target->SetRenderTransformPivot(FVector2D(0.5f, 1.0f));
		Target->SetRenderScale(FVector2D(Scale, Scale));
	}
}

void UDCThoughtBubbleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bPlayingPop)
	{
		PopElapsed += InDeltaTime;
		const float Alpha = FMath::Clamp(PopElapsed / FMath::Max(0.05f, PopDuration), 0.0f, 1.0f);
		const float Scale = DCTheme::EvaluateOvershootScale(Alpha, 0.75f, 1.06f, 1.0f, 0.65f);
		ApplyPopScale(Scale);
		Invalidate(EInvalidateWidgetReason::Paint);

		if (Alpha >= 1.0f)
		{
			bPlayingPop = false;
			ApplyPopScale(1.0f);
		}
	}
}

int32 UDCThoughtBubbleWidget::NativePaint(
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

	if (!CachedThoughtText.IsEmpty() && BubbleBorder)
	{
		const FGeometry BubbleGeom = BubbleBorder->GetCachedGeometry();
		const FVector2f BubbleSize = FVector2f(BubbleGeom.GetLocalSize());
		if (BubbleSize.X > 1.0f && BubbleSize.Y > 1.0f)
		{
			const FVector2f BottomCenterAbs = BubbleGeom.LocalToAbsolute(FVector2f(BubbleSize.X * 0.5f, BubbleSize.Y));
			const FVector2f Origin = AllottedGeometry.AbsoluteToLocal(BottomCenterAbs);

			const float TailScale = AnimRootBorder ? static_cast<float>(AnimRootBorder->GetRenderTransform().Scale.X) : 1.0f;
			const float SafeScale = FMath::Clamp(TailScale, 0.5f, 1.2f);

			DCTheme::PaintFilledCircle(
				OutDrawElements,
				MaxLayer + 1,
				AllottedGeometry,
				Origin + FVector2f(-4.0f, 8.0f) * SafeScale,
				9.0f * SafeScale,
				DCTheme::Paper(),
				DCTheme::Peach(),
				3.8f,
				20);

			DCTheme::PaintFilledCircle(
				OutDrawElements,
				MaxLayer + 2,
				AllottedGeometry,
				Origin + FVector2f(-12.0f, 23.0f) * SafeScale,
				6.5f * SafeScale,
				DCTheme::Paper(),
				DCTheme::Peach(),
				3.4f,
				18);

			DCTheme::PaintFilledCircle(
				OutDrawElements,
				MaxLayer + 3,
				AllottedGeometry,
				Origin + FVector2f(-19.0f, 36.0f) * SafeScale,
				4.5f * SafeScale,
				DCTheme::Paper(),
				DCTheme::Peach(),
				3.0f,
				16);

			MaxLayer += 3;
		}
	}

	return MaxLayer;
}
