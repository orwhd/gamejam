#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "Blueprint/UserWidget.h"
#include "Core/DCTypes.h"
#include "DCLevelConfigData.generated.h"

class UDCPersonaDefinition;
class USoundBase;
class UTexture2D;

UCLASS(BlueprintType)
class DIRECTORCUTJAM_API UDCLevelConfigData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UDCLevelConfigData();

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stage")
    FText SceneTitle;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stage")
    TObjectPtr<UDCPersonaDefinition> DefaultPersonaAsset;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stage")
    FText LevelDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Timings", meta=(ClampMin="0.1"))
    float SecondsToInjureCivilian = 9.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Timings", meta=(ClampMin="0.1"))
    float SecondsToKillCivilian = 19.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Timings", meta=(ClampMin="0.1"))
    float SecondsToEngulfRoom = 20.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Timings", meta=(ClampMin="0.0"))
    float ExtinguishActionSeconds = 1.6f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire Timings", meta=(ClampMin="0.0"))
    float EngulfDelayPerSpotSeconds = 3.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Star Rules", meta=(ClampMin="1", ClampMax="10"))
    int32 InitialStarContainers = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Star Rules", meta=(ClampMin="0"))
    int32 StarsForExtinguish = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Star Rules", meta=(ClampMin="0"))
    int32 StarsForRescueAlive = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Star Rules", meta=(ClampMin="0"))
    int32 BonusStarsForRescueUninjured = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Star Rules", meta=(ClampMin="0"))
    int32 ContainerPenaltyOnInjured = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Star Rules", meta=(ClampMin="0"))
    int32 ContainerPenaltyOnKilled = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Star Rules", meta=(ClampMin="0.2"))
    float StarPopupDisplaySeconds = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement & Interaction", meta=(ClampMin="10.0"))
    float PlayerWalkSpeed = 320.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement & Interaction", meta=(ClampMin="20.0"))
    float DefaultInteractionRadius = 175.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement & Interaction", meta=(ClampMin="20.0"))
    float EscortFollowDistance = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement & Interaction", meta=(ClampMin="10.0"))
    float EscortMoveSpeed = 295.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement & Interaction", meta=(ClampMin="10.0"))
    float WeakEscortMoveSpeed = 130.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement & Interaction", meta=(ClampMin="0.2"))
    float ThoughtBubbleDurationSeconds = 3.8f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta=(ClampMin="55.0", ClampMax="65.0", UIMin="55.0", UIMax="65.0"))
    float CameraFOV = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera")
    FVector CameraWorldLocation = FVector(-320.0f, -180.0f, 275.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera")
    FRotator CameraWorldRotation = FRotator(-15.5f, 0.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera")
    bool bEnableSmoothPanFollow = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera")
    FVector2D CameraPanFollowOffsetXY = FVector2D(-740.0f, 110.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera")
    FVector2D CameraWorldBoundsMinXY = FVector2D(-350.0f, -210.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera")
    FVector2D CameraWorldBoundsMaxXY = FVector2D(-190.0f, 75.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera", meta=(ClampMin="0.1"))
    float CameraPanInterpSpeed = 4.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    TSubclassOf<UUserWidget> HudOverlayWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    TSubclassOf<UUserWidget> ThoughtBubbleWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    TSubclassOf<UUserWidget> PreTakeScriptWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    TSubclassOf<UUserWidget> InteractPromptWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    TSubclassOf<UUserWidget> TakeResultWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    FText PersonaViewDisallowedHintText;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    FText ActionButtonLabel;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    FText RetakeHintLabel;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    FText RetakeButtonLabel;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    FText LegendFixedLabel;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    FText LegendCuttableLabel;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    FText LegendCutLabel;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    FText ResultFiresRowLabel;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    FText ResultFiresAllOutValue;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    FText ResultFiresRemainingFormat;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    FText ResultCivilianRowLabel;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    FText ResultCivilianUninjuredValue;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    FText ResultCivilianInjuredValue;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    FText ResultCivilianLostValue;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides", meta=(ClampMin="0.05"))
    float UiPopAnimSeconds = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    TObjectPtr<UTexture2D> StarEmptyTexture;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    TObjectPtr<UTexture2D> StarFilledTexture;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    TObjectPtr<UTexture2D> StarBurntTexture;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI Overrides")
    TObjectPtr<UTexture2D> ScissorsCursorTexture;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio (Prototype SFX)")
    TObjectPtr<USoundBase> PaperOpenSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio (Prototype SFX)")
    TObjectPtr<USoundBase> MarkerSwipeSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio (Prototype SFX)")
    TObjectPtr<USoundBase> ActionStartSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio (Prototype SFX)")
    TObjectPtr<USoundBase> ThoughtBubbleSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio (Prototype SFX)")
    TObjectPtr<USoundBase> DoorOpenSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio (Prototype SFX)")
    TObjectPtr<USoundBase> PickupToolSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio (Prototype SFX)")
    TObjectPtr<USoundBase> WaterSpraySound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio (Prototype SFX)")
    TObjectPtr<USoundBase> FireExtinguishedSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio (Prototype SFX)")
    TObjectPtr<USoundBase> StarEarnedSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio (Prototype SFX)")
    TObjectPtr<USoundBase> StarBurntSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio (Prototype SFX)")
    TObjectPtr<USoundBase> FootstepSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio (Prototype SFX)")
    TObjectPtr<USoundBase> StageBgmSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Paper Visuals")
    TObjectPtr<UTexture2D> PlayerDefaultPaperTexture;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Paper Visuals")
    TArray<TObjectPtr<UTexture2D>> PlayerIdleCycleTextures;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Paper Visuals")
    TArray<TObjectPtr<UTexture2D>> PlayerWalkCycleTextures;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Paper Visuals")
    TObjectPtr<UTexture2D> PlayerScaredTexture;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Paper Visuals")
    TObjectPtr<UTexture2D> PlayerToolIdleTexture;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Paper Visuals")
    TArray<TObjectPtr<UTexture2D>> PlayerToolWalkCycleTextures;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Paper Visuals")
    TArray<TObjectPtr<UTexture2D>> PlayerSprayCycleTextures;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Paper Visuals")
    TObjectPtr<UTexture2D> PlayerEscortUninjuredIdleTexture;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Paper Visuals")
    TArray<TObjectPtr<UTexture2D>> PlayerEscortUninjuredWalkCycleTextures;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Paper Visuals")
    TObjectPtr<UTexture2D> PlayerEscortInjuredIdleTexture;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Paper Visuals")
    TArray<TObjectPtr<UTexture2D>> PlayerEscortInjuredWalkCycleTextures;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Paper Visuals")
    TObjectPtr<UTexture2D> PlayerWaterJetOverlayTexture;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Paper Visuals")
    TObjectPtr<UTexture2D> BuddyUninjuredTexture;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Paper Visuals")
    TObjectPtr<UTexture2D> BuddyInjuredTexture;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TBD Policies (Pending Design)")
    EDCExtinguishInterruptPolicy ExtinguishInterruptPolicy = EDCExtinguishInterruptPolicy::KeepStatic;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TBD Policies (Pending Design)", meta=(ClampMin="0.0"))
    float ExtinguishProgressDecayPerSecond = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TBD Policies (Pending Design)", meta=(ClampMin="0.0"))
    float ExtraExtinguishSecondsPerFireStage = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TBD Policies (Pending Design)")
    EDCEscortToolPolicy EscortToolPolicy = EDCEscortToolPolicy::HideToolAndDisableSpray;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TBD Policies (Pending Design)")
    bool bScaleFireBlockerWithStage = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TBD Policies (Pending Design)", meta=(ClampMin="1.0", ClampMax="2.0"))
    float FireBlockerStageScaleMultiplier = 1.15f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TBD Policies (Pending Design)")
    EDCActiveTakePersonaViewPolicy ActiveTakePersonaViewPolicy = EDCActiveTakePersonaViewPolicy::AllowTabReadOnlyPeek;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TBD Policies (Pending Design)")
    EDCCharacterVisualMode ProtagonistVisualMode = EDCCharacterVisualMode::Paper2D;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TBD Policies (Pending Design)", meta=(ClampMin="1", ClampMax="10"))
    int32 ActiveFireSpotCount = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TBD Policies (Pending Design)")
    bool bEnableOptionalDailyBlockB = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TBD Policies (Pending Design)")
    EDCFireVisualMode FireVisualMode = EDCFireVisualMode::Paper2DPlaceholder;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TBD Policies (Pending Design)")
    EDCDoorTraversalPolicy DoorTraversalPolicy = EDCDoorTraversalPolicy::FreeWalkAfterBreakthrough;
};
