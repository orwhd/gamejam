#include "UI/DCWidgetTheme.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateImageBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Rendering/DrawElements.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

namespace DCTheme
{
	FLinearColor HexColor(const TCHAR* Hex, float AlphaMultiplier)
	{
		const FColor ParsedSRGB = FColor::FromHex(Hex);
		FLinearColor Result = FLinearColor::FromSRGBColor(ParsedSRGB);
		Result.A *= AlphaMultiplier;
		return Result;
	}

	FLinearColor Paper()             { return HexColor(TEXT("#FFF6E6")); }
	FLinearColor Ink()               { return HexColor(TEXT("#2B2724")); }
	FLinearColor InkMuted()          { return HexColor(TEXT("#2B2724"), 0.75f); }
	FLinearColor Peach()             { return HexColor(TEXT("#F2A46F")); }
	FLinearColor Blush()            { return HexColor(TEXT("#F4A9A0")); }
	FLinearColor Sky()               { return HexColor(TEXT("#9ED4EE")); }
	FLinearColor Butter()            { return HexColor(TEXT("#FFD75E")); }
	FLinearColor ButterShadow()      { return HexColor(TEXT("#D8B243")); }
	FLinearColor Char()              { return HexColor(TEXT("#6B625C")); }
	FLinearColor Highlighter()       { return HexColor(TEXT("#FFE9A8")); }
	FLinearColor CuttableUnderline() { return HexColor(TEXT("#C9A54A")); }
	FLinearColor Desk()              { return HexColor(TEXT("#5A4638")); }
	FLinearColor TapeCream()         { return HexColor(TEXT("#F3E3B5"), 0.96f); }
	FLinearColor TapePink()          { return HexColor(TEXT("#F8C9C3"), 0.96f); }
	FLinearColor TapeBlue()          { return HexColor(TEXT("#CFEAF7"), 0.96f); }
	FLinearColor KeycapShadow()      { return HexColor(TEXT("#D8CBB8")); }
	FLinearColor CutTextMuted()      { return HexColor(TEXT("#9A8F86")); }
	FLinearColor DashedRule()        { return HexColor(TEXT("#D8C7AD")); }
	FLinearColor SmokePuff()         { return HexColor(TEXT("#D9D0C7")); }
	FLinearColor BurntSmoke()        { return HexColor(TEXT("#8C827A")); }
	FLinearColor CalmResultTag()     { return HexColor(TEXT("#FFE3DF")); }
	FLinearColor DimScrim()          { return HexColor(TEXT("#2B2724"), 0.58f); }
	FLinearColor ReviewNoteFill()    { return HexColor(TEXT("#FDF3DB")); }
	FLinearColor ReviewNoteBorder()  { return HexColor(TEXT("#DFC286")); }

	float EvaluateOvershootScale(
		float Alpha,
		float StartScale,
		float PeakScale,
		float EndScale,
		float PeakAlpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		const float SafePeakAlpha = FMath::Clamp(PeakAlpha, 0.1f, 0.9f);
		if (ClampedAlpha <= SafePeakAlpha)
		{
			const float T = ClampedAlpha / SafePeakAlpha;
			const float EaseOut = 1.0f - FMath::Pow(1.0f - T, 3.0f);
			return FMath::Lerp(StartScale, PeakScale, EaseOut);
		}
		const float T = (ClampedAlpha - SafePeakAlpha) / (1.0f - SafePeakAlpha);
		const float Smooth = T * T * (3.0f - 2.0f * T);
		return FMath::Lerp(PeakScale, EndScale, Smooth);
	}

	void PlayUISound(const UObject* WorldContext, USoundBase* Sound)
	{
		if (Sound && WorldContext)
		{
			if (UWorld* World = WorldContext->GetWorld())
			{
				if (World->IsGameWorld())
				{
					UGameplayStatics::PlaySound2D(WorldContext, Sound);
				}
			}
		}
	}

	UTexture2D* LoadOptionalAngelDogSticker()
	{
		static constexpr const TCHAR* PackageName = TEXT("/Game/Textures/Interactions/T_UI_AngelDogSticker");
		static constexpr const TCHAR* ObjectPath = TEXT("/Game/Textures/Interactions/T_UI_AngelDogSticker.T_UI_AngelDogSticker");

		if (UTexture2D* Existing = FindObject<UTexture2D>(nullptr, ObjectPath))
		{
			return Existing;
		}
		if (!FPackageName::DoesPackageExist(PackageName))
		{
			return nullptr;
		}
		return LoadObject<UTexture2D>(nullptr, ObjectPath, nullptr, LOAD_Quiet | LOAD_NoWarn);
	}

