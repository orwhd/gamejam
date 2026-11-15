#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Core/DCTypes.h"
#include "DCInteractableBase.generated.h"

class USceneComponent;
class APawn;
class UDCTakeSubsystem;

USTRUCT(BlueprintType)
struct DIRECTORCUTJAM_API FDCInteractionRule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    FGameplayTag ActionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    FText PromptText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    EDCInteractionInputMode InputMode = EDCInteractionInputMode::InstantPress;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(ClampMin="10.0"))
    float InteractionRadius = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    FVector PromptWorldOffset = FVector(0.0f, 0.0f, 195.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    FGameplayTagContainer RequiredCapabilityTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    FGameplayTagContainer BlockingCapabilityTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    FGameplayTagContainer RequiredPersonaTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    FGameplayTagContainer BlockingPersonaTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    FGameplayTagContainer GrantedCapabilitiesOnSuccess;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(MultiLine=true))
    FText PersonaBlockedThoughtText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(MultiLine=true))
    FText MissingCapabilityThoughtText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    FText MissingCapabilityHintText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(MultiLine=true))
    FText SuccessThoughtText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(ClampMin="0.1"))
    float SuccessThoughtDuration = 2.2f;
};

UCLASS(Blueprintable)
class DIRECTORCUTJAM_API ADCInteractableBase : public AActor
{
    GENERATED_BODY()

public:
    ADCInteractableBase();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    FDCInteractionRule Rule;

    UFUNCTION(BlueprintPure, Category="Interaction")
    virtual bool IsActionAvailable(const APawn* Interactor) const;

    UFUNCTION(BlueprintPure, Category="Interaction")
    virtual FText GetPromptText(const APawn* Interactor) const;

    UFUNCTION(BlueprintPure, Category="Interaction")
    virtual EDCInteractionInputMode GetInteractionInputMode() const { return Rule.InputMode; }

    UFUNCTION(BlueprintPure, Category="Interaction")
    virtual float GetHoldProgressNormalized() const { return 0.0f; }

    UFUNCTION(BlueprintPure, Category="Interaction")
    virtual bool IsHoldActive() const { return false; }

    UFUNCTION(BlueprintPure, Category="Interaction")
    virtual FVector GetPromptWorldLocation() const;

    UFUNCTION(BlueprintPure, Category="Interaction")
    virtual EDCInteractionResult EvaluateInteraction(const APawn* Interactor) const;

    UFUNCTION(BlueprintCallable, Category="Interaction")
    virtual bool TryInteract(APawn* Interactor);

    UFUNCTION(BlueprintCallable, Category="Interaction")
    virtual bool BeginHoldInteraction(APawn* Interactor);

    UFUNCTION(BlueprintCallable, Category="Interaction")
    virtual void TickHoldInteraction(APawn* Interactor, float DeltaSeconds);

    UFUNCTION(BlueprintCallable, Category="Interaction")
    virtual void EndHoldInteraction(APawn* Interactor, bool bCancelled);

    UFUNCTION(BlueprintCallable, Category="Interaction")
    virtual bool ExecuteInteraction(APawn* Interactor);

    UFUNCTION(BlueprintImplementableEvent, Category="Interaction")
    void BP_OnInteractionExecuted(APawn* Interactor);

    UFUNCTION(BlueprintCallable, Category="Interaction")
    virtual void ResetInteractable();

protected:
    float GetEffectiveInteractionRadius() const;

private:
    void HandleInteractionDenied(APawn* Interactor, EDCInteractionResult Result);

    UFUNCTION()
    void HandleTakeReset();

    TWeakObjectPtr<UDCTakeSubsystem> BoundTakeSubsystem;
};
