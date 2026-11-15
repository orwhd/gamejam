#include "Level1/DCStageDirector.h"
#include "Characters/DCPlayerCharacter.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/DCTakeSubsystem.h"
#include "Data/DCLevelConfigData.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Level1/DCFirefighterActors.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "Spatial/DCFixedStageCamera.h"
#include "UObject/ConstructorHelpers.h"

ADCGameMode::ADCGameMode()
{
    DefaultPawnClass = ADCPlayerCharacter::StaticClass();
}

ADCStageDirector::ADCStageDirector()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
    UStaticMesh* CubeMesh = CubeFinder.Object;
    UStaticMesh* CylinderMesh = CylinderFinder.Object;
    UStaticMesh* SphereMesh = SphereFinder.Object;
    UStaticMesh* PlaneMesh = PlaneFinder.Object;

    // Interior volume: X in [0, 960], Y in [-300, 420], Z in [0, 340].
    FloorMesh = CreateBoxMesh(TEXT("Floor"), FVector(480.0f, 60.0f, -10.0f), FVector(960.0f, 720.0f, 20.0f), true, CubeMesh);
    CeilingMesh = CreateBoxMesh(TEXT("Ceiling"), FVector(525.0f, 60.0f, 355.0f), FVector(870.0f, 720.0f, 30.0f), true, CubeMesh);
    BackWallMesh = CreateBoxMesh(TEXT("BackWall"), FVector(972.0f, 60.0f, 170.0f), FVector(24.0f, 768.0f, 340.0f), true, CubeMesh);
    RightWallMesh = CreateBoxMesh(TEXT("RightWall"), FVector(480.0f, 432.0f, 170.0f), FVector(960.0f, 24.0f, 340.0f), true, CubeMesh);

    // Open left doorway: X in [170, 530], Y = -300, Z in [0, 260].
    LeftWallFrontMesh = CreateBoxMesh(TEXT("LeftWallFront"), FVector(85.0f, -312.0f, 170.0f), FVector(170.0f, 24.0f, 340.0f), true, CubeMesh);
    // Dollhouse cutaway sightline window on rear wall (X in [530, 960], Y = -312): wainscot + header + sill trim + full-height pawn blocker.
    LeftWallRearMesh = CreateBoxMesh(TEXT("LeftWallRear"), FVector(745.0f, -312.0f, 39.0f), FVector(430.0f, 24.0f, 78.0f), false, CubeMesh);
    LeftWallRearHeaderMesh = CreateBoxMesh(TEXT("LeftWallRearHeader"), FVector(745.0f, -312.0f, 300.0f), FVector(430.0f, 24.0f, 80.0f), false, CubeMesh);
    LeftWallRearSillMesh = CreateBoxMesh(TEXT("LeftWallRearSill"), FVector(745.0f, -312.0f, 81.0f), FVector(430.0f, 32.0f, 6.0f), false, CubeMesh);
    DoorHeaderMesh = CreateBoxMesh(TEXT("DoorHeader"), FVector(350.0f, -312.0f, 300.0f), FVector(360.0f, 24.0f, 80.0f), true, CubeMesh);

    // Doorframe architectural trim around open left doorway X in [170, 530], Z in [0, 260].
    DoorFrameFrontJambMesh = CreateBoxMesh(TEXT("DoorFrameFrontJamb"), FVector(176.0f, -312.0f, 130.0f), FVector(12.0f, 34.0f, 260.0f), false, CubeMesh);
    DoorFrameRearJambMesh = CreateBoxMesh(TEXT("DoorFrameRearJamb"), FVector(524.0f, -312.0f, 130.0f), FVector(12.0f, 34.0f, 260.0f), false, CubeMesh);
    DoorFrameLintelMesh = CreateBoxMesh(TEXT("DoorFrameLintel"), FVector(350.0f, -312.0f, 254.0f), FVector(360.0f, 36.0f, 14.0f), false, CubeMesh);

    // Low knee-height partitions forming S-curve corridor + shortcut layout without occluding 5-15 deg sightlines.
    PartitionB1Mesh = CreateBoxMesh(TEXT("PartitionB1"), FVector(560.0f, 140.0f, 23.0f), FVector(30.0f, 560.0f, 46.0f), true, CubeMesh);
    PartitionB2Mesh = CreateBoxMesh(TEXT("PartitionB2"), FVector(760.0f, 35.0f, 23.0f), FVector(30.0f, 350.0f, 46.0f), true, CubeMesh);

    // Expanded exterior porch + front floor apron + cozy courtyard enclosure (eliminating black background voids).
    PorchFloorMesh = CreateBoxMesh(TEXT("PorchFloor"), FVector(450.0f, -440.0f, -10.0f), FVector(1020.0f, 280.0f, 20.0f), true, CubeMesh);
    FrontFloorApronMesh = CreateBoxMesh(TEXT("FrontFloorApron"), FVector(-190.0f, -68.0f, -10.2f), FVector(380.0f, 1024.0f, 20.0f), false, CubeMesh);
    ExteriorBackWallMesh = CreateBoxMesh(TEXT("ExteriorBackWall"), FVector(972.2f, -440.0f, 170.0f), FVector(24.0f, 300.0f, 360.0f), true, CubeMesh);
    ExteriorLeftWallMesh = CreateBoxMesh(TEXT("ExteriorLeftWall"), FVector(450.0f, -592.0f, 170.0f), FVector(1020.0f, 24.0f, 360.0f), true, CubeMesh);

    // Cozy living-room set-dressing (strictly NoCollision and <= 46cm in interior walk lanes).
    AreaRugMesh = CreateBoxMesh(TEXT("AreaRug"), FVector(510.0f, 60.0f, 0.3f), FVector(520.0f, 400.0f, 0.6f), false, CubeMesh);
    if (AreaRugMesh)
    {
        AreaRugMesh->SetCastShadow(false);
    }

    // 12cm tall wooden baseboards along walls.
    BaseboardBackMesh = CreateBoxMesh(TEXT("BaseboardBack"), FVector(958.0f, 60.0f, 6.0f), FVector(4.0f, 720.0f, 12.0f), false, CubeMesh);
    BaseboardRightMesh = CreateBoxMesh(TEXT("BaseboardRight"), FVector(480.0f, 418.0f, 6.0f), FVector(960.0f, 4.0f, 12.0f), false, CubeMesh);
    BaseboardLeftFrontMesh = CreateBoxMesh(TEXT("BaseboardLeftFront"), FVector(85.0f, -298.0f, 6.0f), FVector(170.0f, 4.0f, 12.0f), false, CubeMesh);
    BaseboardLeftRearMesh = CreateBoxMesh(TEXT("BaseboardLeftRear"), FVector(745.0f, -298.0f, 6.0f), FVector(430.0f, 4.0f, 12.0f), false, CubeMesh);

    // Ceiling fan + warm dome light shade around (480, 70, 326).
    CeilingFanRodMesh = CreateDecorativeMesh(
        TEXT("CeilingFanRod"), SceneRoot, FVector(480.0f, 70.0f, 333.0f), FRotator::ZeroRotator, FVector(0.06f, 0.06f, 0.14f), CylinderMesh, false);
    CeilingFanPivot = CreateDefaultSubobject<USceneComponent>(TEXT("CeilingFanPivot"));
    CeilingFanPivot->SetupAttachment(SceneRoot);
    CeilingFanPivot->SetRelativeLocation(FVector(480.0f, 70.0f, 326.0f));

    CeilingFanHubMesh = CreateDecorativeMesh(
        TEXT("CeilingFanHub"), CeilingFanPivot, FVector::ZeroVector, FRotator::ZeroRotator, FVector(0.28f, 0.28f, 0.08f), CylinderMesh, false);
    CeilingFanBlade1Mesh = CreateDecorativeMesh(
        TEXT("CeilingFanBlade1"), CeilingFanPivot, FVector(72.0f, 0.0f, 1.0f), FRotator::ZeroRotator, FVector(1.10f, 0.18f, 0.02f), CubeMesh, false);
    CeilingFanBlade2Mesh = CreateDecorativeMesh(
        TEXT("CeilingFanBlade2"), CeilingFanPivot, FVector(-72.0f, 0.0f, 1.0f), FRotator::ZeroRotator, FVector(1.10f, 0.18f, 0.02f), CubeMesh, false);
    CeilingFanBlade3Mesh = CreateDecorativeMesh(
        TEXT("CeilingFanBlade3"), CeilingFanPivot, FVector(0.0f, 72.0f, 1.0f), FRotator::ZeroRotator, FVector(0.18f, 1.10f, 0.02f), CubeMesh, false);
    CeilingFanBlade4Mesh = CreateDecorativeMesh(
        TEXT("CeilingFanBlade4"), CeilingFanPivot, FVector(0.0f, -72.0f, 1.0f), FRotator::ZeroRotator, FVector(0.18f, 1.10f, 0.02f), CubeMesh, false);
    CeilingLightShadeMesh = CreateDecorativeMesh(
        TEXT("CeilingLightShade"), SceneRoot, FVector(480.0f, 70.0f, 318.0f), FRotator::ZeroRotator, FVector(0.26f, 0.26f, 0.14f), SphereMesh, false);

    // Night window & framed portrait on BackWall (NoCollision, facing camera -X).
    const FRotator BackWallPlaneRot = (FQuat(FRotator(0.0f, 180.0f, 0.0f)) * FQuat(FRotator(-90.0f, 0.0f, 0.0f))).Rotator();
    BackWindowFrameMesh = CreateBoxMesh(TEXT("BackWindowFrame"), FVector(958.0f, 195.0f, 175.0f), FVector(6.0f, 176.0f, 136.0f), false, CubeMesh);
    BackWindowPaneMesh = CreateDecorativeMesh(
        TEXT("BackWindowPane"), SceneRoot, FVector(954.0f, 195.0f, 175.0f), BackWallPlaneRot, FVector(1.16f, 1.56f, 1.0f), PlaneMesh, false);
    BackPortraitFrameMesh = CreateBoxMesh(TEXT("BackPortraitFrame"), FVector(958.0f, -140.0f, 185.0f), FVector(6.0f, 76.0f, 96.0f), false, CubeMesh);
    BackPortraitArtMesh = CreateDecorativeMesh(
        TEXT("BackPortraitArt"), SceneRoot, FVector(954.0f, -140.0f, 185.0f), BackWallPlaneRot, FVector(0.80f, 0.60f, 1.0f), PlaneMesh, false);

    // Decorative trim slabs on PartitionB1 (cozy sofa/bench) & PartitionB2 (low wood cabinet), strictly NoCollision & <= 46cm height.
    PartitionB1TrimTopMesh = CreateBoxMesh(TEXT("PartitionB1TrimTop"), FVector(560.0f, 140.0f, 44.0f), FVector(32.0f, 560.0f, 4.0f), false, CubeMesh);
    PartitionB1Cushion1Mesh = CreateBoxMesh(TEXT("PartitionB1Cushion1"), FVector(540.0f, -40.0f, 24.0f), FVector(18.0f, 165.0f, 20.0f), false, CubeMesh);
    PartitionB1Cushion2Mesh = CreateBoxMesh(TEXT("PartitionB1Cushion2"), FVector(540.0f, 140.0f, 24.0f), FVector(18.0f, 165.0f, 20.0f), false, CubeMesh);
    PartitionB1Cushion3Mesh = CreateBoxMesh(TEXT("PartitionB1Cushion3"), FVector(540.0f, 320.0f, 24.0f), FVector(18.0f, 165.0f, 20.0f), false, CubeMesh);
    PartitionB2TrimTopMesh = CreateBoxMesh(TEXT("PartitionB2TrimTop"), FVector(760.0f, 35.0f, 44.0f), FVector(34.0f, 352.0f, 4.0f), false, CubeMesh);
    PartitionB2CabinetFrontMesh = CreateBoxMesh(TEXT("PartitionB2CabinetFront"), FVector(744.0f, 35.0f, 22.0f), FVector(4.0f, 332.0f, 32.0f), false, CubeMesh);

    // Invisible pawn barriers preventing escape through front cutaway (X=0) or off porch bounds.
    FrontCutawayBarrier = CreatePawnBlocker(TEXT("FrontCutawayBarrier"), FVector(-12.0f, 60.0f, 170.0f), FVector(12.0f, 380.0f, 170.0f));
    PorchBoundFront = CreatePawnBlocker(TEXT("PorchBoundFront"), FVector(128.0f, -410.0f, 130.0f), FVector(12.0f, 110.0f, 130.0f));
    PorchBoundRear = CreatePawnBlocker(TEXT("PorchBoundRear"), FVector(632.0f, -410.0f, 130.0f), FVector(12.0f, 110.0f, 130.0f));
    PorchBoundLeft = CreatePawnBlocker(TEXT("PorchBoundLeft"), FVector(380.0f, -532.0f, 130.0f), FVector(252.0f, 12.0f, 130.0f));
    LeftWallRearBlocker = CreatePawnBlocker(TEXT("LeftWallRearBlocker"), FVector(745.0f, -312.0f, 170.0f), FVector(215.0f, 14.0f, 170.0f));

    // Warm matte room lighting and cool exterior porch light.
    IndoorLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("IndoorLight"));
    IndoorLight->SetupAttachment(SceneRoot);
    IndoorLight->SetRelativeLocation(FVector(580.0f, 100.0f, 290.0f));
    IndoorLight->SetLightColor(FLinearColor(1.0f, 0.85f, 0.65f));
    IndoorLight->SetUseInverseSquaredFalloff(false);
    IndoorLight->SetIntensity(6.5f);
    IndoorLight->SetAttenuationRadius(1100.0f);

    PorchLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PorchLight"));
    PorchLight->SetupAttachment(SceneRoot);
    PorchLight->SetRelativeLocation(FVector(360.0f, -410.0f, 250.0f));
    PorchLight->SetLightColor(FLinearColor(0.68f, 0.82f, 1.0f));
    PorchLight->SetUseInverseSquaredFalloff(false);
    PorchLight->SetIntensity(4.5f);
    PorchLight->SetAttenuationRadius(700.0f);

    BgmAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("BgmAudioComponent"));
    BgmAudioComponent->SetupAttachment(SceneRoot);
    BgmAudioComponent->bAutoActivate = false;
    BgmAudioComponent->bAllowSpatialization = false;
    BgmAudioComponent->SetVolumeMultiplier(0.35f);
}