	void ApplyOptionalStickerImage(UImage* StickerImage, bool bShouldShow)
	{
		if (!StickerImage)
		{
			return;
		}
		if (!bShouldShow)
		{
			StickerImage->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}
		if (UTexture2D* StickerTex = LoadOptionalAngelDogSticker())
		{
			StickerImage->SetBrushFromTexture(StickerTex, false);
			StickerImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			StickerImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	FSlateRoundedBoxBrush MakeRoundedBoxBrush(
		const FLinearColor& FillColor,
		float CornerRadius,
		const FLinearColor& OutlineColor,
		float OutlineWidth)
	{
		if (CornerRadius >= 200.0f)
		{
			return FSlateRoundedBoxBrush(FillColor, OutlineColor, OutlineWidth);
		}
		return FSlateRoundedBoxBrush(FillColor, CornerRadius, OutlineColor, OutlineWidth);
	}

	void ApplyRoundedBorderStyle(
		UBorder* Border,
		const FLinearColor& FillColor,
		float CornerRadius,
		const FLinearColor& OutlineColor,
		float OutlineWidth,
		const FMargin& Padding)
	{
		if (!Border)
		{
			return;
		}

		// SetBrushColor(White) prevents Slate from double-multiplying the brush fill color under DX12.
		Border->SetBrush(MakeRoundedBoxBrush(FillColor, CornerRadius, OutlineColor, OutlineWidth));
		Border->SetBrushColor(FLinearColor::White);
		Border->SetPadding(Padding);
	}

	UTextBlock* MakeText(
		UWidgetTree* Tree,
		const FString& InitialText,
		int32 FontSize,
		const FLinearColor& Color,
		bool bBold,
		bool bAutoWrap)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(InitialText));
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetAutoWrapText(bAutoWrap);
		Text->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", FontSize));
		return Text;
	}

	UBorder* MakeJournalBorder(
		UWidgetTree* Tree,
		UWidget* Content,
		const FLinearColor& FillColor,
		float CornerRadius,
		const FLinearColor& OutlineColor,
		float OutlineWidth,
		const FMargin& Padding)
	{
		UBorder* Border = Tree->ConstructWidget<UBorder>();
		ApplyRoundedBorderStyle(Border, FillColor, CornerRadius, OutlineColor, OutlineWidth, Padding);
		if (Content)
		{
			Border->SetContent(Content);
		}
		return Border;
	}

	USizeBox* MakeCornerTape(
		UWidgetTree* Tree,
		const FLinearColor& TapeColor,
		float AngleDegrees,
		float Width,
		float Height)
	{
		USizeBox* TapeBox = Tree->ConstructWidget<USizeBox>();
		TapeBox->SetWidthOverride(Width);
		TapeBox->SetHeightOverride(Height);
		TapeBox->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		TapeBox->SetRenderTransformAngle(AngleDegrees);
		TapeBox->SetVisibility(ESlateVisibility::HitTestInvisible);

		UBorder* TapeVisual = MakeJournalBorder(
			Tree,
			nullptr,
			TapeColor,
			8.0f,
			Ink(),
			3.0f,
			FMargin(0.0f));
		TapeBox->SetContent(TapeVisual);
		return TapeBox;
	}

	UBorder* MakeChubbyKeycap(
		UWidgetTree* Tree,
		const FString& KeyText,
		float BoxSize,
		int32 FontSize,
		float CornerRadius,
		float OutlineWidth,
		UTextBlock** OutKeyText)
	{
		UTextBlock* Label = MakeText(Tree, KeyText, FontSize, Ink(), true, false);
		Label->SetJustification(ETextJustify::Center);
		if (OutKeyText)
		{
			*OutKeyText = Label;
		}

		USizeBox* KeySize = Tree->ConstructWidget<USizeBox>();
		KeySize->SetMinDesiredWidth(BoxSize);
		KeySize->SetMinDesiredHeight(BoxSize);
		KeySize->SetContent(Label);

		UBorder* TopCap = MakeJournalBorder(
			Tree,
			KeySize,
			Paper(),
			FMath::Max(4.0f, CornerRadius - 1.0f),
			Ink(),
			OutlineWidth,
			FMargin(6.0f, 2.0f));
		TopCap->SetHorizontalAlignment(HAlign_Center);
		TopCap->SetVerticalAlignment(VAlign_Center);

		// Bottom chunky lip simulates the chubby hand-drawn plastic keycap extruded bottom
		UBorder* OuterLip = MakeJournalBorder(
			Tree,
			TopCap,
			KeycapShadow(),
			CornerRadius,
			Ink(),
			OutlineWidth,
			FMargin(0.0f, 0.0f, 0.0f, 4.0f));
		return OuterLip;
	}

	UBorder* MakeBadgePill(
		UWidgetTree* Tree,
		const FString& Label,
		const FLinearColor& FillColor,
		const FLinearColor& TextColor,
		int32 FontSize,
		const FLinearColor& OutlineColor,
		float OutlineWidth,
		const FMargin& Padding,
		UTextBlock** OutLabel)
	{
		UTextBlock* BadgeLabel = MakeText(Tree, Label, FontSize, TextColor, true, false);
		if (OutLabel)
		{
			*OutLabel = BadgeLabel;
		}
		return MakeJournalBorder(
			Tree,
			BadgeLabel,
			FillColor,
			999.0f,
			OutlineColor,
			OutlineWidth,
			Padding);
	}

