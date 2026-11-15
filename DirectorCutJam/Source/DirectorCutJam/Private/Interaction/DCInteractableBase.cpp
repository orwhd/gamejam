#include "Interaction/DCInteractableBase.h"
#include "Core/DCTakeSubsystem.h"
#include "Data/DCLevelConfigData.h"
#include "Persona/DCPersonaComponent.h"
#include "Interaction/DCActionStateComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

ADCInteractableBase::ADCInteractableBase()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
}

void ADCInteractableBase::BeginPlay()
{
    Super::BeginPlay();

    if (UWorld* World = GetWorld())
    {
        if (UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            BoundTakeSubsystem = TakeSubsystem;
            TakeSubsystem->OnTakeReset.AddDynamic(this, &ADCInteractableBase::HandleTakeReset);
        }
    }
}

void ADCInteractableBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UDCTakeSubsystem* TakeSubsystem = BoundTakeSubsystem.Get())
    {
        TakeSubsystem->OnTakeReset.RemoveDynamic(this, &ADCInteractableBase::HandleTakeReset);
    }
    BoundTakeSubsystem.Reset();

    Super::EndPlay(EndPlayReason);
}

void ADCInteractableBase::HandleTakeReset()
{
    ResetInteractable();
}

bool ADCInteractableBase::IsActionAvailable(const APawn* Interactor) const
{
    return true;
}

FText ADCInteractableBase::GetPromptText(const APawn* Interactor) const
{
    return Rule.PromptText;
}

FVector ADCInteractableBase::GetPromptWorldLocation() const
{
    return GetActorLocation() + Rule.PromptWorldOffset;
}

float ADCInteractableBase::GetEffectiveInteractionRadius() const
{
    if (Rule.InteractionRadius > 0.0f)
    {
        return Rule.InteractionRadius;
    }

    if (const UWorld* World = GetWorld())
    {
        if (const UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            if (const UDCLevelConfigData* Config = TakeSubsystem->GetActiveConfig())
            {
                return Config->DefaultInteractionRadius;
            }
        }
    }

    return 175.0f;
}

EDCInteractionResult ADCInteractableBase::EvaluateInteraction(const APawn* Interactor) const
{
    if (!IsValid(Interactor))
    {
        return EDCInteractionResult::Denied_UnavailableInScene;
    }

    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    if (!TakeSubsystem || !TakeSubsystem->IsActiveTake())
    {
        return EDCInteractionResult::Denied_NotActiveTake;
    }

    if (!IsActionAvailable(Interactor))
    {
        return EDCInteractionResult::Denied_UnavailableInScene;
    }

    const float EffectiveRadius = GetEffectiveInteractionRadius();
    const float HorizontalDistance = FVector::Dist2D(Interactor->GetActorLocation(), GetActorLocation());
    if (HorizontalDistance > EffectiveRadius)
    {
        return EDCInteractionResult::Denied_OutOfRange;
    }

    const UDCActionStateComponent* ActionState = Interactor->FindComponentByClass<UDCActionStateComponent>();
    if (ActionState && ActionState->IsPersonaPanelOpen())
    {
        return EDCInteractionResult::Denied_UnavailableInScene;
    }

    const UDCPersonaComponent* PersonaComp = Interactor->FindComponentByClass<UDCPersonaComponent>();

    if (!Rule.RequiredPersonaTags.IsEmpty())
    {
        if (!PersonaComp || !PersonaComp->HasAllPersonaTags(Rule.RequiredPersonaTags))
        {
            return EDCInteractionResult::Denied_MissingPersonaTag;
        }
    }

    if (!Rule.BlockingPersonaTags.IsEmpty())
    {
        if (PersonaComp && PersonaComp->HasAnyPersonaTags(Rule.BlockingPersonaTags))
        {
            return EDCInteractionResult::Denied_BlockedByPersona;
        }
    }

    if (!Rule.RequiredCapabilityTags.IsEmpty())
    {
        if (!ActionState || !ActionState->HasAllCapabilities(Rule.RequiredCapabilityTags))
        {
            return EDCInteractionResult::Denied_MissingCapability;
        }
    }

    if (!Rule.BlockingCapabilityTags.IsEmpty())
    {
        if (ActionState && ActionState->HasAnyCapabilities(Rule.BlockingCapabilityTags))
        {
            return EDCInteractionResult::Denied_UnavailableInScene;
        }
    }

    return EDCInteractionResult::Allowed;
}

