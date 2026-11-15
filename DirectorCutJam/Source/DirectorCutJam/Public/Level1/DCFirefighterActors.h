#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/DCTypes.h"
#include "Interaction/DCInteractableBase.h"
#include "DCFirefighterActors.generated.h"

class APawn;
class UBoxComponent;
class UCapsuleComponent;
class UDCPaperVisualComponent;
class UDCTakeSubsystem;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Silent blocking captain NPC positioned outside the open doorway.
 * Arms-spread 2D paper card; never speaks and ignores Persona edits.
 */
UCLASS(Blueprintable)
class DIRECTORCUTJAM_API ADCCaptainActor : public AActor
{
    GENERATED_BODY()

public:
    ADCCaptainActor();

    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Captain")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Captain")
    TObjectPtr<UCapsuleComponent> BlockingCapsule;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Captain")
    TObjectPtr<UDCPaperVisualComponent> PaperVisual;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="DirectorCut|Captain")
    bool bHasDialogue = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="DirectorCut|Captain")
    FName BlockingPostureState = TEXT("BlockEntranceArmsSpread");
};

/**
 * Persona-gated doorway interactable spanning the open left doorway (X in [170, 530], Y = -300).
 * Blocks physical passage until player passes the Persona check and executes Action.Door.Enter.
 */
UCLASS(Blueprintable)
class DIRECTORCUTJAM_API ADCDoorwayInteractable : public ADCInteractableBase
{
    GENERATED_BODY()

public:
    ADCDoorwayInteractable();

    virtual void Tick(float DeltaSeconds) override;
    virtual bool IsActionAvailable(const APawn* Interactor) const override;
    virtual bool ExecuteInteraction(APawn* Interactor) override;
    virtual void ResetInteractable() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Door")
    TObjectPtr<UBoxComponent> DoorwayBlocker;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Door")
    TObjectPtr<UDCPaperVisualComponent> PaperVisual;

    UFUNCTION(BlueprintPure, Category="DirectorCut|Door")
    bool IsDoorUnlocked() const { return bDoorUnlocked; }

private:
    void SetDoorBlockerActive(bool bBlocked);

    UPROPERTY(Transient)
    bool bDoorUnlocked = false;

    UPROPERTY(Transient)
    bool bAwaitingCrossingRelock = false;

    UPROPERTY(Transient)
    float CrossingStartSideSign = -1.0f;

    TWeakObjectPtr<APawn> CrossingPawn;
};

/**
 * Extinguisher tool pickup in the front-right Z1 corner. Grants Capability.ExtinguishFire.
 */
UCLASS(Blueprintable)
class DIRECTORCUTJAM_API ADCExtinguisherPickup : public ADCInteractableBase
{
    GENERATED_BODY()

public:
    ADCExtinguisherPickup();

    virtual bool IsActionAvailable(const APawn* Interactor) const override;
    virtual bool ExecuteInteraction(APawn* Interactor) override;
    virtual void ResetInteractable() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Extinguisher")
    TObjectPtr<UDCPaperVisualComponent> PaperVisual;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DirectorCut|Interaction", meta=(MultiLine=true))
    FText CannotPickupWhileEscortingThoughtText;

    UFUNCTION(BlueprintPure, Category="DirectorCut|Extinguisher")
    bool IsPickedUp() const { return bPickedUp; }

private:
    UPROPERTY(Transient)
    bool bPickedUp = false;
};

/**
 * Individual blocking fire hazard spot.
 * Subscribes to UDCTakeSubsystem::OnSceneStatusChanged for 4-stage visual/collision scaling,
 * supports HoldToComplete spraying with per-spot retained progress, and notifies TakeSubsystem on extinguish.
 */
UCLASS(Blueprintable)
class DIRECTORCUTJAM_API ADCFireHazardActor : public ADCInteractableBase
{
    GENERATED_BODY()

public:
    ADCFireHazardActor();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