	UBorder* MakeChubbyButton(
		UWidgetTree* Tree,
		UButton*& OutButton,
		const FText& Label,
		const FLinearColor& FillColor,
		const FLinearColor& HoverFillColor,
		const FLinearColor& LipColor,
		const FLinearColor& TextColor,
		int32 FontSize,
		float CornerRadius,
		float OutlineWidth,
		const FMargin& ContentPadding,
		FDCOptionalTextBlockOut OutLabel)
	{
		OutButton = Tree->ConstructWidget<UButton>();
		FButtonStyle Style;
		Style.Normal = MakeRoundedBoxBrush(FillColor, CornerRadius, Ink(), OutlineWidth);
		Style.Hovered = MakeRoundedBoxBrush(HoverFillColor, CornerRadius, Ink(), OutlineWidth);
		Style.Pressed = MakeRoundedBoxBrush(HoverFillColor, CornerRadius, Ink(), OutlineWidth);
		Style.Disabled = MakeRoundedBoxBrush(Paper(), CornerRadius, CutTextMuted(), OutlineWidth);
		Style.NormalPadding = FMargin(0.0f);
		Style.PressedPadding = FMargin(0.0f, 2.0f, 0.0f, 0.0f);
		OutButton->SetStyle(Style);

		UTextBlock* Caption = MakeText(Tree, Label.ToString(), FontSize, TextColor, true, false);
		Caption->SetJustification(ETextJustify::Center);
		if (OutLabel.Ptr)
		{
			*OutLabel.Ptr = Caption;
		}

		UBorder* InnerPadding = Tree->ConstructWidget<UBorder>();
		InnerPadding->SetBrushColor(FLinearColor::Transparent);
		InnerPadding->SetPadding(ContentPadding);
		InnerPadding->SetHorizontalAlignment(HAlign_Center);
		InnerPadding->SetVerticalAlignment(VAlign_Center);
		InnerPadding->SetContent(Caption);
		OutButton->SetContent(InnerPadding);

		UBorder* OuterLip = MakeJournalBorder(
			Tree,
			OutButton,
			LipColor,
			CornerRadius,
			Ink(),
			OutlineWidth,
			FMargin(0.0f, 0.0f, 0.0f, 5.0f));
		return OuterLip;
	}

	UButton* MakeKeycapPillButton(
		UWidgetTree* Tree,
		const FString& KeyText,
		const FString& LabelText,
		const FLinearColor& FillColor,
		const FLinearColor& HoverFillColor,
		float OutlineWidth,
		float KeycapSize,
		int32 KeycapFontSize,
		int32 LabelFontSize,
		const FMargin& ContentPadding,
		UTextBlock** OutLabelText)
	{
		UButton* Button = Tree->ConstructWidget<UButton>();
		FButtonStyle PillStyle;
		PillStyle.Normal = MakeRoundedBoxBrush(FillColor, 999.0f, Ink(), OutlineWidth);
		PillStyle.Hovered = MakeRoundedBoxBrush(HoverFillColor, 999.0f, Ink(), OutlineWidth);
		PillStyle.Pressed = MakeRoundedBoxBrush(HoverFillColor, 999.0f, Ink(), OutlineWidth);
		PillStyle.Disabled = MakeRoundedBoxBrush(Paper(), 999.0f, CutTextMuted(), OutlineWidth);
		PillStyle.NormalPadding = FMargin(0.0f);
		PillStyle.PressedPadding = FMargin(0.0f, OutlineWidth >= 4.0f ? 2.0f : 1.0f, 0.0f, 0.0f);
		Button->SetStyle(PillStyle);

		UHorizontalBox* ContentRow = Tree->ConstructWidget<UHorizontalBox>();
		UBorder* Keycap = MakeChubbyKeycap(
			Tree,
			KeyText,
			KeycapSize,
			KeycapFontSize,
			FMath::Clamp(KeycapSize * 0.28f, 6.0f, 10.0f),
			2.5f);
		AddHBoxChild(ContentRow, Keycap, FMargin(0.0f, 0.0f, 10.0f, 0.0f), HAlign_Left, VAlign_Center);

		UTextBlock* Label = MakeText(Tree, LabelText, LabelFontSize, Ink(), true, false);
		if (OutLabelText)
		{
			*OutLabelText = Label;
		}
		AddHBoxChild(ContentRow, Label, FMargin(0.0f), HAlign_Left, VAlign_Center);

		UBorder* InnerPad = Tree->ConstructWidget<UBorder>();
		InnerPad->SetBrushColor(FLinearColor::Transparent);
		InnerPad->SetPadding(ContentPadding);
		InnerPad->SetHorizontalAlignment(HAlign_Center);
		InnerPad->SetVerticalAlignment(VAlign_Center);
		InnerPad->SetContent(ContentRow);
		Button->SetContent(InnerPad);

		return Button;
	}

	UVerticalBoxSlot* AddVBoxChild(
		UVerticalBox* Box,
		UWidget* Child,
		const FMargin& Padding,
		EHorizontalAlignment HAlign,
		EVerticalAlignment VAlign,
		ESlateSizeRule::Type SizeRule,
		float FillValue)
	{
		UVerticalBoxSlot* ChildSlot = Box->AddChildToVerticalBox(Child);
		if (ChildSlot)
		{
			ChildSlot->SetPadding(Padding);
			ChildSlot->SetHorizontalAlignment(HAlign);
			ChildSlot->SetVerticalAlignment(VAlign);
			FSlateChildSize ChildSize(SizeRule);
			ChildSize.Value = FillValue;
			ChildSlot->SetSize(ChildSize);
		}
		return ChildSlot;
	}

	UHorizontalBoxSlot* AddHBoxChild(
		UHorizontalBox* Box,
		UWidget* Child,
		const FMargin& Padding,
		EHorizontalAlignment HAlign,
		EVerticalAlignment VAlign,
		ESlateSizeRule::Type SizeRule,
		float FillValue)
	{
		UHorizontalBoxSlot* ChildSlot = Box->AddChildToHorizontalBox(Child);
		if (ChildSlot)
		{
			ChildSlot->SetPadding(Padding);
			ChildSlot->SetHorizontalAlignment(HAlign);
			ChildSlot->SetVerticalAlignment(VAlign);
			FSlateChildSize ChildSize(SizeRule);
			ChildSize.Value = FillValue;
			ChildSlot->SetSize(ChildSize);
		}
		return ChildSlot;
	}

