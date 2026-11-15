#include "UI/DCStarPopupWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Core/DCTakeSubsystem.h"
#include "Data/DCLevelConfigData.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "UI/DCWidgetTheme.h"

UDCStarPopupWidget::UDCStarPopupWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDCStarPopupWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}
}

TSharedRef<SWidget> UDCStarPopupWidget::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}
	return Super::RebuildWidget();
}

void UDCStarPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PopupAccentBorder)
	{
		PopupAccentBorder->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	}

	RemainingDisplaySeconds = 0.0f;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDCStarPopupWidget::NativeDestruct()
{
	UnbindSubsystem();
	Super::NativeDestruct();
}

void UDCStarPopupWidget::EnsureDefaultLayout()
{
	if (bDefaultLayoutBuilt || !WidgetTree)
	{
		return;
	}
	bDefaultLayoutBuilt = true;

	UBorder* Root = WidgetTree->ConstructWidget<UBorder>();
	Root->SetBrushColor(FLinearColor::Transparent);
	Root->SetHorizontalAlignment(HAlign_Center);
	Root->SetVerticalAlignment(VAlign_Top);
	Root->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
	WidgetTree->RootWidget = Root;

	StarsGlyphRow = WidgetTree->ConstructWidget<UHorizontalBox>();

	PopupAccentBorder = DCTheme::MakeJournalBorder(
		WidgetTree,
		StarsGlyphRow,
		DCTheme::Paper(),
		999.0f,
		DCTheme::Ink(),
		4.0f,
		FMargin(28.0f, 12.0f));
	PopupAccentBorder->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Root->SetContent(PopupAccentBorder);
}

void UDCStarPopupWidget::UnbindSubsystem()
{
	if (BoundSubsystem.IsValid())
	{
		BoundSubsystem->OnStarStateChanged.RemoveDynamic(this, &UDCStarPopupWidget::HandleStarStateChanged);
		BoundSubsystem->OnTakePhaseChanged.RemoveDynamic(this, &UDCStarPopupWidget::HandleTakePhaseChanged);
		BoundSubsystem->OnTakeReset.RemoveDynamic(this, &UDCStarPopupWidget::HandleTakeReset);
		BoundSubsystem.Reset();
	}
}

void UDCStarPopupWidget::BindToTakeSubsystem(UDCTakeSubsystem* InSubsystem)
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

	const UDCLevelConfigData* Config = InSubsystem->GetActiveConfig();
	CachedMaxContainers = Config ? FMath::Max(1, Config->InitialStarContainers) : FMath::Max(1, InSubsystem->GetMaxStarContainers());
	CachedEarnedStars = InSubsystem->GetEarnedStars();

	InSubsystem->OnStarStateChanged.AddDynamic(this, &UDCStarPopupWidget::HandleStarStateChanged);
	InSubsystem->OnTakePhaseChanged.AddDynamic(this, &UDCStarPopupWidget::HandleTakePhaseChanged);
	InSubsystem->OnTakeReset.AddDynamic(this, &UDCStarPopupWidget::HandleTakeReset);

	if (!InSubsystem->IsActiveTake())
	{
		DismissPopup();
	}
}

