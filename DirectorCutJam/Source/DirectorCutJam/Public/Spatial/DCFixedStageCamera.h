#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "DCFixedStageCamera.generated.h"

class APlayerController;
class UDCLevelConfigData;

/**
 * Single dollhouse fixed-angle stage camera.
 * Enforces confirmed FOV [55, 65], downward pitch [-20, -10] with Roll = 0, fixed Z,
 * and smooth clamped XY pan-following without dynamic tilt, zoom, or Z-bob.
 */
UCLASS(Blueprintable)
class DIRECTORCUTJAM_API ADCFixedStageCamera : public ACameraActor
{
    GENERATED_BODY()

public:
    ADCFixedStageCamera();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Camera")
    void ApplyConfig(const UDCLevelConfigData* Config);

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Camera")
    void SnapToPlayerNow();

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Camera")
    void ActivateForPlayer(APlayerController* PlayerController);

    UFUNCTION(BlueprintPure, Category="DirectorCut|Camera")
    FVector GetCameraPlaneForward() const;

    UFUNCTION(BlueprintPure, Category="DirectorCut|Camera")
    FVector GetCameraPlaneRight() const;

private:
    UFUNCTION()
    void HandleTakeReset();

    const APawn* FindFollowPawn() const;
    FVector2D ComputeDesiredCameraXY() const;

    FVector BaseLocation = FVector(-320.0f, -180.0f, 275.0f);
    FRotator LockedRotation = FRotator(-15.5f, 0.0f, 0.0f);
    bool bEnableSmoothPanFollow = true;
    FVector2D PanFollowOffsetXY = FVector2D(-740.0f, 110.0f);
    FVector2D BoundsMinXY = FVector2D(-350.0f, -210.0f);
    FVector2D BoundsMaxXY = FVector2D(-190.0f, 75.0f);
    float PanInterpSpeed = 4.5f;
    bool bHasAppliedConfig = false;
};