	FString BuildStarGlyphText(int32 EarnedStars, int32 MaxContainers)
	{
		const int32 ClampedMax = FMath::Max(0, MaxContainers);
		const int32 ClampedEarned = FMath::Clamp(EarnedStars, 0, ClampedMax);
		FString Result;
		for (int32 Index = 0; Index < ClampedMax; ++Index)
		{
			if (Index > 0)
			{
				Result.Append(TEXT(" "));
			}
			Result.Append(Index < ClampedEarned ? TEXT("★") : TEXT("☆"));
		}
		return Result.IsEmpty() ? TEXT("(no stars)") : Result;
	}

	void PaintFilledCircle(
		FSlateWindowElementList& OutDrawElements,
		uint32 LayerId,
		const FGeometry& AllottedGeometry,
		const FVector2f& CenterLocal,
		float Radius,
		const FLinearColor& FillColor,
		const FLinearColor& OutlineColor,
		float OutlineThickness,
		int32 NumSegments)
	{
		if (Radius <= 0.1f || NumSegments < 6)
		{
			return;
		}

		const FSlateRenderTransform& Transform = AllottedGeometry.GetAccumulatedRenderTransform();
		const FColor FillByteColor = FillColor.ToFColor(true);

		TArray<FSlateVertex> Verts;
		TArray<SlateIndex> Indices;
		Verts.Reserve(NumSegments + 1);
		Indices.Reserve(NumSegments * 3);

		Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			Transform, CenterLocal, FVector2f(0.5f, 0.5f), FillByteColor));

		TArray<FVector2f> RingPoints;
		const bool bDrawOutline = (OutlineColor.A > KINDA_SMALL_NUMBER && OutlineThickness > 0.0f);
		if (bDrawOutline)
		{
			RingPoints.Reserve(NumSegments + 1);
		}

		for (int32 i = 0; i < NumSegments; ++i)
		{
			const float Angle = (UE_TWO_PI * static_cast<float>(i)) / static_cast<float>(NumSegments);
			const FVector2f EdgePoint = CenterLocal + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
			Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
				Transform, EdgePoint, FVector2f(0.5f, 0.5f), FillByteColor));
			if (bDrawOutline)
			{
				RingPoints.Add(EdgePoint);
			}

			const uint32 NextIdx = static_cast<uint32>((i + 1) % NumSegments) + 1u;
			Indices.Add(0u);
			Indices.Add(static_cast<uint32>(i + 1));
			Indices.Add(NextIdx);
		}

		if (FillColor.A > KINDA_SMALL_NUMBER)
		{
			FSlateDrawElement::MakeCustomVerts(
				OutDrawElements,
				LayerId,
				FSlateResourceHandle(),
				Verts,
				Indices,
				nullptr,
				0,
				0);
		}

		if (bDrawOutline && RingPoints.Num() > 1)
		{
			const FVector2f FirstPoint = RingPoints[0];
			RingPoints.Add(FirstPoint);
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry.ToPaintGeometry(),
				MoveTemp(RingPoints),
				ESlateDrawEffect::None,
				OutlineColor,
				true,
				OutlineThickness);
		}
	}

	void PaintArcStroke(
		FSlateWindowElementList& OutDrawElements,
		uint32 LayerId,
		const FGeometry& AllottedGeometry,
		const FVector2f& CenterLocal,
		float Radius,
		float StartAngleRadians,
		float EndAngleRadians,
		const FLinearColor& Color,
		float Thickness,
		int32 NumSegments,
		bool bRoundEndCaps)
	{
		if (Radius <= 0.1f || FMath::Abs(EndAngleRadians - StartAngleRadians) <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		const int32 Steps = FMath::Max(4, NumSegments);
		TArray<FVector2f> Points;
		Points.Reserve(Steps + 1);
		for (int32 i = 0; i <= Steps; ++i)
		{
			const float Alpha = static_cast<float>(i) / static_cast<float>(Steps);
			const float Angle = FMath::Lerp(StartAngleRadians, EndAngleRadians, Alpha);
			Points.Add(CenterLocal + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}

		if (bRoundEndCaps && Points.Num() >= 2)
		{
			const float CapRadius = Thickness * 0.5f;
			PaintFilledCircle(OutDrawElements, LayerId, AllottedGeometry, Points[0], CapRadius, Color, FLinearColor::Transparent, 0.0f, 12);
			PaintFilledCircle(OutDrawElements, LayerId, AllottedGeometry, Points.Last(), CapRadius, Color, FLinearColor::Transparent, 0.0f, 12);
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			MoveTemp(Points),
			ESlateDrawEffect::None,
			Color,
			true,
			Thickness);
	}
}

namespace
{
	FVector2f RotatePointAround(const FVector2f& Point, const FVector2f& Pivot, float CosAngle, float SinAngle)
	{
		const FVector2f Delta = Point - Pivot;
		return Pivot + FVector2f(Delta.X * CosAngle - Delta.Y * SinAngle, Delta.X * SinAngle + Delta.Y * CosAngle);
	}

	void ApplyChaikinBevel(const TArray<FVector2f>& Input, TArray<FVector2f>& OutSmoothed, float CornerRatio = 0.16f)
	{
		OutSmoothed.Reset(Input.Num() * 2);
		const int32 Count = Input.Num();
		for (int32 i = 0; i < Count; ++i)
		{
			const FVector2f& A = Input[i];
			const FVector2f& B = Input[(i + 1) % Count];
			OutSmoothed.Add(FMath::Lerp(A, B, CornerRatio));
			OutSmoothed.Add(FMath::Lerp(A, B, 1.0f - CornerRatio));
		}
	}
}

// -----------------------------------------------------------------------------
// UDCJournalStarWidget & SDCJournalStar
// -----------------------------------------------------------------------------

class UDCJournalStarWidget::SDCJournalStar : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SDCJournalStar) {}
		SLATE_ARGUMENT(EDCJournalStarState, StarState)
		SLATE_ARGUMENT(float, StarDiameter)
		SLATE_ARGUMENT(float, VisualOpacity)
		SLATE_ARGUMENT(float, SparkleStrength)
		SLATE_ARGUMENT(float, BurntProgress)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		StarState = InArgs._StarState;
		StarDiameter = InArgs._StarDiameter;
		VisualOpacity = InArgs._VisualOpacity;
		SparkleStrength = InArgs._SparkleStrength;
		BurntProgress = InArgs._BurntProgress;
	}

	void UpdateVisuals(
		EDCJournalStarState InState,
		float InDiameter,
		float InOpacity,
		float InSparkleStrength,
		float InBurntProgress,
		UTexture2D* InEmptyTex,
		UTexture2D* InFilledTex,
		UTexture2D* InBurntTex)
	{
		StarState = InState;
		StarDiameter = FMath::Max(16.0f, InDiameter);
		VisualOpacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);
		SparkleStrength = FMath::Clamp(InSparkleStrength, 0.0f, 1.0f);
		BurntProgress = FMath::Clamp(InBurntProgress, 0.0f, 1.0f);
		EmptyTex = InEmptyTex;
		FilledTex = InFilledTex;
		BurntTex = InBurntTex;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(StarDiameter, StarDiameter);
	}

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		const FVector2f LocalSize = AllottedGeometry.GetLocalSize();
		const float Size = FMath::Min(LocalSize.X, LocalSize.Y);
		const float EffectiveOpacity = VisualOpacity * InWidgetStyle.GetColorAndOpacityTint().A;
		if (Size <= 2.0f || EffectiveOpacity <= KINDA_SMALL_NUMBER)
		{
			return LayerId;
		}

		const FVector2f Origin((LocalSize.X - Size) * 0.5f, (LocalSize.Y - Size) * 0.5f);

		UTexture2D* OverrideTexture = nullptr;
		if (StarState == EDCJournalStarState::Filled)
		{
			OverrideTexture = FilledTex.Get();
		}
		else if (StarState == EDCJournalStarState::Burnt)
		{
			OverrideTexture = BurntTex.Get();
		}
		else
		{
			OverrideTexture = EmptyTex.Get();
		}

		if (OverrideTexture != nullptr)
		{
			const FSlateImageBrush Brush(OverrideTexture, FVector2D(Size, Size));
			const FPaintGeometry SquareGeom = AllottedGeometry.ToPaintGeometry(
				FVector2f(Size, Size),
				FSlateLayoutTransform(Origin));
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				SquareGeom,
				&Brush,
				ESlateDrawEffect::None,
				FLinearColor(1.0f, 1.0f, 1.0f, EffectiveOpacity));
			return LayerId + 1;
		}

		static const FVector2f BaseStarPoints[10] = {
			FVector2f(50.0f,  9.0f),
			FVector2f(63.0f, 35.0f),
			FVector2f(91.0f, 38.0f),
			FVector2f(70.0f, 58.0f),
			FVector2f(76.0f, 87.0f),
			FVector2f(50.0f, 72.0f),
			FVector2f(24.0f, 87.0f),
			FVector2f(30.0f, 58.0f),
			FVector2f( 9.0f, 38.0f),
			FVector2f(37.0f, 35.0f)
		};

		const float Scale = Size / 100.0f;
		const FVector2f Pivot(50.0f, 50.0f);
		const float TiltRadians = (StarState == EDCJournalStarState::Burnt) ? FMath::DegreesToRadians(14.0f) : 0.0f;
		const float CosAngle = FMath::Cos(TiltRadians);
		const float SinAngle = FMath::Sin(TiltRadians);

		TArray<FVector2f> RawLocal;
		RawLocal.Reserve(10);
		for (const FVector2f& NormPt : BaseStarPoints)
		{
			const FVector2f Tilted = (TiltRadians != 0.0f)
				? RotatePointAround(NormPt, Pivot, CosAngle, SinAngle)
				: NormPt;
			RawLocal.Add(Origin + Tilted * Scale);
		}

		TArray<FVector2f> SmoothPerimeter;
		ApplyChaikinBevel(RawLocal, SmoothPerimeter, 0.16f);

		FLinearColor FillColor = DCTheme::Paper();
		if (StarState == EDCJournalStarState::Filled)
		{
			FillColor = DCTheme::Butter();
		}
		else if (StarState == EDCJournalStarState::Burnt)
		{
			FillColor = DCTheme::Char();
		}
		FillColor.A *= EffectiveOpacity;

		FLinearColor InkColor = DCTheme::Ink();
		InkColor.A *= EffectiveOpacity;

		const FSlateRenderTransform& Transform = AllottedGeometry.GetAccumulatedRenderTransform();
		const FColor FillByteColor = FillColor.ToFColor(true);
		const FVector2f CenterLocal = Origin + Pivot * Scale;

		TArray<FSlateVertex> Verts;
		TArray<SlateIndex> Indices;
		const int32 PerimeterCount = SmoothPerimeter.Num();
		Verts.Reserve(PerimeterCount + 1);
		Indices.Reserve(PerimeterCount * 3);

		Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			Transform, CenterLocal, FVector2f(0.5f, 0.5f), FillByteColor));

		for (int32 i = 0; i < PerimeterCount; ++i)
		{
			Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
				Transform, SmoothPerimeter[i], FVector2f(0.5f, 0.5f), FillByteColor));
			const uint32 NextIdx = static_cast<uint32>((i + 1) % PerimeterCount) + 1u;
			Indices.Add(0u);
			Indices.Add(static_cast<uint32>(i + 1));
			Indices.Add(NextIdx);
		}

		FSlateDrawElement::MakeCustomVerts(
			OutDrawElements,
			LayerId,
			FSlateResourceHandle(),
			Verts,
			Indices,
			nullptr,
			0,
			0);

		TArray<FVector2f> ClosedLoop = SmoothPerimeter;
		ClosedLoop.Add(SmoothPerimeter[0]);
		const float OutlineThickness = FMath::Clamp(Size * 0.055f, 2.2f, 5.0f);
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(),
			MoveTemp(ClosedLoop),
			ESlateDrawEffect::None,
			InkColor,
			true,
			OutlineThickness);

		int32 HighestLayer = LayerId + 1;

		if (StarState == EDCJournalStarState::Burnt)
		{
			static const FVector2f CrackNorm[4] = {
				FVector2f(52.0f, 16.0f),
				FVector2f(44.0f, 40.0f),
				FVector2f(58.0f, 52.0f),
				FVector2f(46.0f, 76.0f)
			};
			TArray<FVector2f> CrackPoints;
			CrackPoints.Reserve(4);
			for (const FVector2f& NormPt : CrackNorm)
			{
				CrackPoints.Add(Origin + RotatePointAround(NormPt, Pivot, CosAngle, SinAngle) * Scale);
			}
			FLinearColor CrackColor = DCTheme::Paper();
			CrackColor.A *= EffectiveOpacity;
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 2,
				AllottedGeometry.ToPaintGeometry(),
				MoveTemp(CrackPoints),
				ESlateDrawEffect::None,
				CrackColor,
				true,
				FMath::Clamp(Size * 0.045f, 1.8f, 3.8f));

			HighestLayer = LayerId + 2;

			// Static cartoon smoke puffs for settled burnt stars (e.g. TakeResult); transient popups paint rising puffs in NativePaint
			if (BurntProgress >= 0.99f)
			{
				const float SmokeLift = -3.0f * BurntProgress * Scale;
				FLinearColor SmokeColor = DCTheme::SmokePuff();
				SmokeColor.A *= EffectiveOpacity * FMath::Lerp(0.95f, 0.72f, BurntProgress);
				FLinearColor SmokeInk = InkColor;
				SmokeInk.A *= FMath::Lerp(0.95f, 0.72f, BurntProgress);

				const float PuffOutline = FMath::Clamp(Size * 0.032f, 1.5f, 2.8f);
				DCTheme::PaintFilledCircle(
					OutDrawElements, LayerId + 3, AllottedGeometry,
					Origin + FVector2f(72.0f * Scale, 17.0f * Scale + SmokeLift),
					6.5f * Scale, SmokeColor, SmokeInk, PuffOutline, 16);
				DCTheme::PaintFilledCircle(
					OutDrawElements, LayerId + 3, AllottedGeometry,
					Origin + FVector2f(84.0f * Scale, 20.0f * Scale + SmokeLift),
					5.0f * Scale, SmokeColor, SmokeInk, PuffOutline, 16);
				DCTheme::PaintFilledCircle(
					OutDrawElements, LayerId + 3, AllottedGeometry,
					Origin + FVector2f(78.0f * Scale, 9.0f * Scale + SmokeLift),
					5.6f * Scale, SmokeColor, SmokeInk, PuffOutline, 16);

				HighestLayer = LayerId + 4;
			}
		}
		else if (StarState == EDCJournalStarState::Filled && SparkleStrength > 0.01f)
		{
			FLinearColor SparkleColor = DCTheme::Butter();
			SparkleColor.A *= SparkleStrength * EffectiveOpacity;
			const auto DrawSparkle = [&](const FVector2f& NormCenter, float Arm)
			{
				const FVector2f C = Origin + NormCenter * Scale;
				const float ArmPx = Arm * Scale * (0.6f + 0.4f * SparkleStrength);
				FSlateDrawElement::MakeLines(
					OutDrawElements,
					LayerId + 2,
					AllottedGeometry.ToPaintGeometry(),
					{ C + FVector2f(-ArmPx, 0.0f), C + FVector2f(ArmPx, 0.0f) },
					ESlateDrawEffect::None,
					SparkleColor,
					true,
					2.2f);
				FSlateDrawElement::MakeLines(
					OutDrawElements,
					LayerId + 2,
					AllottedGeometry.ToPaintGeometry(),
					{ C + FVector2f(0.0f, -ArmPx), C + FVector2f(0.0f, ArmPx) },
					ESlateDrawEffect::None,
					SparkleColor,
					true,
					2.2f);
			};
			DrawSparkle(FVector2f(14.0f, 16.0f), 5.5f);
			DrawSparkle(FVector2f(86.0f, 15.0f), 4.8f);
			HighestLayer = LayerId + 2;
		}

		return HighestLayer;
	}