void UDCStarPopupWidget::DismissPopup()
{
	RemainingDisplaySeconds = 0.0f;
	ElapsedDisplaySeconds = 0.0f;
	NewlyFilledIndex = INDEX_NONE;
	NewlyBurntIndex = INDEX_NONE;
	SetRenderOpacity(1.0f);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDCStarPopupWidget::HandleStarStateChanged(int32 EarnedStars, int32 MaxContainers, EDCStarChangeReason Reason)
{
	if (Reason == EDCStarChangeReason::Initialized || (BoundSubsystem.IsValid() && !BoundSubsystem->IsActiveTake()))
	{
		CachedEarnedStars = EarnedStars;
		CachedMaxContainers = MaxContainers;
		NewlyBurntIndex = INDEX_NONE;
		NewlyFilledIndex = INDEX_NONE;
		DismissPopup();
		return;
	}

	const UDCLevelConfigData* Config = BoundSubsystem.IsValid() ? BoundSubsystem->GetActiveConfig() : nullptr;
	const float Duration = Config ? Config->StarPopupDisplaySeconds : 2.0f;
	TriggerPopup(EarnedStars, MaxContainers, Reason, Duration);
}

void UDCStarPopupWidget::HandleTakePhaseChanged(EDCTakePhase NewPhase)
{
	if (NewPhase != EDCTakePhase::ActiveTake)
	{
		DismissPopup();
	}
}

void UDCStarPopupWidget::HandleTakeReset()
{
	const UDCLevelConfigData* Config = BoundSubsystem.IsValid() ? BoundSubsystem->GetActiveConfig() : nullptr;
	CachedMaxContainers = Config ? FMath::Max(1, Config->InitialStarContainers) : 3;
	CachedEarnedStars = 0;
	NewlyBurntIndex = INDEX_NONE;
	NewlyFilledIndex = INDEX_NONE;
	DismissPopup();
}

void UDCStarPopupWidget::RebuildStarIcons(int32 EarnedStars, int32 MaxContainers, EDCStarChangeReason Reason)
{
	const UDCLevelConfigData* Config = BoundSubsystem.IsValid() ? BoundSubsystem->GetActiveConfig() : nullptr;
	const int32 InitialContainers = Config ? FMath::Max(1, Config->InitialStarContainers) : 3;
	const int32 TotalSlots = FMath::Max(InitialContainers, MaxContainers);
	const int32 ClampedMax = FMath::Clamp(MaxContainers, 0, TotalSlots);
	const int32 ClampedEarned = FMath::Clamp(EarnedStars, 0, ClampedMax);

	NewlyFilledIndex = (ClampedEarned > CachedEarnedStars) ? (ClampedEarned - 1) : INDEX_NONE;
	const bool bContainerLost = (Reason == EDCStarChangeReason::ContainerLost_CivilianInjured
		|| Reason == EDCStarChangeReason::ContainerLost_CivilianKilled
		|| ClampedMax < CachedMaxContainers);
	NewlyBurntIndex = (bContainerLost && ClampedMax < TotalSlots) ? ClampedMax : INDEX_NONE;

	CachedEarnedStars = ClampedEarned;
	CachedMaxContainers = ClampedMax;

	if (StarsGlyphRow && WidgetTree)
	{
		StarsGlyphRow->ClearChildren();
		StarWidgets.Reset();

		UTexture2D* EmptyTex = Config ? Config->StarEmptyTexture.Get() : nullptr;
		UTexture2D* FilledTex = Config ? Config->StarFilledTexture.Get() : nullptr;
		UTexture2D* BurntTex = Config ? Config->StarBurntTexture.Get() : nullptr;

		for (int32 Index = 0; Index < TotalSlots; ++Index)
		{
			UDCJournalStarWidget* Star = WidgetTree->ConstructWidget<UDCJournalStarWidget>();
			if (!Star)
			{
				continue;
			}

			EDCJournalStarState State = EDCJournalStarState::Empty;
			float InitialOpacity = 1.0f;
			if (Index < ClampedEarned)
			{
				State = EDCJournalStarState::Filled;
			}
			else if (Index >= ClampedMax)
			{
				State = EDCJournalStarState::Burnt;
				InitialOpacity = (Index == NewlyBurntIndex) ? 1.0f : 0.72f;
			}

			Star->SetStarDiameter(46.0f);
			Star->SetStarState(State);
			Star->SetVisualOpacity(InitialOpacity);
			Star->SetSparkleStrength((Index == NewlyFilledIndex) ? 1.0f : 0.0f);
			Star->SetBurntProgress((Index == NewlyBurntIndex) ? 0.0f : 1.0f);
			Star->SetTextureOverrides(EmptyTex, FilledTex, BurntTex);
			Star->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			Star->SetRenderTranslation(FVector2D::ZeroVector);
			Star->SetRenderTransformAngle(0.0f);
			const float InitialStarScale = (Index == NewlyFilledIndex) ? 1.30f : 1.0f;
			Star->SetRenderScale(FVector2D(InitialStarScale, InitialStarScale));

			StarWidgets.Add(Star);
			DCTheme::AddHBoxChild(
				StarsGlyphRow,
				Star,
				FMargin(Index > 0 ? 14.0f : 0.0f, 0.0f, 0.0f, 0.0f),
				HAlign_Center,
				VAlign_Center);
		}
	}

	// Optional BindWidgetOptional fields for custom Designer Blueprints (non-clapper copy)
	if (StarsGlyphText)
	{
		StarsGlyphText->SetText(FText::FromString(DCTheme::BuildStarGlyphText(ClampedEarned, ClampedMax)));
	}
	if (EventCaptionText)
	{
		FString Caption;
		switch (Reason)
		{
		case EDCStarChangeReason::FireExtinguished: Caption = TEXT("Fire Out!"); break;
		case EDCStarChangeReason::CivilianRescuedUninjured: Caption = TEXT("Saved Uninjured!"); break;
		case EDCStarChangeReason::CivilianRescuedInjured: Caption = TEXT("Saved!"); break;
		case EDCStarChangeReason::ContainerLost_CivilianInjured: Caption = TEXT("Injured in Fire"); break;
		case EDCStarChangeReason::ContainerLost_CivilianKilled: Caption = TEXT("Lost in Fire"); break;
		default: break;
		}
		EventCaptionText->SetText(FText::FromString(Caption));
	}
	if (ContainerStatusText)
	{
		ContainerStatusText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), ClampedEarned, ClampedMax)));
	}
}