UStaticMeshComponent* ADCStageDirector::CreateBoxMesh(const TCHAR* Name, const FVector& CenterCm, const FVector& SizeCm, bool bSolidCollision, UStaticMesh* CubeMesh)
{
    UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(Name);
    Comp->SetupAttachment(SceneRoot);
    if (CubeMesh)
    {
        Comp->SetStaticMesh(CubeMesh);
    }
    Comp->SetRelativeLocation(CenterCm);
    Comp->SetRelativeScale3D(SizeCm / 100.0f);
    if (bSolidCollision)
    {
        Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Comp->SetCollisionResponseToAllChannels(ECR_Block);
    }
    else
    {
        Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Comp->SetGenerateOverlapEvents(false);
    }
    return Comp;
}

UStaticMeshComponent* ADCStageDirector::CreateDecorativeMesh(
    const TCHAR* Name,
    USceneComponent* Parent,
    const FVector& RelLocationCm,
    const FRotator& RelRotation,
    const FVector& RelScale,
    UStaticMesh* MeshAsset,
    bool bCastShadow)
{
    UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(Name);
    Comp->SetupAttachment(Parent ? Parent : SceneRoot.Get());
    if (MeshAsset)
    {
        Comp->SetStaticMesh(MeshAsset);
    }
    Comp->SetRelativeLocation(RelLocationCm);
    Comp->SetRelativeRotation(RelRotation);
    Comp->SetRelativeScale3D(RelScale);
    Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Comp->SetGenerateOverlapEvents(false);
    Comp->SetCastShadow(bCastShadow);
    return Comp;
}

