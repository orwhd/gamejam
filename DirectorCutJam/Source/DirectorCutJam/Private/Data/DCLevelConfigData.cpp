#include "Data/DCLevelConfigData.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

UDCLevelConfigData::UDCLevelConfigData()
{
    SceneTitle = NSLOCTEXT("DirectorCut", "DefaultSceneTitle", "Fire Call");
    LevelDisplayName = NSLOCTEXT("DirectorCut", "DefaultLevelDisplayName", "Fire Call");
    PersonaViewDisallowedHintText = NSLOCTEXT(
        "DirectorCut",
        "PersonaViewDisallowedHint",
        "Persona script locked during action. Press [R] to Retake.");
    ActionButtonLabel = NSLOCTEXT("DirectorCut", "DefaultActionButtonLabel", "ACTION!");
    RetakeHintLabel = NSLOCTEXT("DirectorCut", "DefaultRetakeHintLabel", "Retake");
    RetakeButtonLabel = NSLOCTEXT("DirectorCut", "DefaultRetakeButtonLabel", "[R] Retake");
    LegendFixedLabel = NSLOCTEXT("DirectorCut", "DefaultLegendFixedLabel", "FIXED");
    LegendCuttableLabel = NSLOCTEXT("DirectorCut", "DefaultLegendCuttableLabel", "CUTTABLE");
    LegendCutLabel = NSLOCTEXT("DirectorCut", "DefaultLegendCutLabel", "CUT");
    ResultFiresRowLabel = NSLOCTEXT("DirectorCut", "DefaultResultFiresRowLabel", "FIRES");
    ResultFiresAllOutValue = NSLOCTEXT("DirectorCut", "DefaultResultFiresAllOutValue", "ALL OUT");
    ResultFiresRemainingFormat = NSLOCTEXT("DirectorCut", "DefaultResultFiresRemainingFormat", "{0} REMAINING");
    ResultCivilianRowLabel = NSLOCTEXT("DirectorCut", "DefaultResultCivilianRowLabel", "CIVILIAN");
    ResultCivilianUninjuredValue = NSLOCTEXT("DirectorCut", "DefaultResultCivilianUninjuredValue", "SAVED UNINJURED");
    ResultCivilianInjuredValue = NSLOCTEXT("DirectorCut", "DefaultResultCivilianInjuredValue", "SAVED INJURED");
    ResultCivilianLostValue = NSLOCTEXT("DirectorCut", "DefaultResultCivilianLostValue", "LOST IN FIRE");
}

#if WITH_EDITOR
void UDCLevelConfigData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    CameraFOV = FMath::Clamp(CameraFOV, 55.0f, 65.0f);
    CameraWorldRotation.Pitch = FMath::Clamp(CameraWorldRotation.Pitch, -20.0f, -10.0f);
    CameraWorldRotation.Roll = 0.0f;
    CameraPanInterpSpeed = FMath::Max(0.1f, CameraPanInterpSpeed);
    CameraWorldBoundsMaxXY.X = FMath::Max(CameraWorldBoundsMinXY.X, CameraWorldBoundsMaxXY.X);
    CameraWorldBoundsMaxXY.Y = FMath::Max(CameraWorldBoundsMinXY.Y, CameraWorldBoundsMaxXY.Y);
    PlayerWalkSpeed = FMath::Max(10.0f, PlayerWalkSpeed);
    EscortMoveSpeed = FMath::Max(10.0f, EscortMoveSpeed);
    WeakEscortMoveSpeed = FMath::Max(10.0f, WeakEscortMoveSpeed);
    SecondsToInjureCivilian = FMath::Max(0.1f, SecondsToInjureCivilian);
    SecondsToKillCivilian = FMath::Max(SecondsToInjureCivilian, SecondsToKillCivilian);
    SecondsToEngulfRoom = FMath::Max(SecondsToKillCivilian, SecondsToEngulfRoom);
    ExtinguishActionSeconds = FMath::Max(0.0f, ExtinguishActionSeconds);
    EngulfDelayPerSpotSeconds = FMath::Max(0.0f, EngulfDelayPerSpotSeconds);
    StarPopupDisplaySeconds = FMath::Max(0.2f, StarPopupDisplaySeconds);
    ThoughtBubbleDurationSeconds = FMath::Max(0.2f, ThoughtBubbleDurationSeconds);
    UiPopAnimSeconds = FMath::Max(0.05f, UiPopAnimSeconds);
    ExtinguishProgressDecayPerSecond = FMath::Max(0.0f, ExtinguishProgressDecayPerSecond);
    ExtraExtinguishSecondsPerFireStage = FMath::Max(0.0f, ExtraExtinguishSecondsPerFireStage);
    FireBlockerStageScaleMultiplier = FMath::Clamp(FireBlockerStageScaleMultiplier, 1.0f, 2.0f);
    ActiveFireSpotCount = FMath::Max(1, ActiveFireSpotCount);
    InitialStarContainers = FMath::Clamp(InitialStarContainers, 1, 10);
}
#endif