private:
	EDCJournalStarState StarState = EDCJournalStarState::Empty;
	float StarDiameter = 44.0f;
	float VisualOpacity = 1.0f;
	float SparkleStrength = 0.0f;
	float BurntProgress = 1.0f;
	TWeakObjectPtr<UTexture2D> EmptyTex;
	TWeakObjectPtr<UTexture2D> FilledTex;
	TWeakObjectPtr<UTexture2D> BurntTex;
};

UDCJournalStarWidget::UDCJournalStarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDCJournalStarWidget::SetStarState(EDCJournalStarState NewState)
{
	StarState = NewState;
	SynchronizeProperties();
}

void UDCJournalStarWidget::SetStarDiameter(float NewDiameter)
{
	StarDiameter = FMath::Max(16.0f, NewDiameter);
	SynchronizeProperties();
}

void UDCJournalStarWidget::SetVisualOpacity(float NewOpacity)
{
	VisualOpacity = FMath::Clamp(NewOpacity, 0.0f, 1.0f);
	SynchronizeProperties();
}

void UDCJournalStarWidget::SetSparkleStrength(float NewSparkleStrength)
{
	SparkleStrength = FMath::Clamp(NewSparkleStrength, 0.0f, 1.0f);
	SynchronizeProperties();
}

void UDCJournalStarWidget::SetBurntProgress(float NewBurntProgress)
{
	BurntProgress = FMath::Clamp(NewBurntProgress, 0.0f, 1.0f);
	SynchronizeProperties();
}