    virtual bool IsActionAvailable(const APawn* Interactor) const override;
    virtual float GetHoldProgressNormalized() const override;
    virtual bool IsHoldActive() const override { return bIsSpraying; }
    virtual bool BeginHoldInteraction(APawn* Interactor) override;
    virtual void TickHoldInteraction(APawn* Interactor, float DeltaSeconds) override;
    virtual void EndHoldInteraction(APawn* Interactor, bool bCancelled) override;
    virtual bool ExecuteInteraction(APawn* Interactor) override;
    virtual void ResetInteractable() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DirectorCut|Fire", meta=(ClampMin="0"))
    int32 FireSpotIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DirectorCut|Fire")
    bool bAutoExpandIndexZeroShortcutBlocker = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Fire")
    TObjectPtr<UBoxComponent> FireBlockerBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Fire")
    TObjectPtr<UDCPaperVisualComponent> PaperVisual;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Fire")
    TObjectPtr<UStaticMeshComponent> FireCoreMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Fire")
    TObjectPtr<UStaticMeshComponent> FireOuterMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Fire")
    TObjectPtr<UStaticMeshComponent> SprayStreamMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Fire")
    TObjectPtr<UPointLightComponent> FireLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Fire")
    TObjectPtr<UPointLightComponent> FireGlowLight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DirectorCut|Interaction", meta=(MultiLine=true))
    FText CannotExtinguishWhileEscortingThoughtText;

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Fire")
    void SetSpotEnabledInLevel(bool bInEnabled);

    UFUNCTION(BlueprintPure, Category="DirectorCut|Fire")
    bool IsSpotEnabledInLevel() const { return bSpotEnabledInLevel; }

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Fire")
    void CompleteExtinguishNow();

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Fire")
    void EvaluateStageForElapsedTime(float ElapsedSeconds);