void UDCStarPopupWidget::TriggerPopup(int32 EarnedStars, int32 MaxContainers, EDCStarChangeReason Reason, float DurationSeconds)
{
	if (Reason == EDCStarChangeReason::Initialized)
	{
		return;
	}

	if (BoundSubsystem.IsValid() && !BoundSubsystem->IsActiveTake())
	{
		DismissPopup();
		return;
	}

	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		EnsureDefaultLayout();
	}

	const UDCLevelConfigData* Config = BoundSubsystem.IsValid() ? BoundSubsystem->GetActiveConfig() : nullptr;
	PopAnimSeconds = Config ? FMath::Max(0.10f, Config->UiPopAnimSeconds) : 0.35f;

	RebuildStarIcons(EarnedStars, MaxContainers, Reason);

	if (Config)
	{
		if (NewlyFilledIndex != INDEX_NONE)
		{
			DCTheme::PlayUISound(this, Config->StarEarnedSound.Get());
		}
		if (NewlyBurntIndex != INDEX_NONE)
		{
			DCTheme::PlayUISound(this, Config->StarBurntSound.Get());
		}
	}

	DisplayDurationSeconds = FMath::Max(0.4f, DurationSeconds);
	ElapsedDisplaySeconds = 0.0f;
	RemainingDisplaySeconds = DisplayDurationSeconds;

	SetRenderOpacity(1.0f);
	if (PopupAccentBorder)
	{
		PopupAccentBorder->SetRenderScale(FVector2D(0.75f, 0.75f));
		PopupAccentBorder->SetRenderTranslation(FVector2D(0.0f, -10.0f));
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
	BP_OnStarPopupTriggered(EarnedStars, MaxContainers, Reason);
}

bool UDCStarPopupWidget::IsPopupVisible() const
{
	return GetVisibility() != ESlateVisibility::Collapsed
		&& GetVisibility() != ESlateVisibility::Hidden
		&& RemainingDisplaySeconds > 0.0f;
}

void UDCStarPopupWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (BoundSubsystem.IsValid() && !BoundSubsystem->IsActiveTake())
	{
		if (RemainingDisplaySeconds > 0.0f || GetVisibility() != ESlateVisibility::Collapsed)
		{
			DismissPopup();
		}
		return;
	}

	if (RemainingDisplaySeconds <= 0.0f)
	{
		return;
	}

	ElapsedDisplaySeconds += InDeltaTime;
	RemainingDisplaySeconds = FMath::Max(0.0f, DisplayDurationSeconds - ElapsedDisplaySeconds);

	// Pop-in BackOut curve on the pill container
	if (PopupAccentBorder)
	{
		const float PopAlpha = FMath::Clamp(ElapsedDisplaySeconds / PopAnimSeconds, 0.0f, 1.0f);
		const float c1 = 1.70158f;
		const float c3 = c1 + 1.0f;
		const float Tm1 = PopAlpha - 1.0f;
		const float BackOut = 1.0f + c3 * Tm1 * Tm1 * Tm1 + c1 * Tm1 * Tm1;
		const float Scale = FMath::Lerp(0.75f, 1.0f, BackOut);
		PopupAccentBorder->SetRenderScale(FVector2D(Scale, Scale));

		float TranslateY = FMath::Lerp(-10.0f, 0.0f, PopAlpha);
		float PillOpacity = 1.0f;
		if (RemainingDisplaySeconds < 0.30f)
		{
			const float OutAlpha = 1.0f - (RemainingDisplaySeconds / 0.30f);
			PillOpacity = 1.0f - OutAlpha;
			TranslateY -= 8.0f * OutAlpha;
		}
		PopupAccentBorder->SetRenderTranslation(FVector2D(0.0f, TranslateY));
		SetRenderOpacity(PillOpacity);
	}

	// Animate 1.30x -> 1.0x bounce & sparkle burst on newly filled star, and crack jiggle + smoke on newly burnt star
	bool bNeedsPaintInvalidate = false;
	if (StarWidgets.IsValidIndex(NewlyFilledIndex) && StarWidgets[NewlyFilledIndex])
	{
		UDCJournalStarWidget* FilledStar = StarWidgets[NewlyFilledIndex];
		const float BounceAlpha = FMath::Clamp(ElapsedDisplaySeconds / 0.35f, 0.0f, 1.0f);
		const float BounceScale = (BounceAlpha < 1.0f)
			? (1.0f + 0.30f * FMath::Cos(BounceAlpha * UE_PI * 1.5f) * FMath::Square(1.0f - BounceAlpha))
			: 1.0f;
		FilledStar->SetRenderScale(FVector2D(BounceScale, BounceScale));

		const float SparkleAlpha = FMath::Clamp(1.0f - (ElapsedDisplaySeconds / 0.75f), 0.0f, 1.0f);
		FilledStar->SetSparkleStrength(SparkleAlpha);
		if (ElapsedDisplaySeconds < 0.60f)
		{
			bNeedsPaintInvalidate = true;
		}
	}
	if (StarWidgets.IsValidIndex(NewlyBurntIndex) && StarWidgets[NewlyBurntIndex])
	{
		UDCJournalStarWidget* BurntStar = StarWidgets[NewlyBurntIndex];
		const float JiggleAlpha = FMath::Clamp(ElapsedDisplaySeconds / 0.35f, 0.0f, 1.0f);
		if (JiggleAlpha < 1.0f)
		{
			const float Damping = 1.0f - JiggleAlpha;
			const float Wave = FMath::Sin(JiggleAlpha * UE_PI * 6.0f) * Damping;
			BurntStar->SetRenderTranslation(FVector2D(Wave * 3.5f, FMath::Abs(Wave) * -1.5f));
			BurntStar->SetRenderTransformAngle(Wave * 8.0f);
		}
		else
		{
			BurntStar->SetRenderTranslation(FVector2D::ZeroVector);
			BurntStar->SetRenderTransformAngle(0.0f);
		}

		const float SmokeAlpha = FMath::Clamp(ElapsedDisplaySeconds / 0.75f, 0.0f, 1.0f);
		// Keep internal SDCJournalStar static puffs hidden during the popup so NativePaint's rising puffs animate cleanly
		BurntStar->SetBurntProgress(FMath::Min(0.95f, SmokeAlpha));
		const float FadeAlpha = FMath::Lerp(1.0f, 0.72f, SmokeAlpha);
		BurntStar->SetVisualOpacity(FadeAlpha);
		if (ElapsedDisplaySeconds < 0.80f)
		{
			bNeedsPaintInvalidate = true;
		}
	}

	if (bNeedsPaintInvalidate || RemainingDisplaySeconds < 0.35f)
	{
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	if (RemainingDisplaySeconds <= 0.0f)
	{
		DismissPopup();
	}
}

