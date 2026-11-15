#include "Spatial/DCFixedStageCamera.h"
#include "Camera/CameraComponent.h"
#include "Core/DCTakeSubsystem.h"
#include "Data/DCLevelConfigData.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ADCFixedStageCamera::ADCFixedStageCamera()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    BaseLocation = FVector(-320.0f, -180.0f, 275.0f);
    LockedRotation = FRotator(-15.5f, 0.0f, 0.0f);

    SetActorLocationAndRotation(BaseLocation, LockedRotation);

    if (UCameraComponent* Cam = GetCameraComponent())
    {
        Cam->SetProjectionMode(ECameraProjectionMode::Perspective);
        Cam->SetFieldOfView(60.0f);
        Cam->SetConstraintAspectRatio(false);
    }
}

void ADCFixedStageCamera::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetWorld();
    if (World && World->IsGameWorld())
    {
        if (UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            TakeSubsystem->OnTakeReset.RemoveDynamic(this, &ADCFixedStageCamera::HandleTakeReset);
            TakeSubsystem->OnTakeReset.AddDynamic(this, &ADCFixedStageCamera::HandleTakeReset);

            if (!bHasAppliedConfig)
            {
                if (const UDCLevelConfigData* ActiveConfig = TakeSubsystem->GetActiveConfig())
                {
                    ApplyConfig(ActiveConfig);
                }
            }
        }
    }
}

void ADCFixedStageCamera::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            TakeSubsystem->OnTakeReset.RemoveDynamic(this, &ADCFixedStageCamera::HandleTakeReset);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void ADCFixedStageCamera::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        return;
    }

    const FVector CurrentLocation = GetActorLocation();
    const FVector2D CurrentXY(CurrentLocation.X, CurrentLocation.Y);
    const FVector2D DesiredXY = ComputeDesiredCameraXY();
    const FVector2D NextXY = FMath::Vector2DInterpTo(
        CurrentXY,
        DesiredXY,
        FMath::Max(0.0f, DeltaSeconds),
        FMath::Max(0.1f, PanInterpSpeed));

    SetActorLocationAndRotation(FVector(NextXY.X, NextXY.Y, BaseLocation.Z), LockedRotation);
}

void ADCFixedStageCamera::ApplyConfig(const UDCLevelConfigData* Config)
{
    FVector TargetLocation = GetActorLocation();
    FRotator TargetRotation = GetActorRotation();
    float TargetFOV = 60.0f;

    if (Config)
    {
        TargetLocation = Config->CameraWorldLocation;
        TargetRotation = Config->CameraWorldRotation;
        TargetFOV = Config->CameraFOV;

        bEnableSmoothPanFollow = Config->bEnableSmoothPanFollow;
        PanFollowOffsetXY = Config->CameraPanFollowOffsetXY;
        BoundsMinXY = Config->CameraWorldBoundsMinXY;
        BoundsMaxXY = Config->CameraWorldBoundsMaxXY;
        BoundsMaxXY.X = FMath::Max(BoundsMinXY.X, BoundsMaxXY.X);
        BoundsMaxXY.Y = FMath::Max(BoundsMinXY.Y, BoundsMaxXY.Y);
        PanInterpSpeed = FMath::Max(0.1f, Config->CameraPanInterpSpeed);
        bHasAppliedConfig = true;
    }
    else if (const UCameraComponent* Cam = GetCameraComponent())
    {
        TargetFOV = Cam->FieldOfView;
    }

    TargetRotation.Pitch = FMath::Clamp(TargetRotation.Pitch, -20.0f, -10.0f);
    TargetRotation.Roll = 0.0f;
    const float ClampedFOV = FMath::Clamp(TargetFOV, 55.0f, 65.0f);

    BaseLocation = TargetLocation;
    LockedRotation = TargetRotation;

    if (UCameraComponent* Cam = GetCameraComponent())
    {
        Cam->SetProjectionMode(ECameraProjectionMode::Perspective);
        Cam->SetFieldOfView(ClampedFOV);
        Cam->SetConstraintAspectRatio(false);
    }

    SnapToPlayerNow();
}

void ADCFixedStageCamera::SnapToPlayerNow()
{
    const FVector2D DesiredXY = ComputeDesiredCameraXY();
    SetActorLocationAndRotation(FVector(DesiredXY.X, DesiredXY.Y, BaseLocation.Z), LockedRotation);
}

void ADCFixedStageCamera::HandleTakeReset()
{
    SnapToPlayerNow();
}

const APawn* ADCFixedStageCamera::FindFollowPawn() const
{
    const UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        return nullptr;
    }

    return UGameplayStatics::GetPlayerPawn(World, 0);
}

FVector2D ADCFixedStageCamera::ComputeDesiredCameraXY() const
{
    if (bEnableSmoothPanFollow)
    {
        if (const APawn* PlayerPawn = FindFollowPawn())
        {
            const FVector PawnLoc = PlayerPawn->GetActorLocation();
            return FVector2D(
                FMath::Clamp(PawnLoc.X + PanFollowOffsetXY.X, BoundsMinXY.X, BoundsMaxXY.X),
                FMath::Clamp(PawnLoc.Y + PanFollowOffsetXY.Y, BoundsMinXY.Y, BoundsMaxXY.Y));
        }
    }

    return FVector2D(BaseLocation.X, BaseLocation.Y);
}

void ADCFixedStageCamera::ActivateForPlayer(APlayerController* PlayerController)
{
    if (IsValid(PlayerController) && PlayerController->GetViewTarget() != this)
    {
        PlayerController->SetViewTarget(this);
    }
}

FVector ADCFixedStageCamera::GetCameraPlaneForward() const
{
    const FRotator YawOnly(0.0f, GetActorRotation().Yaw, 0.0f);
    return YawOnly.Vector().GetSafeNormal();
}

FVector ADCFixedStageCamera::GetCameraPlaneRight() const
{
    const FRotator YawOnly(0.0f, GetActorRotation().Yaw, 0.0f);
    return FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y).GetSafeNormal();
}