void UDCJournalStarWidget::SetTextureOverrides(UTexture2D* InEmptyTexture, UTexture2D* InFilledTexture, UTexture2D* InBurntTexture)
{
	EmptyTextureOverride = InEmptyTexture;
	FilledTextureOverride = InFilledTexture;
	BurntTextureOverride = InBurntTexture;
	SynchronizeProperties();
}

void UDCJournalStarWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (MyStarWidget.IsValid())
	{
		MyStarWidget->UpdateVisuals(
			StarState,
			StarDiameter,
			VisualOpacity,
			SparkleStrength,
			BurntProgress,
			EmptyTextureOverride.Get(),
			FilledTextureOverride.Get(),
			BurntTextureOverride.Get());
		InvalidateLayoutAndVolatility();
	}
}

void UDCJournalStarWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyStarWidget.Reset();
}

TSharedRef<SWidget> UDCJournalStarWidget::RebuildWidget()
{
	MyStarWidget = SNew(SDCJournalStar)
		.StarState(StarState)
		.StarDiameter(StarDiameter)
		.VisualOpacity(VisualOpacity)
		.SparkleStrength(SparkleStrength)
		.BurntProgress(BurntProgress);
	return MyStarWidget.ToSharedRef();
}

// -----------------------------------------------------------------------------
// UDCJournalScissorsWidget & SDCJournalScissors
// -----------------------------------------------------------------------------