UBoxComponent* ADCStageDirector::CreatePawnBlocker(const TCHAR* Name, const FVector& CenterCm, const FVector& BoxExtentCm)
{
    UBoxComponent* Box = CreateDefaultSubobject<UBoxComponent>(Name);
    Box->SetupAttachment(SceneRoot);
    Box->SetRelativeLocation(CenterCm);
    Box->SetBoxExtent(BoxExtentCm);
    Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Box->SetCollisionResponseToAllChannels(ECR_Ignore);
    Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    Box->SetHiddenInGame(true);
    return Box;
}

void ADCStageDirector::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyGrayboxMaterials();
}

void ADCStageDirector::ApplyGrayboxMaterials()
{
    UMaterialInterface* FloorMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Floor.M_Floor"));
    UMaterialInterface* WallMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Wall.M_Wall"));
    UMaterialInterface* CeilingMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Ceiling.M_Ceiling"));
    UMaterialInterface* GroundMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Ground.M_Ground"));
    UMaterialInterface* DoorFrameMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_DoorFrame.M_DoorFrame"));
    UMaterialInterface* EntryMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Entry.M_Entry"));
    UMaterialInterface* PartitionMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Partition.M_Partition"));
    UMaterialInterface* RugMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Rug.M_Rug"));
    UMaterialInterface* NightWindowMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_NightWindow.M_NightWindow"));
    UMaterialInterface* PortraitMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Portrait.M_Portrait"));
    UMaterialInterface* CeilingLightMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_CeilingLight.M_CeilingLight"));

    if (!CeilingMat)
    {
        CeilingMat = WallMat;
    }

    if (FloorMesh && FloorMat) { FloorMesh->SetMaterial(0, FloorMat); }
    if (CeilingMesh && CeilingMat) { CeilingMesh->SetMaterial(0, CeilingMat); }
    if (BackWallMesh && WallMat) { BackWallMesh->SetMaterial(0, WallMat); }
    if (RightWallMesh && WallMat) { RightWallMesh->SetMaterial(0, WallMat); }
    if (LeftWallFrontMesh && WallMat) { LeftWallFrontMesh->SetMaterial(0, WallMat); }
    if (LeftWallRearMesh && WallMat) { LeftWallRearMesh->SetMaterial(0, WallMat); }
    if (LeftWallRearHeaderMesh && WallMat) { LeftWallRearHeaderMesh->SetMaterial(0, WallMat); }
    if (ExteriorBackWallMesh && WallMat) { ExteriorBackWallMesh->SetMaterial(0, WallMat); }
    if (ExteriorLeftWallMesh && WallMat) { ExteriorLeftWallMesh->SetMaterial(0, WallMat); }
    if (DoorHeaderMesh && WallMat) { DoorHeaderMesh->SetMaterial(0, WallMat); }
    if (PorchFloorMesh && GroundMat) { PorchFloorMesh->SetMaterial(0, GroundMat); }
    if (FrontFloorApronMesh && (GroundMat || FloorMat)) { FrontFloorApronMesh->SetMaterial(0, GroundMat ? GroundMat : FloorMat); }

    UMaterialInterface* TrimMat = DoorFrameMat ? DoorFrameMat : (EntryMat ? EntryMat : WallMat);
    if (LeftWallRearSillMesh && TrimMat) { LeftWallRearSillMesh->SetMaterial(0, TrimMat); }
    if (DoorFrameFrontJambMesh && TrimMat) { DoorFrameFrontJambMesh->SetMaterial(0, TrimMat); }
    if (DoorFrameRearJambMesh && TrimMat) { DoorFrameRearJambMesh->SetMaterial(0, TrimMat); }
    if (DoorFrameLintelMesh && TrimMat) { DoorFrameLintelMesh->SetMaterial(0, TrimMat); }
    if (BaseboardBackMesh && TrimMat) { BaseboardBackMesh->SetMaterial(0, TrimMat); }
    if (BaseboardRightMesh && TrimMat) { BaseboardRightMesh->SetMaterial(0, TrimMat); }
    if (BaseboardLeftFrontMesh && TrimMat) { BaseboardLeftFrontMesh->SetMaterial(0, TrimMat); }
    if (BaseboardLeftRearMesh && TrimMat) { BaseboardLeftRearMesh->SetMaterial(0, TrimMat); }
    if (CeilingFanRodMesh && TrimMat) { CeilingFanRodMesh->SetMaterial(0, TrimMat); }
    if (CeilingFanHubMesh && TrimMat) { CeilingFanHubMesh->SetMaterial(0, TrimMat); }
    if (CeilingFanBlade1Mesh && TrimMat) { CeilingFanBlade1Mesh->SetMaterial(0, TrimMat); }
    if (CeilingFanBlade2Mesh && TrimMat) { CeilingFanBlade2Mesh->SetMaterial(0, TrimMat); }
    if (CeilingFanBlade3Mesh && TrimMat) { CeilingFanBlade3Mesh->SetMaterial(0, TrimMat); }
    if (CeilingFanBlade4Mesh && TrimMat) { CeilingFanBlade4Mesh->SetMaterial(0, TrimMat); }
    if (BackWindowFrameMesh && TrimMat) { BackWindowFrameMesh->SetMaterial(0, TrimMat); }
    if (BackPortraitFrameMesh && TrimMat) { BackPortraitFrameMesh->SetMaterial(0, TrimMat); }
    if (PartitionB1TrimTopMesh && TrimMat) { PartitionB1TrimTopMesh->SetMaterial(0, TrimMat); }
    if (PartitionB2TrimTopMesh && TrimMat) { PartitionB2TrimTopMesh->SetMaterial(0, TrimMat); }
    if (PartitionB2CabinetFrontMesh && TrimMat) { PartitionB2CabinetFrontMesh->SetMaterial(0, TrimMat); }

    UMaterialInterface* EffectivePartitionMat = PartitionMat ? PartitionMat : TrimMat;
    if (PartitionB1Mesh && EffectivePartitionMat) { PartitionB1Mesh->SetMaterial(0, EffectivePartitionMat); }
    if (PartitionB2Mesh && EffectivePartitionMat) { PartitionB2Mesh->SetMaterial(0, EffectivePartitionMat); }

    UMaterialInterface* EffectiveRugMat = RugMat ? RugMat : TrimMat;
    if (AreaRugMesh && EffectiveRugMat) { AreaRugMesh->SetMaterial(0, EffectiveRugMat); }

    UMaterialInterface* CushionMat = RugMat ? RugMat : EffectivePartitionMat;
    if (PartitionB1Cushion1Mesh && CushionMat) { PartitionB1Cushion1Mesh->SetMaterial(0, CushionMat); }
    if (PartitionB1Cushion2Mesh && CushionMat) { PartitionB1Cushion2Mesh->SetMaterial(0, CushionMat); }
    if (PartitionB1Cushion3Mesh && CushionMat) { PartitionB1Cushion3Mesh->SetMaterial(0, CushionMat); }

    UMaterialInterface* EffectiveWindowMat = NightWindowMat ? NightWindowMat : (GroundMat ? GroundMat : WallMat);
    if (BackWindowPaneMesh && EffectiveWindowMat) { BackWindowPaneMesh->SetMaterial(0, EffectiveWindowMat); }

    UMaterialInterface* EffectivePortraitMat = PortraitMat ? PortraitMat : (CeilingMat ? CeilingMat : TrimMat);
    if (BackPortraitArtMesh && EffectivePortraitMat) { BackPortraitArtMesh->SetMaterial(0, EffectivePortraitMat); }

    UMaterialInterface* EffectiveLightShadeMat = CeilingLightMat ? CeilingLightMat : (CeilingMat ? CeilingMat : TrimMat);
    if (CeilingLightShadeMesh && EffectiveLightShadeMat) { CeilingLightShadeMesh->SetMaterial(0, EffectiveLightShadeMat); }
}

