#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCUI.generated.h"
class ADCPlayer;
class UTextBlock;
class UVerticalBox;
class UBorder;

UCLASS()
class DIRECTORCUTJAM_API UDCDeleteButton : public UUserWidget
{
    GENERATED_BODY()
public:
    FName BlockID;
    UPROPERTY() TObjectPtr<ADCPlayer> Player;
    void Build(const FString& Caption);
    UFUNCTION() void Delete();
};

UCLASS()
class DIRECTORCUTJAM_API UDCOverlay : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<ADCPlayer> Player;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    UFUNCTION() void RefreshPersona();
    UFUNCTION() void TogglePersona();
    UFUNCTION() void Restart();
    UFUNCTION() void ClosePersona();
    void Refresh();
private:
    UPROPERTY() TObjectPtr<UTextBlock> Status;
    UPROPERTY() TObjectPtr<UTextBlock> Prompt;
    UPROPERTY() TObjectPtr<UTextBlock> Debug;
    UPROPERTY() TObjectPtr<UTextBlock> Result;
    UPROPERTY() TObjectPtr<UBorder> PersonaPanel;
    UPROPERTY() TObjectPtr<UBorder> ResultPanel;
    UPROPERTY() TObjectPtr<UVerticalBox> Blocks;
};

UCLASS()
class DIRECTORCUTJAM_API UDCThought : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    void SetThought(const FString& Value);
    FString CurrentThought;
private:
    UPROPERTY() TObjectPtr<UTextBlock> Text;
};
