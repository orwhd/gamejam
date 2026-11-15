#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "Core/DCTypes.h"
#include "DCStageDirector.generated.h"

class ADCFixedStageCamera;
class UAudioComponent;
class UBoxComponent;
class UDCLevelConfigData;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class DIRECTORCUTJAM_API ADCGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADCGameMode();
};

/**
 * Architectural 3D room + open left doorway + exterior porch + low knee-height S-curve partitions.
 * Applies UDCLevelConfigData to UDCTakeSubsystem, ADCFixedStageCamera, and dynamic fire spot registration.
 */
UCLASS(Blueprintable)
class DIRECTORCUTJAM_API ADCStageDirector : public AActor
{
    GENERATED_BODY()

public:
    ADCStageDirector();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="DirectorCut|Stage")
    TObjectPtr<UDCLevelConfigData> LevelConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DirectorCut|Stage")
    FVector PlayerSpawnLocation = FVector(500.0f, -345.0f, 92.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DirectorCut|Stage")
    FRotator PlayerSpawnRotation = FRotator(0.0f, 0.0f, 0.0f);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> FloorMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> CeilingMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> BackWallMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> RightWallMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> LeftWallFrontMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> LeftWallRearMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> LeftWallRearHeaderMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> LeftWallRearSillMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> DoorHeaderMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> DoorFrameFrontJambMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> DoorFrameRearJambMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> DoorFrameLintelMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> PartitionB1Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> PartitionB2Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> PorchFloorMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> FrontFloorApronMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> ExteriorBackWallMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Architecture")
    TObjectPtr<UStaticMeshComponent> ExteriorLeftWallMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> AreaRugMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> BaseboardBackMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> BaseboardRightMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> BaseboardLeftFrontMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> BaseboardLeftRearMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<USceneComponent> CeilingFanPivot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> CeilingFanRodMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> CeilingFanHubMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> CeilingFanBlade1Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> CeilingFanBlade2Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> CeilingFanBlade3Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> CeilingFanBlade4Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> CeilingLightShadeMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> BackWindowFrameMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> BackWindowPaneMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> BackPortraitFrameMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> BackPortraitArtMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> PartitionB1TrimTopMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> PartitionB1Cushion1Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> PartitionB1Cushion2Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> PartitionB1Cushion3Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> PartitionB2TrimTopMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SetDressing")
    TObjectPtr<UStaticMeshComponent> PartitionB2CabinetFrontMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Collision")
    TObjectPtr<UBoxComponent> FrontCutawayBarrier;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Collision")
    TObjectPtr<UBoxComponent> PorchBoundFront;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Collision")
    TObjectPtr<UBoxComponent> PorchBoundRear;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Collision")
    TObjectPtr<UBoxComponent> PorchBoundLeft;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Collision")
    TObjectPtr<UBoxComponent> LeftWallRearBlocker;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Lighting")
    TObjectPtr<UPointLightComponent> IndoorLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Lighting")
    TObjectPtr<UPointLightComponent> PorchLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Audio")
    TObjectPtr<UAudioComponent> BgmAudioComponent;

    UFUNCTION(BlueprintPure, Category="DirectorCut|Stage")
    ADCFixedStageCamera* GetStageCamera() const { return StageCamera.Get(); }

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Stage")
    void ResetPlayerToSpawn();

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Stage")
    void SyncFireSpotsWithConfig();

private:
    UStaticMeshComponent* CreateBoxMesh(const TCHAR* Name, const FVector& CenterCm, const FVector& SizeCm, bool bSolidCollision, UStaticMesh* CubeMesh);
    UStaticMeshComponent* CreateDecorativeMesh(
        const TCHAR* Name,
        USceneComponent* Parent,
        const FVector& RelLocationCm,
        const FRotator& RelRotation,
        const FVector& RelScale,
        UStaticMesh* MeshAsset,
        bool bCastShadow = true);
    UBoxComponent* CreatePawnBlocker(const TCHAR* Name, const FVector& CenterCm, const FVector& BoxExtentCm);
    void ApplyGrayboxMaterials();
    ADCFixedStageCamera* EnsureStageCamera();
    void StartOrRefreshBgm(const UDCLevelConfigData* Config);

    UFUNCTION()
    void HandleTakeReset();

    UFUNCTION()
    void HandleTakePhaseChanged(EDCTakePhase NewPhase);

    UFUNCTION()
    void HandleBgmFinished();

    UPROPERTY(Transient)
    TObjectPtr<ADCFixedStageCamera> StageCamera;

    UPROPERTY(Transient)
    bool bIsTearingDown = false;
};