class UDCJournalScissorsWidget::SDCJournalScissors : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SDCJournalScissors) {}
		SLATE_ARGUMENT(float, IconSize)
		SLATE_ARGUMENT(FLinearColor, IconTint)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		IconSize = InArgs._IconSize;
		IconTint = InArgs._IconTint;
	}

	void UpdateVisuals(float InSize, const FLinearColor& InTint, UTexture2D* InTex)
	{
		IconSize = FMath::Max(12.0f, InSize);
		IconTint = InTint;
		ScissorsTex = InTex;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(IconSize, IconSize);
	}

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		const FVector2f LocalSize = AllottedGeometry.GetLocalSize();
		const float Size = FMath::Min(LocalSize.X, LocalSize.Y);
		const float Alpha = IconTint.A * InWidgetStyle.GetColorAndOpacityTint().A;
		if (Size <= 2.0f || Alpha <= KINDA_SMALL_NUMBER)
		{
			return LayerId;
		}
		const FVector2f Origin((LocalSize.X - Size) * 0.5f, (LocalSize.Y - Size) * 0.5f);

		if (ScissorsTex.IsValid())
		{
			FLinearColor EffectiveTint = IconTint;
			EffectiveTint.A = Alpha;
			const FSlateImageBrush Brush(ScissorsTex.Get(), FVector2D(Size, Size));
			const FPaintGeometry SquareGeom = AllottedGeometry.ToPaintGeometry(
				FVector2f(Size, Size),
				FSlateLayoutTransform(Origin));
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, SquareGeom, &Brush, ESlateDrawEffect::None, EffectiveTint);
			return LayerId + 1;
		}

		const float Scale = Size / 28.0f;
		FLinearColor InkColor = DCTheme::Ink();
		InkColor.A *= Alpha;
		FLinearColor PaperColor = DCTheme::Paper();
		PaperColor.A *= Alpha;
		FLinearColor PivotColor = DCTheme::Sky();
		PivotColor.A *= Alpha;

		// Crossed scissor blades
		const float BladeWidth = FMath::Clamp(2.8f * Scale, 1.8f, 4.0f);
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			{ Origin + FVector2f(10.0f, 10.0f) * Scale, Origin + FVector2f(24.0f, 20.5f) * Scale },
			ESlateDrawEffect::None,
			InkColor,
			true,
			BladeWidth);
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			{ Origin + FVector2f(10.0f, 18.0f) * Scale, Origin + FVector2f(24.0f, 7.5f) * Scale },
			ESlateDrawEffect::None,
			InkColor,
			true,
			BladeWidth);

		// Two handle finger loops
		const float HandleOutline = FMath::Clamp(2.5f * Scale, 1.6f, 3.6f);
		DCTheme::PaintFilledCircle(
			OutDrawElements, LayerId + 1, AllottedGeometry,
			Origin + FVector2f(7.5f, 8.5f) * Scale,
			4.0f * Scale, PaperColor, InkColor, HandleOutline, 16);
		DCTheme::PaintFilledCircle(
			OutDrawElements, LayerId + 1, AllottedGeometry,
			Origin + FVector2f(7.5f, 19.5f) * Scale,
			4.0f * Scale, PaperColor, InkColor, HandleOutline, 16);

		// Cute blue pivot pin matching journal sky accent
		DCTheme::PaintFilledCircle(
			OutDrawElements, LayerId + 3, AllottedGeometry,
			Origin + FVector2f(15.0f, 14.0f) * Scale,
			2.3f * Scale, PivotColor, InkColor, FMath::Clamp(1.6f * Scale, 1.0f, 2.5f), 12);

		return LayerId + 4;
	}

