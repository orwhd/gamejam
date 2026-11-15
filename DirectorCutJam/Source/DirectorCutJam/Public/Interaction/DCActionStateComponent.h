#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "DCActionStateComponent.generated.h"

class AActor;
class ADCInteractableBase;
class UDCPersonaComponent;
class UDCTakeSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCActionStateChangedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCPersonaPanelToggledDelegate, bool, bIsOpen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDCThoughtRequestedDelegate, FText, Text, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCThoughtClearedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDCHudHintRequestedDelegate, FText, Text, float, Duration);

UCLASS(ClassGroup=(DirectorCut), meta=(BlueprintSpawnableComponent))
class DIRECTORCUTJAM_API UDCActionStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDCActionStateComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category="DirectorCut|ActionState")
    void GrantCapability(FGameplayTag Tag);

    UFUNCTION(BlueprintCallable, Category="DirectorCut|ActionState")
    void RevokeCapability(FGameplayTag Tag);

    UFUNCTION(BlueprintPure, Category="DirectorCut|ActionState")
    bool HasCapability(FGameplayTag Tag) const;

    UFUNCTION(BlueprintPure, Category="DirectorCut|ActionState")
    bool HasAllCapabilities(const FGameplayTagContainer& Tags) const;

    UFUNCTION(BlueprintPure, Category="DirectorCut|ActionState")
    bool HasAnyCapabilities(const FGameplayTagContainer& Tags) const;

    UFUNCTION(BlueprintPure, Category="DirectorCut|ActionState")
    const FGameplayTagContainer& GetActiveCapabilities() const { return ActiveCapabilities; }

    UFUNCTION(BlueprintCallable, Category="DirectorCut|ActionState")
    void SetEscortedActor(AActor* InEscortedActor);

    UFUNCTION(BlueprintPure, Category="DirectorCut|ActionState")
    AActor* GetEscortedActor() const { return EscortedActor.Get(); }

    UFUNCTION(BlueprintCallable, Category="DirectorCut|ActionState")
    void SetFocusedInteractable(ADCInteractableBase* InInteractable);

    UFUNCTION(BlueprintPure, Category="DirectorCut|ActionState")
    ADCInteractableBase* GetFocusedInteractable() const { return FocusedInteractable.Get(); }

    UFUNCTION(BlueprintCallable, Category="DirectorCut|ActionState")
    void SetActiveHoldInteractable(ADCInteractableBase* InInteractable);

    UFUNCTION(BlueprintPure, Category="DirectorCut|ActionState")
    ADCInteractableBase* GetActiveHoldInteractable() const { return ActiveHoldInteractable.Get(); }

    UFUNCTION(BlueprintCallable, Category="DirectorCut|ActionState")
    void SetPersonaPanelOpen(bool bOpen);

    UFUNCTION(BlueprintPure, Category="DirectorCut|ActionState")
    bool IsPersonaPanelOpen() const { return bPersonaPanelOpen; }

    UFUNCTION(BlueprintCallable, Category="DirectorCut|ActionState")
    void RequestThoughtBubble(const FText& Text, float DurationSeconds = -1.0f);

    UFUNCTION(BlueprintCallable, Category="DirectorCut|ActionState")
    void ClearThoughtBubble();

    UFUNCTION(BlueprintCallable, Category="DirectorCut|ActionState")
    void RequestHudHint(const FText& Text, float DurationSeconds = -1.0f);

    UFUNCTION(BlueprintCallable, Category="DirectorCut|ActionState")
    void SetLastDebugReason(const FString& InReason);

    UFUNCTION(BlueprintPure, Category="DirectorCut|ActionState")
    FString GetLastDebugReason() const { return LastDebugReason; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|ActionState")
    FText GetCurrentThoughtText() const { return CurrentThoughtText; }

    UFUNCTION(BlueprintCallable, Category="DirectorCut|ActionState")
    void ResetActionState();

    UPROPERTY(BlueprintAssignable, Category="DirectorCut|ActionState")
    FDCActionStateChangedDelegate OnActionStateChanged;

    UPROPERTY(BlueprintAssignable, Category="DirectorCut|ActionState")
    FDCPersonaPanelToggledDelegate OnPersonaPanelToggled;

    UPROPERTY(BlueprintAssignable, Category="DirectorCut|ActionState")
    FDCThoughtRequestedDelegate OnThoughtRequested;

    UPROPERTY(BlueprintAssignable, Category="DirectorCut|ActionState")
    FDCThoughtClearedDelegate OnThoughtCleared;

    UPROPERTY(BlueprintAssignable, Category="DirectorCut|ActionState")
    FDCHudHintRequestedDelegate OnHudHintRequested;

private:
    UFUNCTION()
    void HandleTakeReset();

    UFUNCTION()
    void HandlePersonaChanged();

    UPROPERTY(Transient)
    FGameplayTagContainer ActiveCapabilities;

    UPROPERTY(Transient)
    TObjectPtr<AActor> EscortedActor;

    UPROPERTY(Transient)
    TObjectPtr<ADCInteractableBase> FocusedInteractable;

    UPROPERTY(Transient)
    TObjectPtr<ADCInteractableBase> ActiveHoldInteractable;

    UPROPERTY(Transient)
    bool bPersonaPanelOpen = false;

    UPROPERTY(Transient)
    FText CurrentThoughtText;

    UPROPERTY(Transient)
    FString LastDebugReason;

    TWeakObjectPtr<UDCTakeSubsystem> BoundTakeSubsystem;
    TWeakObjectPtr<UDCPersonaComponent> BoundPersonaComponent;
};