ADCFixedStageCamera* ADCStageDirector::EnsureStageCamera()
{
    if (StageCamera)
    {
        return StageCamera.Get();
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<ADCFixedStageCamera> It(World); It; ++It)
    {
        StageCamera = *It;
        break;
    }

    if (!StageCamera)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        StageCamera = World->SpawnActor<ADCFixedStageCamera>(
            ADCFixedStageCamera::StaticClass(),
            FVector(-320.0f, -180.0f, 275.0f),
            FRotator(-15.5f, 0.0f, 0.0f),
            Params);
    }

    return StageCamera.Get();
}

void ADCStageDirector::StartOrRefreshBgm(const UDCLevelConfigData* Config)
{
    UWorld* World = GetWorld();
    if (bIsTearingDown || !World || !World->IsGameWorld() || !BgmAudioComponent || !Config || !Config->StageBgmSound)
    {
        return;
    }

    if (BgmAudioComponent->Sound != Config->StageBgmSound)
    {
        BgmAudioComponent->SetSound(Config->StageBgmSound);
    }
    BgmAudioComponent->SetVolumeMultiplier(0.35f);
    if (!BgmAudioComponent->IsPlaying())
    {
        BgmAudioComponent->Play();
    }
}

void ADCStageDirector::HandleBgmFinished()
{
    UWorld* World = GetWorld();
    if (!bIsTearingDown && World && World->IsGameWorld() && BgmAudioComponent && BgmAudioComponent->Sound)
    {
        BgmAudioComponent->Play();
    }
}