private:
	float IconSize = 28.0f;
	FLinearColor IconTint = FLinearColor::White;
	TWeakObjectPtr<UTexture2D> ScissorsTex;
};

UDCJournalScissorsWidget::UDCJournalScissorsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDCJournalScissorsWidget::SetIconSize(float NewSize)
{
	IconSize = FMath::Max(12.0f, NewSize);
	SynchronizeProperties();
}

void UDCJournalScissorsWidget::SetTint(const FLinearColor& NewTint)
{
	IconTint = NewTint;
	SynchronizeProperties();
}

void UDCJournalScissorsWidget::SetTextureOverride(UTexture2D* InScissorsTexture)
{
	ScissorsTextureOverride = InScissorsTexture;
	SynchronizeProperties();
}

void UDCJournalScissorsWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (MyScissorsWidget.IsValid())
	{
		MyScissorsWidget->UpdateVisuals(IconSize, IconTint, ScissorsTextureOverride.Get());
		InvalidateLayoutAndVolatility();
	}
}

void UDCJournalScissorsWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyScissorsWidget.Reset();
}

TSharedRef<SWidget> UDCJournalScissorsWidget::RebuildWidget()
{
	MyScissorsWidget = SNew(SDCJournalScissors)
		.IconSize(IconSize)
		.IconTint(IconTint);
	return MyScissorsWidget.ToSharedRef();
}

// -----------------------------------------------------------------------------
// UDCDashedRuleWidget & SDCDashedRule
// -----------------------------------------------------------------------------

class UDCDashedRuleWidget::SDCDashedRule : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SDCDashedRule) {}
		SLATE_ARGUMENT(FLinearColor, RuleColor)
		SLATE_ARGUMENT(float, RuleThickness)
		SLATE_ARGUMENT(float, DashLengthPx)
		SLATE_ARGUMENT(float, DesiredHeight)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		RuleColor = InArgs._RuleColor;
		RuleThickness = InArgs._RuleThickness;
		DashLengthPx = InArgs._DashLengthPx;
		DesiredHeight = InArgs._DesiredHeight;
	}

	void UpdateVisuals(const FLinearColor& InColor, float InThickness, float InDashLength, float InHeight)
	{
		RuleColor = InColor;
		RuleThickness = FMath::Max(1.0f, InThickness);
		DashLengthPx = FMath::Max(2.0f, InDashLength);
		DesiredHeight = FMath::Max(2.0f, InHeight);
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(32.0f, DesiredHeight);
	}

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		const FVector2f Size = AllottedGeometry.GetLocalSize();
		if (Size.X <= 2.0f)
		{
			return LayerId;
		}
		const float MidY = Size.Y * 0.5f;
		FSlateDrawElement::MakeDashedLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			{ FVector2f(0.0f, MidY), FVector2f(Size.X, MidY) },
			ESlateDrawEffect::None,
			RuleColor,
			RuleThickness,
			DashLengthPx,
			0.0f);
		return LayerId;
	}

private:
	FLinearColor RuleColor = FLinearColor::White;
	float RuleThickness = 3.0f;
	float DashLengthPx = 9.0f;
	float DesiredHeight = 10.0f;
};

UDCDashedRuleWidget::UDCDashedRuleWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RuleColor = DCTheme::DashedRule();
}

void UDCDashedRuleWidget::ConfigureRule(const FLinearColor& NewColor, float NewThickness, float NewDashLength, float NewHeight)
{
	RuleColor = NewColor;
	RuleThickness = FMath::Max(1.0f, NewThickness);
	DashLengthPx = FMath::Max(2.0f, NewDashLength);
	DesiredHeight = FMath::Max(2.0f, NewHeight);
	SynchronizeProperties();
}

void UDCDashedRuleWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (MyRuleWidget.IsValid())
	{
		MyRuleWidget->UpdateVisuals(RuleColor, RuleThickness, DashLengthPx, DesiredHeight);
		InvalidateLayoutAndVolatility();
	}
}

void UDCDashedRuleWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyRuleWidget.Reset();
}

TSharedRef<SWidget> UDCDashedRuleWidget::RebuildWidget()
{
	MyRuleWidget = SNew(SDCDashedRule)
		.RuleColor(RuleColor)
		.RuleThickness(RuleThickness)
		.DashLengthPx(DashLengthPx)
		.DesiredHeight(DesiredHeight);
	return MyRuleWidget.ToSharedRef();
}