    UFUNCTION(BlueprintPure, Category="DirectorCut|Fire")
    EDCFireStage GetCurrentFireStage() const { return CurrentStage; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Fire")
    bool IsSpraying() const { return bIsSpraying; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Fire")
    float GetRetainedSpraySeconds() const { return RetainedSpraySeconds; }

private:
    UFUNCTION()
    void HandleSceneStatusChanged(EDCFireStage FireStage, EDCCivilianState NewCivilianState);

    void ResolveBaseBlockerExtent();
    float GetEffectiveRequiredSpraySeconds() const;
    float GetBaseFireGlowIntensity() const;
    bool StartSprayingInternal(APawn* Interactor);
    void StopSprayingInternal(APawn* Interactor);
    void UpdateFireVisualsForStage();
    void UpdateFireBlockerCollision();
    void ApplyFireMaterials();

    UPROPERTY(Transient)
    bool bSpotEnabledInLevel = true;

    UPROPERTY(Transient)
    EDCFireStage CurrentStage = EDCFireStage::Active;

    UPROPERTY(Transient)
    bool bIsSpraying = false;

    UPROPERTY(Transient)
    float RetainedSpraySeconds = 0.0f;

    UPROPERTY(Transient)
    float FireFlickerTimeSeconds = 0.0f;

    UPROPERTY(Transient)
    FVector2D BasePaperSizeCm = FVector2D(140.0f, 145.0f);

    UPROPERTY(Transient)
    FVector BaseBlockerExtent = FVector(32.0f, 42.0f, 90.0f);

    UPROPERTY(Transient)
    bool bHasResolvedBaseBlockerExtent = false;

    TWeakObjectPtr<APawn> SprayingInteractor;
    TWeakObjectPtr<UDCTakeSubsystem> BoundTakeSubsystem;
};

/**
 * Trapped civilian inside the burning room (deep-left Z3).
 * Pressing E transitions player + civilian into a single combined escort paper card and freezes civilian health state.
 * Delivering to ADCSafeZoneVolume parks the rescued civilian in Z_Safe and awards stars.
 */
UCLASS(Blueprintable)
class DIRECTORCUTJAM_API ADCCivilianActor : public ADCInteractableBase
{
    GENERATED_BODY()

public:
    ADCCivilianActor();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual bool IsActionAvailable(const APawn* Interactor) const override;
    virtual EDCInteractionResult EvaluateInteraction(const APawn* Interactor) const override;
    virtual bool ExecuteInteraction(APawn* Interactor) override;
    virtual void ResetInteractable() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Civilian")
    TObjectPtr<UDCPaperVisualComponent> PaperVisual;

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Civilian")
    void CompleteRescue(const FVector& SafeParkLocation);

    UFUNCTION(BlueprintPure, Category="DirectorCut|Civilian")
    EDCCivilianState GetCivilianState() const { return CivilianState; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Civilian")
    bool IsAlive() const { return CivilianState != EDCCivilianState::Dead; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Civilian")
    bool IsEscorting() const
    {
        return CivilianState == EDCCivilianState::EscortingUninjured
            || CivilianState == EDCCivilianState::EscortingInjured;
    }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Civilian")
    bool IsFollowing() const { return IsEscorting(); }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Civilian")
    APawn* GetEscortingPawn() const { return EscortingPawn.Get(); }

private:
    UFUNCTION()
    void HandleSceneStatusChanged(EDCFireStage FireStage, EDCCivilianState NewCivilianState);

    void ClearEscortFromPawn();

    UPROPERTY(Transient)
    EDCCivilianState CivilianState = EDCCivilianState::Uninjured;

    UPROPERTY(Transient)
    EDCCivilianState PreEscortState = EDCCivilianState::Uninjured;

    UPROPERTY(Transient)
    FVector InitialLocation = FVector(885.0f, -95.0f, 0.0f);

    UPROPERTY(Transient)
    bool bHasCapturedInitialLocation = false;

    TWeakObjectPtr<APawn> EscortingPawn;
    TWeakObjectPtr<UDCTakeSubsystem> BoundTakeSubsystem;
};

/**
 * Exterior porch safe zone volume. Completes rescue when an escorting player enters TriggerBox.
 */
UCLASS(Blueprintable)
class DIRECTORCUTJAM_API ADCSafeZoneVolume : public AActor
{
    GENERATED_BODY()

public:
    ADCSafeZoneVolume();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SafeZone")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SafeZone")
    TObjectPtr<UBoxComponent> TriggerBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|SafeZone")
    TObjectPtr<UStaticMeshComponent> FloorPadMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DirectorCut|SafeZone")
    FVector CivilianParkOffset = FVector(-100.0f, -15.0f, 0.0f);

private:
    void ApplyPadMaterial();
    bool IsPointInsideZone2D(const FVector& WorldPoint) const;
};

/**
 * Little Match Girl NPC standing on the right exterior sidewalk near the red extinguisher.
 * Freely repeatable comedic dialogue interaction; never alters puzzle win/loss state.
 */
UCLASS(Blueprintable)
class DIRECTORCUTJAM_API ADCMatchGirlActor : public ADCInteractableBase
{
    GENERATED_BODY()

public:
    ADCMatchGirlActor();

    virtual bool IsActionAvailable(const APawn* Interactor) const override;
    virtual bool ExecuteInteraction(APawn* Interactor) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|MatchGirl")
    TObjectPtr<UDCPaperVisualComponent> PaperVisual;
};

/**
 * Chubby angel pup Coco waiting in the exterior porch safe zone.
 * Celebrates ("Happy" PaperState) once the trapped civilian is rescued.
 */
UCLASS(Blueprintable)
class DIRECTORCUTJAM_API ADCAngelDogActor : public AActor
{
    GENERATED_BODY()

public:
    ADCAngelDogActor();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|AngelDog")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|AngelDog")
    TObjectPtr<UDCPaperVisualComponent> PaperVisual;

private:
    UFUNCTION()
    void HandleSceneStatusChanged(EDCFireStage FireStage, EDCCivilianState CivilianState);

    UFUNCTION()
    void HandleTakeReset();

    TWeakObjectPtr<UDCTakeSubsystem> BoundTakeSubsystem;
};