void ADCStageDirector::BeginPlay()
{
    Super::BeginPlay();

    bIsTearingDown = false;
    ApplyGrayboxMaterials();

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>();
    if (TakeSubsystem)
    {
        TakeSubsystem->OnTakeReset.AddUniqueDynamic(this, &ADCStageDirector::HandleTakeReset);
        TakeSubsystem->OnTakePhaseChanged.AddUniqueDynamic(this, &ADCStageDirector::HandleTakePhaseChanged);
        TakeSubsystem->InitializeStage(LevelConfig.Get());
    }

    if (BgmAudioComponent)
    {
        BgmAudioComponent->OnAudioFinished.AddUniqueDynamic(this, &ADCStageDirector::HandleBgmFinished);
    }

    const UDCLevelConfigData* EffectiveConfig = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : LevelConfig.Get();
    ADCFixedStageCamera* Cam = EnsureStageCamera();
    if (Cam)
    {
        Cam->ApplyConfig(EffectiveConfig);
        Cam->ActivateForPlayer(World->GetFirstPlayerController());
    }

    SyncFireSpotsWithConfig();
    ResetPlayerToSpawn();
    if (Cam)
    {
        Cam->SnapToPlayerNow();
    }
    StartOrRefreshBgm(EffectiveConfig);
}

void ADCStageDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bIsTearingDown = true;

    if (BgmAudioComponent)
    {
        BgmAudioComponent->OnAudioFinished.RemoveDynamic(this, &ADCStageDirector::HandleBgmFinished);
        BgmAudioComponent->Stop();
    }

    if (UWorld* World = GetWorld())
    {
        if (UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            TakeSubsystem->OnTakeReset.RemoveDynamic(this, &ADCStageDirector::HandleTakeReset);
            TakeSubsystem->OnTakePhaseChanged.RemoveDynamic(this, &ADCStageDirector::HandleTakePhaseChanged);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void ADCStageDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (CeilingFanPivot)
    {
        CeilingFanPivot->AddLocalRotation(FRotator(0.0f, 65.0f * FMath::Max(0.0f, DeltaSeconds), 0.0f));
    }

    if (UWorld* World = GetWorld())
    {
        if (UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            if (TakeSubsystem->IsActiveTake())
            {
                TakeSubsystem->AdvanceElapsedTime(DeltaSeconds);
            }
        }
    }
}

void ADCStageDirector::SyncFireSpotsWithConfig()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>();
    const UDCLevelConfigData* EffectiveConfig = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : LevelConfig.Get();
    const int32 MaxActiveSpots = EffectiveConfig ? FMath::Max(1, EffectiveConfig->ActiveFireSpotCount) : 3;

    TArray<ADCFireHazardActor*> FireSpots;
    for (TActorIterator<ADCFireHazardActor> It(World); It; ++It)
    {
        if (ADCFireHazardActor* Fire = *It)
        {
            FireSpots.Add(Fire);
        }
    }

    FireSpots.Sort([](const ADCFireHazardActor& A, const ADCFireHazardActor& B)
    {
        if (A.FireSpotIndex != B.FireSpotIndex)
        {
            return A.FireSpotIndex < B.FireSpotIndex;
        }
        return A.GetFName().LexicalLess(B.GetFName());
    });

    int32 ActiveCount = 0;
    for (ADCFireHazardActor* Fire : FireSpots)
    {
        const bool bEnabled = (Fire->FireSpotIndex >= 0) && (Fire->FireSpotIndex < MaxActiveSpots);
        Fire->SetSpotEnabledInLevel(bEnabled);
        if (bEnabled)
        {
            ++ActiveCount;
        }
    }

    if (TakeSubsystem)
    {
        TakeSubsystem->RegisterFireSpots(ActiveCount);
    }
}