void ADCInteractableBase::HandleInteractionDenied(APawn* Interactor, EDCInteractionResult Result)
{
    UDCActionStateComponent* ActionState = Interactor ? Interactor->FindComponentByClass<UDCActionStateComponent>() : nullptr;
    if (!ActionState)
    {
        return;
    }

    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    const UDCLevelConfigData* Config = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : nullptr;
    const float ThoughtDuration = Config ? Config->ThoughtBubbleDurationSeconds : 3.8f;

    switch (Result)
    {
        case EDCInteractionResult::Denied_BlockedByPersona:
        {
            ActionState->SetLastDebugReason(
                FString::Printf(TEXT("Denied_BlockedByPersona: %s"), *Rule.BlockingPersonaTags.ToStringSimple()));

            const FText ThoughtText = !Rule.PersonaBlockedThoughtText.IsEmpty()
                ? Rule.PersonaBlockedThoughtText
                : NSLOCTEXT("DirectorCut", "DefaultBlockedThought", "Something holds me back...");
            ActionState->RequestThoughtBubble(ThoughtText, ThoughtDuration);
            break;
        }

        case EDCInteractionResult::Denied_MissingPersonaTag:
        {
            ActionState->SetLastDebugReason(
                FString::Printf(TEXT("Denied_MissingPersonaTag: %s"), *Rule.RequiredPersonaTags.ToStringSimple()));

            const FText ThoughtText = !Rule.PersonaBlockedThoughtText.IsEmpty()
                ? Rule.PersonaBlockedThoughtText
                : NSLOCTEXT("DirectorCut", "DefaultMissingPersonaThought", "I don't feel ready to do this...");
            ActionState->RequestThoughtBubble(ThoughtText, ThoughtDuration);
            break;
        }

        case EDCInteractionResult::Denied_MissingCapability:
        {
            ActionState->SetLastDebugReason(
                FString::Printf(TEXT("Denied_MissingCapability: %s"), *Rule.RequiredCapabilityTags.ToStringSimple()));

            const FText ThoughtText = !Rule.MissingCapabilityThoughtText.IsEmpty()
                ? Rule.MissingCapabilityThoughtText
                : (!Rule.MissingCapabilityHintText.IsEmpty()
                    ? Rule.MissingCapabilityHintText
                    : NSLOCTEXT("DirectorCut", "DefaultMissingCapabilityThought", "I need the right tool first."));
            ActionState->RequestThoughtBubble(ThoughtText, ThoughtDuration);
            break;
        }

        case EDCInteractionResult::Denied_NotActiveTake:
        {
            ActionState->SetLastDebugReason(TEXT("Denied_NotActiveTake"));
            break;
        }

        case EDCInteractionResult::Denied_OutOfRange:
        {
            ActionState->SetLastDebugReason(TEXT("Denied_OutOfRange"));
            break;
        }

        case EDCInteractionResult::Denied_UnavailableInScene:
        default:
        {
            ActionState->SetLastDebugReason(TEXT("Denied_UnavailableInScene"));
            break;
        }
    }
}

bool ADCInteractableBase::TryInteract(APawn* Interactor)
{
    const EDCInteractionResult Result = EvaluateInteraction(Interactor);
    if (Result != EDCInteractionResult::Allowed)
    {
        HandleInteractionDenied(Interactor, Result);
        return false;
    }

    UDCActionStateComponent* ActionState = Interactor ? Interactor->FindComponentByClass<UDCActionStateComponent>() : nullptr;
    if (ActionState)
    {
        ActionState->ClearThoughtBubble();
    }

    if (!ExecuteInteraction(Interactor))
    {
        return false;
    }

    if (ActionState)
    {
        for (const FGameplayTag& GrantedTag : Rule.GrantedCapabilitiesOnSuccess)
        {
            ActionState->GrantCapability(GrantedTag);
        }
        ActionState->SetLastDebugReason(TEXT(""));
        if (!Rule.SuccessThoughtText.IsEmpty())
        {
            ActionState->RequestThoughtBubble(Rule.SuccessThoughtText, Rule.SuccessThoughtDuration);
        }
    }

    BP_OnInteractionExecuted(Interactor);
    return true;
}

bool ADCInteractableBase::BeginHoldInteraction(APawn* Interactor)
{
    const EDCInteractionResult Result = EvaluateInteraction(Interactor);
    if (Result != EDCInteractionResult::Allowed)
    {
        HandleInteractionDenied(Interactor, Result);
        return false;
    }

    if (UDCActionStateComponent* ActionState = Interactor ? Interactor->FindComponentByClass<UDCActionStateComponent>() : nullptr)
    {
        ActionState->SetLastDebugReason(TEXT(""));
        ActionState->ClearThoughtBubble();
    }

    return true;
}

void ADCInteractableBase::TickHoldInteraction(APawn* Interactor, float DeltaSeconds)
{
}

void ADCInteractableBase::EndHoldInteraction(APawn* Interactor, bool bCancelled)
{
}

bool ADCInteractableBase::ExecuteInteraction(APawn* Interactor)
{
    return true;
}

void ADCInteractableBase::ResetInteractable()
{
}
