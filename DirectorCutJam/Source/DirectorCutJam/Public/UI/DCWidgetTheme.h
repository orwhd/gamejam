// Retro Cute Journal UI theme tokens, rounded brushes, tactile widgets, and pure-Slate vector shapes.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Layout/Margin.h"
#include "Styling/SlateTypes.h"
#include "DCWidgetTheme.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UHorizontalBoxSlot;
class UImage;
class USizeBox;
class USoundBase;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UVerticalBoxSlot;
class UWidgetTree;
struct FSlateRoundedBoxBrush;

UENUM(BlueprintType)
enum class EDCJournalStarState : uint8
{
	Empty,
	Filled,
	Burnt
};

struct FDCOptionalTextBlockOut
{
	UTextBlock** Ptr = nullptr;

	FDCOptionalTextBlockOut() = default;
	explicit FDCOptionalTextBlockOut(UTextBlock*& InPtr) : Ptr(&InPtr) {}
	explicit FDCOptionalTextBlockOut(const TObjectPtr<UTextBlock>&) = delete;
};

namespace DCTheme
{
	// Exact Retro Cute Journal color palette (sRGB hex converted for linear DX12 rendering)
	FLinearColor HexColor(const TCHAR* Hex, float AlphaMultiplier = 1.0f);

	FLinearColor Paper();
	FLinearColor Ink();
	FLinearColor InkMuted();
	FLinearColor Peach();
	FLinearColor Blush();
	FLinearColor Sky();
	FLinearColor Butter();
	FLinearColor ButterShadow();
	FLinearColor Char();
	FLinearColor Highlighter();
	FLinearColor CuttableUnderline();
	FLinearColor Desk();
	FLinearColor TapeCream();
	FLinearColor TapePink();
	FLinearColor TapeBlue();
	FLinearColor KeycapShadow();
	FLinearColor CutTextMuted();
	FLinearColor DashedRule();
	FLinearColor SmokePuff();
	FLinearColor BurntSmoke();
	FLinearColor CalmResultTag();
	FLinearColor DimScrim();
	FLinearColor ReviewNoteFill();
	FLinearColor ReviewNoteBorder();

	// Smooth 2-stage overshoot settle (StartScale -> PeakScale -> EndScale)
	float EvaluateOvershootScale(
		float Alpha,
		float StartScale,
		float PeakScale,
		float EndScale = 1.0f,
		float PeakAlpha = 0.65f);

	// Play a 2D UI sound only when Sound is non-null inside a valid game world
	void PlayUISound(const UObject* WorldContext, USoundBase* Sound);

	// Dynamically load optional /Game/Textures/Interactions/T_UI_AngelDogSticker without warning if absent
	UTexture2D* LoadOptionalAngelDogSticker();

	// Apply optional sticker texture to a UImage, collapsing cleanly if absent or hidden
	void ApplyOptionalStickerImage(UImage* StickerImage, bool bShouldShow);

	FSlateRoundedBoxBrush MakeRoundedBoxBrush(
		const FLinearColor& FillColor,
		float CornerRadius,
		const FLinearColor& OutlineColor = FLinearColor::Transparent,
		float OutlineWidth = 0.0f);

	void ApplyRoundedBorderStyle(
		UBorder* Border,
		const FLinearColor& FillColor,
		float CornerRadius,
		const FLinearColor& OutlineColor = FLinearColor::Transparent,
		float OutlineWidth = 0.0f,
		const FMargin& Padding = FMargin(0.0f));

	UTextBlock* MakeText(
		UWidgetTree* Tree,
		const FString& InitialText,
		int32 FontSize,
		const FLinearColor& Color,
		bool bBold = false,
		bool bAutoWrap = false);

	UBorder* MakeJournalBorder(
		UWidgetTree* Tree,
		UWidget* Content,
		const FLinearColor& FillColor,
		float CornerRadius,
		const FLinearColor& OutlineColor,
		float OutlineWidth,
		const FMargin& Padding);

	USizeBox* MakeCornerTape(
		UWidgetTree* Tree,
		const FLinearColor& TapeColor,
		float AngleDegrees,
		float Width = 146.0f,
		float Height = 42.0f);

	UBorder* MakeChubbyKeycap(
		UWidgetTree* Tree,
		const FString& KeyText,
		float BoxSize = 36.0f,
		int32 FontSize = 16,
		float CornerRadius = 10.0f,
		float OutlineWidth = 3.0f,
		UTextBlock** OutKeyText = nullptr);

	UBorder* MakeBadgePill(
		UWidgetTree* Tree,
		const FString& Label,
		const FLinearColor& FillColor,
		const FLinearColor& TextColor,
		int32 FontSize = 18,
		const FLinearColor& OutlineColor = FLinearColor::Transparent,
		float OutlineWidth = 3.0f,
		const FMargin& Padding = FMargin(16.0f, 5.0f),
		UTextBlock** OutLabel = nullptr);