void ADCStageDirector::HandleTakeReset()
{
    ADCFixedStageCamera* Cam = nullptr;
    const UDCLevelConfigData* EffectiveConfig = LevelConfig.Get();
    if (UWorld* World = GetWorld())
    {
        const UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>();
        EffectiveConfig = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : LevelConfig.Get();
        Cam = EnsureStageCamera();
        if (Cam)
        {
            Cam->ApplyConfig(EffectiveConfig);
            Cam->ActivateForPlayer(World->GetFirstPlayerController());
        }
    }

    SyncFireSpotsWithConfig();
    ResetPlayerToSpawn();
    if (Cam)
    {
        Cam->SnapToPlayerNow();
    }
    StartOrRefreshBgm(EffectiveConfig);
}

void ADCStageDirector::HandleTakePhaseChanged(EDCTakePhase NewPhase)
{
    if (UWorld* World = GetWorld())
    {
        if (ADCFixedStageCamera* Cam = EnsureStageCamera())
        {
            Cam->ActivateForPlayer(World->GetFirstPlayerController());
        }
    }
}

void ADCStageDirector::ResetPlayerToSpawn()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (TActorIterator<ADCPlayerCharacter> It(World); It; ++It)
    {
        ADCPlayerCharacter* Player = *It;
        if (!IsValid(Player))
        {
            continue;
        }

        if (UCharacterMovementComponent* MoveComp = Player->GetCharacterMovement())
        {
            MoveComp->StopMovementImmediately();
        }
        Player->SetActorLocationAndRotation(PlayerSpawnLocation, PlayerSpawnRotation, false, nullptr, ETeleportType::TeleportPhysics);
        Player->RefreshConfigAndCamera();
    }
}