int32 UDCStarPopupWidget::NativePaint(
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

	if (RemainingDisplaySeconds <= 0.0f
		|| !IsPopupVisible()
		|| (BoundSubsystem.IsValid() && !BoundSubsystem->IsActiveTake()))
	{
		return MaxLayer;
	}

	const float WidgetAlpha = FMath::Clamp(GetRenderOpacity() * InWidgetStyle.GetColorAndOpacityTint().A, 0.0f, 1.0f);
	if (WidgetAlpha <= 0.01f)
	{
		return MaxLayer;
	}

	auto ComputeStarCenter = [&AllottedGeometry](const UDCJournalStarWidget* Star, FVector2f& OutCenter) -> bool
	{
		if (!Star)
		{
			return false;
		}
		const FGeometry StarGeom = Star->GetCachedGeometry();
		const FVector2f LocalSize = FVector2f(StarGeom.GetLocalSize());
		if (LocalSize.X <= 1.0f || LocalSize.Y <= 1.0f)
		{
			return false;
		}
		const FVector2f CenterAbs = StarGeom.LocalToAbsolute(LocalSize * 0.5f);
		OutCenter = AllottedGeometry.AbsoluteToLocal(CenterAbs);
		return true;
	};

	// Shot 03: 4 cute flat Butter + Ink sparkle dots radiating outward around newly earned star slot
	FVector2f FilledCenter;
	if (ElapsedDisplaySeconds < 0.55f
		&& StarWidgets.IsValidIndex(NewlyFilledIndex)
		&& ComputeStarCenter(StarWidgets[NewlyFilledIndex], FilledCenter))
	{
		const float Alpha = FMath::Clamp(ElapsedDisplaySeconds / 0.55f, 0.0f, 1.0f);
		const float EaseOut = 1.0f - FMath::Square(1.0f - Alpha);
		const float Fade = (Alpha < 0.70f) ? 1.0f : FMath::Clamp((1.0f - Alpha) / 0.30f, 0.0f, 1.0f);
		const float DotAlpha = Fade * WidgetAlpha;

		if (DotAlpha > 0.01f)
		{
			FLinearColor SparkleFill = DCTheme::Butter();
			SparkleFill.A *= DotAlpha;
			FLinearColor SparkleOutline = DCTheme::Ink();
			SparkleOutline.A *= DotAlpha;

			static constexpr float AnglesRad[4] = { -2.356194f, -0.785398f, 0.785398f, 2.356194f };
			static constexpr float RadiusScales[4] = { 1.0f, 0.85f, 1.0f, 0.85f };
			const float Travel = FMath::Lerp(18.0f, 36.0f, EaseOut);
			const float BaseRadius = FMath::Lerp(4.4f, 2.0f, Alpha * Alpha);

			for (int32 i = 0; i < 4; ++i)
			{
				const FVector2f Offset(FMath::Cos(AnglesRad[i]) * Travel, FMath::Sin(AnglesRad[i]) * Travel);
				DCTheme::PaintFilledCircle(
					OutDrawElements,
					++MaxLayer,
					AllottedGeometry,
					FilledCenter + Offset,
					BaseRadius * RadiusScales[i],
					SparkleFill,
					SparkleOutline,
					1.6f,
					14);
			}
		}
	}

	// Shot 06: 3 rising flat cartoon grey smoke puffs (#8C827A + thin Ink outline) drifting upward & shrinking over 0.75s
	FVector2f BurntCenter;
	if (ElapsedDisplaySeconds < 0.75f
		&& StarWidgets.IsValidIndex(NewlyBurntIndex)
		&& ComputeStarCenter(StarWidgets[NewlyBurntIndex], BurntCenter))
	{
		struct FSmokePuffSpec
		{
			float StartX;
			float StartY;
			float DriftX;
			float RiseHeight;
			float StartRadius;
			float DelayFrac;
		};
		static constexpr FSmokePuffSpec SmokePuffs[3] = {
			{ -9.0f, -12.0f, -5.0f, 30.0f, 6.2f, 0.00f },
			{  1.0f, -15.0f,  2.0f, 36.0f, 7.2f, 0.08f },
			{ 10.0f, -11.0f,  6.0f, 28.0f, 5.4f, 0.16f }
		};

		const float GlobalAlpha = FMath::Clamp(ElapsedDisplaySeconds / 0.75f, 0.0f, 1.0f);
		for (int32 i = 0; i < 3; ++i)
		{
			const FSmokePuffSpec& Puff = SmokePuffs[i];
			const float LocalAlpha = FMath::Clamp((GlobalAlpha - Puff.DelayFrac) / (1.0f - Puff.DelayFrac), 0.0f, 1.0f);
			if (LocalAlpha <= 0.0f || LocalAlpha >= 1.0f)
			{
				continue;
			}

			const float EaseOut = 1.0f - FMath::Square(1.0f - LocalAlpha);
			const float Radius = FMath::Lerp(Puff.StartRadius, 1.2f, LocalAlpha);
			const float PuffFade = (LocalAlpha < 0.65f) ? 1.0f : FMath::Clamp((1.0f - LocalAlpha) / 0.35f, 0.0f, 1.0f);
			const float AlphaTint = PuffFade * WidgetAlpha;
			if (AlphaTint <= 0.01f)
			{
				continue;
			}

			FLinearColor PuffFill = DCTheme::BurntSmoke();
			PuffFill.A *= AlphaTint;
			FLinearColor PuffOutline = DCTheme::Ink();
			PuffOutline.A *= AlphaTint;

			const FVector2f Pos = BurntCenter + FVector2f(
				Puff.StartX + Puff.DriftX * EaseOut,
				Puff.StartY - Puff.RiseHeight * EaseOut);

			DCTheme::PaintFilledCircle(
				OutDrawElements,
				++MaxLayer,
				AllottedGeometry,
				Pos,
				Radius,
				PuffFill,
				PuffOutline,
				1.6f,
				16);
		}
	}

	return MaxLayer;
}
