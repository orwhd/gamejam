#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/DCTypes.h"
#include "DCPlayerCharacter.generated.h"

class ADCInteractableBase;
class UDCActionStateComponent;
class UDCLevelConfigData;
class UDCPaperVisualComponent;
class UDCPersonaComponent;
class UDCTakeSubsystem;
class UInputComponent;
class USceneComponent;
class UStaticMeshComponent;
class UUserWidget;
class UWidgetComponent;

/**
 * Player-controlled 2.5D character.
 * Uses upright Bottom-Center grounded paper card (no spring arm, no sphere meshes, no jumping).
 * Camera-plane WASD movement and deterministic interaction/input bindings.
 */
UCLASS(Blueprintable)
class DIRECTORCUTJAM_API ADCPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ADCPlayerCharacter();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Components")
    TObjectPtr<UDCPersonaComponent> PersonaComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Components")
    TObjectPtr<UDCActionStateComponent> ActionStateComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Visual")
    TObjectPtr<UDCPaperVisualComponent> PaperVisual;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DirectorCut|Visual")
    EDCCharacterVisualMode VisualMode = EDCCharacterVisualMode::Paper2D;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Visual")
    TObjectPtr<USceneComponent> Fallback3DRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Visual")
    TObjectPtr<UStaticMeshComponent> Fallback3DBodyMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Visual")
    TObjectPtr<UStaticMeshComponent> Fallback3DHeadMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Visual")
    TObjectPtr<UStaticMeshComponent> Fallback3DHelmetMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Visual")
    TObjectPtr<UStaticMeshComponent> Fallback3DToolMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|Visual")
    TObjectPtr<UStaticMeshComponent> Fallback3DBuddyMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|UI")
    TObjectPtr<UWidgetComponent> ThoughtWidgetComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="DirectorCut|UI")
    TObjectPtr<UWidgetComponent> InteractPromptWidgetComponent;

    UPROPERTY(Transient, BlueprintReadOnly, Category="DirectorCut|UI")
    TObjectPtr<UUserWidget> HudOverlayWidget;

    UPROPERTY(Transient, BlueprintReadOnly, Category="DirectorCut|UI")
    TObjectPtr<UUserWidget> ThoughtWidget;

    UPROPERTY(Transient, BlueprintReadOnly, Category="DirectorCut|UI")
    TObjectPtr<UUserWidget> InteractPromptWidget;

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Input")
    void Interact();

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Input")
    void StartInteractInput();

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Input")
    void StopInteractInput();

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Input")
    void TogglePersona();

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Input")
    void StartTakeAction();

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Input")
    void RestartTakeAction();

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Visual")
    void SetVisualMode(EDCCharacterVisualMode NewMode);

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Input")
    void ToggleVisualMode();

    UFUNCTION(BlueprintPure, Category="DirectorCut|Movement")
    bool CanMove() const;

    UFUNCTION(BlueprintPure, Category="DirectorCut|Interaction")
    ADCInteractableBase* FindNearestAvailableInteractable() const;

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Stage")
    void RefreshConfigAndCamera();

private:
    void MoveForward(float Value);
    void MoveRight(float Value);

    void GetCameraPlaneAxes(FVector& OutForward, FVector& OutRight) const;
    float GetInteractableRadius(const ADCInteractableBase* Candidate, const UDCLevelConfigData* Config) const;
    void ApplyPlayerPaperTexturesFromConfig(const UDCLevelConfigData* Config);
    void ApplyVisualMode();
    void ApplyFallback3DColors();
    void UpdateFallback3DState(const UDCLevelConfigData* Config);
    void CancelActiveHoldInteraction();
    void UpdateActiveHoldInteraction(const UDCTakeSubsystem* TakeSubsystem, const UDCLevelConfigData* Config, float DeltaSeconds);
    void UpdateAudioFeedback(const UDCTakeSubsystem* TakeSubsystem, const UDCLevelConfigData* Config, float DeltaSeconds);
    void UpdateMovementAndVisualState(const UDCLevelConfigData* Config);
    void UpdateFocusedInteractable();
    void UpdateInteractPromptWidget(const UDCTakeSubsystem* TakeSubsystem);
    void UpdateInputModeForState();
    void SpawnStageWidgets();

    UFUNCTION()
    void HandleTakePhaseChanged(EDCTakePhase NewPhase);

    UFUNCTION()
    void HandleTakeReset();

    UFUNCTION()
    void HandleThoughtRequested(FText Text, float DurationSeconds);

    UFUNCTION()
    void HandleThoughtCleared();

    UFUNCTION()
    void HandlePersonaPanelToggled(bool bIsOpen);

    UPROPERTY(Transient)
    float ThoughtRemainingSeconds = 0.0f;

    UPROPERTY(Transient)
    float BlockedReactionRemainingSeconds = 0.0f;

    UPROPERTY(Transient)
    float FootstepCooldownSeconds = 0.0f;

    UPROPERTY(Transient)
    float WaterSpraySoundCooldownSeconds = 0.0f;

    UPROPERTY(Transient)
    bool bWasSprayingForAudio = false;
};
