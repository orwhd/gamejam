#include "Interaction/DCActionStateComponent.h"
#include "Core/DCTakeSubsystem.h"
#include "Data/DCLevelConfigData.h"
#include "Interaction/DCInteractableBase.h"
#include "Persona/DCPersonaComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

UDCActionStateComponent::UDCActionStateComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UDCActionStateComponent::BeginPlay()
{
    Super::BeginPlay();

    if (UWorld* World = GetWorld())
    {
        if (UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            BoundTakeSubsystem = TakeSubsystem;
            TakeSubsystem->OnTakeReset.AddDynamic(this, &UDCActionStateComponent::HandleTakeReset);
        }
    }

    if (AActor* Owner = GetOwner())
    {
        if (UDCPersonaComponent* PersonaComp = Owner->FindComponentByClass<UDCPersonaComponent>())
        {
            BoundPersonaComponent = PersonaComp;
            PersonaComp->OnPersonaChanged.AddDynamic(this, &UDCActionStateComponent::HandlePersonaChanged);
        }
    }
}

void UDCActionStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UDCTakeSubsystem* TakeSubsystem = BoundTakeSubsystem.Get())
    {
        TakeSubsystem->OnTakeReset.RemoveDynamic(this, &UDCActionStateComponent::HandleTakeReset);
    }
    BoundTakeSubsystem.Reset();

    if (UDCPersonaComponent* PersonaComp = BoundPersonaComponent.Get())
    {
        PersonaComp->OnPersonaChanged.RemoveDynamic(this, &UDCActionStateComponent::HandlePersonaChanged);
    }
    BoundPersonaComponent.Reset();

    Super::EndPlay(EndPlayReason);
}

void UDCActionStateComponent::HandleTakeReset()
{
    ResetActionState();
}

void UDCActionStateComponent::HandlePersonaChanged()
{
    ClearThoughtBubble();
}

void UDCActionStateComponent::GrantCapability(FGameplayTag Tag)
{
    if (!Tag.IsValid() || ActiveCapabilities.HasTagExact(Tag))
    {
        return;
    }

    ActiveCapabilities.AddTag(Tag);
    OnActionStateChanged.Broadcast();
}

void UDCActionStateComponent::RevokeCapability(FGameplayTag Tag)
{
    if (!Tag.IsValid())
    {
        return;
    }

    if (ActiveCapabilities.RemoveTag(Tag))
    {
        OnActionStateChanged.Broadcast();
    }
}

bool UDCActionStateComponent::HasCapability(FGameplayTag Tag) const
{
    return Tag.IsValid() && ActiveCapabilities.HasTag(Tag);
}

bool UDCActionStateComponent::HasAllCapabilities(const FGameplayTagContainer& Tags) const
{
    return ActiveCapabilities.HasAll(Tags);
}

bool UDCActionStateComponent::HasAnyCapabilities(const FGameplayTagContainer& Tags) const
{
    return !Tags.IsEmpty() && ActiveCapabilities.HasAny(Tags);
}

void UDCActionStateComponent::SetEscortedActor(AActor* InEscortedActor)
{
    if (EscortedActor.Get() != InEscortedActor)
    {
        EscortedActor = InEscortedActor;
        OnActionStateChanged.Broadcast();
    }
}

void UDCActionStateComponent::SetFocusedInteractable(ADCInteractableBase* InInteractable)
{
    if (FocusedInteractable.Get() != InInteractable)
    {
        FocusedInteractable = InInteractable;
        OnActionStateChanged.Broadcast();
    }
}

void UDCActionStateComponent::SetActiveHoldInteractable(ADCInteractableBase* InInteractable)
{
    if (ActiveHoldInteractable.Get() != InInteractable)
    {
        ActiveHoldInteractable = InInteractable;
        OnActionStateChanged.Broadcast();
    }
}

void UDCActionStateComponent::SetPersonaPanelOpen(bool bOpen)
{
    if (bPersonaPanelOpen != bOpen)
    {
        bPersonaPanelOpen = bOpen;
        OnPersonaPanelToggled.Broadcast(bPersonaPanelOpen);
        OnActionStateChanged.Broadcast();
    }
}

void UDCActionStateComponent::RequestThoughtBubble(const FText& Text, float DurationSeconds)
{
    float ResolvedDuration = DurationSeconds;
    if (ResolvedDuration <= 0.0f)
    {
        const UWorld* World = GetWorld();
        const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
        const UDCLevelConfigData* Config = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : nullptr;
        ResolvedDuration = Config ? Config->ThoughtBubbleDurationSeconds : 3.8f;
    }

    CurrentThoughtText = Text;
    OnThoughtRequested.Broadcast(Text, ResolvedDuration);
}

void UDCActionStateComponent::ClearThoughtBubble()
{
    CurrentThoughtText = FText::GetEmpty();
    OnThoughtCleared.Broadcast();
}

void UDCActionStateComponent::RequestHudHint(const FText& Text, float DurationSeconds)
{
    float ResolvedDuration = DurationSeconds;
    if (ResolvedDuration <= 0.0f)
    {
        const UWorld* World = GetWorld();
        const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
        const UDCLevelConfigData* Config = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : nullptr;
        ResolvedDuration = Config ? Config->StarPopupDisplaySeconds : 2.0f;
    }

    OnHudHintRequested.Broadcast(Text, ResolvedDuration);
}

void UDCActionStateComponent::SetLastDebugReason(const FString& InReason)
{
    LastDebugReason = InReason;
}

void UDCActionStateComponent::ResetActionState()
{
    ActiveCapabilities.Reset();
    EscortedActor = nullptr;
    FocusedInteractable = nullptr;
    ActiveHoldInteractable = nullptr;
    LastDebugReason.Reset();

    ClearThoughtBubble();

    if (bPersonaPanelOpen)
    {
        bPersonaPanelOpen = false;
        OnPersonaPanelToggled.Broadcast(false);
    }

    OnActionStateChanged.Broadcast();
}