	UBorder* MakeChubbyButton(
		UWidgetTree* Tree,
		UButton*& OutButton,
		const FText& Label,
		const FLinearColor& FillColor,
		const FLinearColor& HoverFillColor,
		const FLinearColor& LipColor,
		const FLinearColor& TextColor,
		int32 FontSize = 22,
		float CornerRadius = 999.0f,
		float OutlineWidth = 4.0f,
		const FMargin& ContentPadding = FMargin(28.0f, 10.0f),
		FDCOptionalTextBlockOut OutLabel = FDCOptionalTextBlockOut());

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
		UTextBlock** OutLabelText = nullptr);

	UVerticalBoxSlot* AddVBoxChild(
		UVerticalBox* Box,
		UWidget* Child,
		const FMargin& Padding = FMargin(0.0f),
		EHorizontalAlignment HAlign = HAlign_Fill,
		EVerticalAlignment VAlign = VAlign_Center,
		ESlateSizeRule::Type SizeRule = ESlateSizeRule::Automatic,
		float FillValue = 1.0f);

	UHorizontalBoxSlot* AddHBoxChild(
		UHorizontalBox* Box,
		UWidget* Child,
		const FMargin& Padding = FMargin(0.0f),
		EHorizontalAlignment HAlign = HAlign_Left,
		EVerticalAlignment VAlign = VAlign_Center,
		ESlateSizeRule::Type SizeRule = ESlateSizeRule::Automatic,
		float FillValue = 1.0f);

	FString BuildStarGlyphText(int32 EarnedStars, int32 MaxContainers);

	// Shared pure-Slate vector draw primitives for custom journal shapes
	void PaintFilledCircle(
		FSlateWindowElementList& OutDrawElements,
		uint32 LayerId,
		const FGeometry& AllottedGeometry,
		const FVector2f& CenterLocal,
		float Radius,
		const FLinearColor& FillColor,
		const FLinearColor& OutlineColor = FLinearColor::Transparent,
		float OutlineThickness = 0.0f,
		int32 NumSegments = 24);

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
		int32 NumSegments = 48,
		bool bRoundEndCaps = true);
}

/**
 * Pure-Slate vector chubby 5-point star supporting Empty, Filled, and Burnt (cracked + smoke) states,
 * plus optional artist UTexture2D overrides from LevelConfigData.
 */
UCLASS(BlueprintType)
class DIRECTORCUTJAM_API UDCJournalStarWidget : public UWidget
{
	GENERATED_BODY()

public:
	UDCJournalStarWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Journal UI")
	void SetStarState(EDCJournalStarState NewState);

	UFUNCTION(BlueprintPure, Category = "Journal UI")
	EDCJournalStarState GetStarState() const { return StarState; }

	UFUNCTION(BlueprintCallable, Category = "Journal UI")
	void SetStarDiameter(float NewDiameter);

	UFUNCTION(BlueprintCallable, Category = "Journal UI")
	void SetVisualOpacity(float NewOpacity);

	UFUNCTION(BlueprintCallable, Category = "Journal UI")
	void SetSparkleStrength(float NewSparkleStrength);

	UFUNCTION(BlueprintCallable, Category = "Journal UI")
	void SetBurntProgress(float NewBurntProgress);

	void SetTextureOverrides(UTexture2D* InEmptyTexture, UTexture2D* InFilledTexture, UTexture2D* InBurntTexture);

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal UI")
	EDCJournalStarState StarState = EDCJournalStarState::Empty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal UI")
	float StarDiameter = 44.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal UI")
	float VisualOpacity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal UI")
	float SparkleStrength = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal UI")
	float BurntProgress = 1.0f;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> EmptyTextureOverride = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> FilledTextureOverride = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> BurntTextureOverride = nullptr;

private:
	class SDCJournalStar;
	TSharedPtr<SDCJournalStar> MyStarWidget;
};

/**
 * Pure-Slate vector cute minimal scissors icon shown when hovering cuttable Persona blocks.
 */
UCLASS(BlueprintType)
class DIRECTORCUTJAM_API UDCJournalScissorsWidget : public UWidget
{
	GENERATED_BODY()

public:
	UDCJournalScissorsWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Journal UI")
	void SetIconSize(float NewSize);

	UFUNCTION(BlueprintCallable, Category = "Journal UI")
	void SetTint(const FLinearColor& NewTint);

	void SetTextureOverride(UTexture2D* InScissorsTexture);

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal UI")
	float IconSize = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal UI")
	FLinearColor IconTint = FLinearColor::White;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> ScissorsTextureOverride = nullptr;

private:
	class SDCJournalScissors;
	TSharedPtr<SDCJournalScissors> MyScissorsWidget;
};

/**
 * Pure-Slate crisp dashed horizontal rule matching the warm journal notebook dividers (#D8C7AD).
 */
UCLASS(BlueprintType)
class DIRECTORCUTJAM_API UDCDashedRuleWidget : public UWidget
{
	GENERATED_BODY()

public:
	UDCDashedRuleWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Journal UI")
	void ConfigureRule(const FLinearColor& NewColor, float NewThickness = 3.0f, float NewDashLength = 9.0f, float NewHeight = 10.0f);

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal UI")
	FLinearColor RuleColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal UI")
	float RuleThickness = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal UI")
	float DashLengthPx = 9.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal UI")
	float DesiredHeight = 10.0f;

private:
	class SDCDashedRule;
	TSharedPtr<SDCDashedRule> MyRuleWidget;
};
