#include "Level1/DCFirefighterActors.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/DCGameplayTags.h"
#include "Core/DCTakeSubsystem.h"
#include "Data/DCLevelConfigData.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Interaction/DCActionStateComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Persona/DCPersonaComponent.h"
#include "Sound/SoundBase.h"
#include "Spatial/DCPaperVisualComponent.h"
#include "UObject/ConstructorHelpers.h"

ADCCaptainActor::ADCCaptainActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    BlockingCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BlockingCapsule"));
    BlockingCapsule->SetupAttachment(SceneRoot);
    BlockingCapsule->InitCapsuleSize(24.0f, 88.0f);
    BlockingCapsule->SetRelativeLocation(FVector(0.0f, 0.0f, 88.0f));
    BlockingCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    BlockingCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
    BlockingCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

    PaperVisual = CreateDefaultSubobject<UDCPaperVisualComponent>(TEXT("PaperVisual"));
    PaperVisual->SetupAttachment(SceneRoot);
    PaperVisual->CardSizeCm = FVector2D(130.0f, 182.0f);
    PaperVisual->GrayboxTint = FLinearColor(0.16f, 0.35f, 0.90f, 1.0f);
}

void ADCCaptainActor::BeginPlay()
{
    Super::BeginPlay();

    bHasDialogue = false;
    if (PaperVisual)
    {
        PaperVisual->SetPaperState(BlockingPostureState);
    }
}

ADCDoorwayInteractable::ADCDoorwayInteractable()
{
    PrimaryActorTick.bCanEverTick = true;

    DoorwayBlocker = CreateDefaultSubobject<UBoxComponent>(TEXT("DoorwayBlocker"));
    DoorwayBlocker->SetupAttachment(SceneRoot);
    DoorwayBlocker->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
    DoorwayBlocker->SetBoxExtent(FVector(180.0f, 16.0f, 130.0f));
    SetDoorBlockerActive(true);

    PaperVisual = CreateDefaultSubobject<UDCPaperVisualComponent>(TEXT("PaperVisual"));
    PaperVisual->SetupAttachment(SceneRoot);
    PaperVisual->SetRelativeLocation(FVector(130.0f, 18.0f, 0.0f));
    PaperVisual->CardSizeCm = FVector2D(105.0f, 215.0f);
    PaperVisual->GrayboxTint = FLinearColor(0.62f, 0.42f, 0.24f, 1.0f);
    PaperVisual->bEnableIdleBreathing = false;
    PaperVisual->bEnableWalkBounce = false;
    PaperVisual->StateTints.Add(TEXT("Unlocked"), FLinearColor(0.32f, 0.72f, 0.42f, 1.0f));

    Rule.ActionTag = DCGameplayTags::Action_Door_Enter;
    Rule.InputMode = EDCInteractionInputMode::InstantPress;
    Rule.PromptText = NSLOCTEXT("DirectorCut", "DoorEnterPrompt", "ENTER");
    Rule.InteractionRadius = 210.0f;
    Rule.PromptWorldOffset = FVector(0.0f, 0.0f, 130.0f);
    Rule.BlockingPersonaTags.AddTag(DCGameplayTags::Persona_Limit_FearDisobeyCaptain);
    Rule.PersonaBlockedThoughtText = NSLOCTEXT(
        "DirectorCut",
        "DoorBlockedThought",
        "EEK! Red heels and a star wand! Disobey Chief Witch and ribbit—I'm a frog!");
    Rule.SuccessThoughtText = NSLOCTEXT(
        "DirectorCut",
        "DoorSuccessThought",
        "Fab red heels and party hat, ma'am~ Coming through!");
    Rule.SuccessThoughtDuration = 1.9f;
}

void ADCDoorwayInteractable::SetDoorBlockerActive(bool bBlocked)
{
    if (!DoorwayBlocker)
    {
        return;
    }

    if (bBlocked)
    {
        DoorwayBlocker->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        DoorwayBlocker->SetCollisionResponseToAllChannels(ECR_Ignore);
        DoorwayBlocker->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    }
    else
    {
        DoorwayBlocker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

bool ADCDoorwayInteractable::IsActionAvailable(const APawn* Interactor) const
{
    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    const UDCLevelConfigData* Config = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : nullptr;

    if (Config && Config->DoorTraversalPolicy == EDCDoorTraversalPolicy::RequireInteractEachCrossing)
    {
        return !bAwaitingCrossingRelock;
    }

    return !bDoorUnlocked;
}

bool ADCDoorwayInteractable::ExecuteInteraction(APawn* Interactor)
{
    bDoorUnlocked = true;
    SetDoorBlockerActive(false);

    if (PaperVisual)
    {
        PaperVisual->SetPaperState(TEXT("Unlocked"));
    }

    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    const UDCLevelConfigData* Config = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : nullptr;
    if (World && World->IsGameWorld() && Config && Config->DoorOpenSound)
    {
        UGameplayStatics::PlaySound2D(this, Config->DoorOpenSound);
    }

    if (Config && Config->DoorTraversalPolicy == EDCDoorTraversalPolicy::RequireInteractEachCrossing && IsValid(Interactor))
    {
        bAwaitingCrossingRelock = true;
        CrossingPawn = Interactor;
        CrossingStartSideSign = (Interactor->GetActorLocation().Y < GetActorLocation().Y) ? -1.0f : 1.0f;
    }

    return true;
}

void ADCDoorwayInteractable::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bAwaitingCrossingRelock)
    {
        return;
    }

    const APawn* Pawn = CrossingPawn.Get();
    if (!IsValid(Pawn))
    {
        bAwaitingCrossingRelock = false;
        SetDoorBlockerActive(true);
        return;
    }

    const float DeltaY = Pawn->GetActorLocation().Y - GetActorLocation().Y;
    const float CurrentSign = (DeltaY < 0.0f) ? -1.0f : 1.0f;
    if (CurrentSign != CrossingStartSideSign && FMath::Abs(DeltaY) > 75.0f)
    {
        bAwaitingCrossingRelock = false;
        bDoorUnlocked = false;
        SetDoorBlockerActive(true);
        if (PaperVisual)
        {
            PaperVisual->SetPaperState(NAME_None);
        }
    }
}

void ADCDoorwayInteractable::ResetInteractable()
{
    bDoorUnlocked = false;
    bAwaitingCrossingRelock = false;
    CrossingPawn.Reset();
    SetDoorBlockerActive(true);
    if (PaperVisual)
    {
        PaperVisual->SetPaperState(NAME_None);
    }
}

ADCExtinguisherPickup::ADCExtinguisherPickup()
{
    PrimaryActorTick.bCanEverTick = false;

    PaperVisual = CreateDefaultSubobject<UDCPaperVisualComponent>(TEXT("PaperVisual"));
    PaperVisual->SetupAttachment(SceneRoot);
    PaperVisual->CardSizeCm = FVector2D(48.0f, 96.0f);
    PaperVisual->GrayboxTint = FLinearColor(0.86f, 0.14f, 0.14f, 1.0f);
    PaperVisual->bEnableIdleBreathing = false;
    PaperVisual->bEnableWalkBounce = false;

    Rule.ActionTag = DCGameplayTags::Action_Tool_PickupExtinguisher;
    Rule.InputMode = EDCInteractionInputMode::InstantPress;
    Rule.PromptText = NSLOCTEXT("DirectorCut", "ExtinguisherPrompt", "PICK UP");
    Rule.InteractionRadius = 175.0f;
    Rule.PromptWorldOffset = FVector(0.0f, 0.0f, 125.0f);
    Rule.RequiredPersonaTags.AddTag(DCGameplayTags::Persona_KnowHow_UseExtinguisher);
    Rule.PersonaBlockedThoughtText = NSLOCTEXT(
        "DirectorCut",
        "ExtinguisherMissingKnowHowThought",
        "A giant ketchup bottle for that kid's imaginary roast chicken?! No clue how it works!");
    Rule.GrantedCapabilitiesOnSuccess.AddTag(DCGameplayTags::Capability_ExtinguishFire);
    CannotPickupWhileEscortingThoughtText = NSLOCTEXT(
        "DirectorCut",
        "CannotPickupWhileEscortingThought",
        "Hands are full carrying someone...\nGet them outside first!");
}

bool ADCExtinguisherPickup::IsActionAvailable(const APawn* Interactor) const
{
    return !bPickedUp;
}

bool ADCExtinguisherPickup::ExecuteInteraction(APawn* Interactor)
{
    if (bPickedUp)
    {
        return false;
    }

    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    const UDCLevelConfigData* Config = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : nullptr;

    if (IsValid(Interactor) && Config)
    {
        if (UDCActionStateComponent* ActionState = Interactor->FindComponentByClass<UDCActionStateComponent>())
        {
            if (Config->EscortToolPolicy == EDCEscortToolPolicy::HideToolAndDisableSpray
                && ActionState->HasCapability(DCGameplayTags::State_EscortingCivilian))
            {
                if (!CannotPickupWhileEscortingThoughtText.IsEmpty())
                {
                    ActionState->RequestThoughtBubble(
                        CannotPickupWhileEscortingThoughtText,
                        Config->ThoughtBubbleDurationSeconds);
                }
                return false;
            }
        }
    }

    bPickedUp = true;
    if (World && World->IsGameWorld() && Config && Config->PickupToolSound)
    {
        UGameplayStatics::PlaySound2D(this, Config->PickupToolSound);
    }
    if (PaperVisual)
    {
        PaperVisual->SetPaperVisible(false);
    }
    return true;
}

void ADCExtinguisherPickup::ResetInteractable()
{
    bPickedUp = false;
    if (PaperVisual)
    {
        PaperVisual->SetPaperState(NAME_None);
        PaperVisual->SetPaperVisible(true);
    }
}

ADCFireHazardActor::ADCFireHazardActor()
{
    PrimaryActorTick.bCanEverTick = true;

    FireBlockerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("FireBlockerBox"));
    FireBlockerBox->SetupAttachment(SceneRoot);
    FireBlockerBox->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
    FireBlockerBox->SetBoxExtent(FVector(32.0f, 42.0f, 90.0f));
    FireBlockerBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    FireBlockerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    FireBlockerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    FireBlockerBox->SetHiddenInGame(true);

    PaperVisual = CreateDefaultSubobject<UDCPaperVisualComponent>(TEXT("PaperVisual"));
    PaperVisual->SetupAttachment(SceneRoot);
    PaperVisual->CardSizeCm = BasePaperSizeCm;
    PaperVisual->GrayboxTint = FLinearColor(1.0f, 0.30f, 0.04f, 1.0f);
    PaperVisual->StateTints.Add(TEXT("InjuredCivilian"), FLinearColor(1.0f, 0.42f, 0.05f, 1.0f));
    PaperVisual->StateTints.Add(TEXT("KilledCivilian"), FLinearColor(1.0f, 0.16f, 0.02f, 1.0f));
    PaperVisual->StateTints.Add(TEXT("RoomEngulfed"), FLinearColor(0.85f, 0.06f, 0.02f, 1.0f));
    PaperVisual->StateTints.Add(TEXT("Spraying"), FLinearColor(0.65f, 0.80f, 0.92f, 1.0f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    FireCoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FireCoreMesh"));
    FireCoreMesh->SetupAttachment(SceneRoot);
    if (ConeFinder.Succeeded())
    {
        FireCoreMesh->SetStaticMesh(ConeFinder.Object);
    }
    FireCoreMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));
    FireCoreMesh->SetRelativeScale3D(FVector(0.95f, 0.95f, 1.30f));
    FireCoreMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FireCoreMesh->SetGenerateOverlapEvents(false);

    FireOuterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FireOuterMesh"));
    FireOuterMesh->SetupAttachment(SceneRoot);
    if (ConeFinder.Succeeded())
    {
        FireOuterMesh->SetStaticMesh(ConeFinder.Object);
    }
    FireOuterMesh->SetRelativeLocation(FVector(25.0f, -32.0f, 48.0f));
    FireOuterMesh->SetRelativeScale3D(FVector(0.65f, 0.65f, 0.95f));
    FireOuterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FireOuterMesh->SetGenerateOverlapEvents(false);

    SprayStreamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SprayStreamMesh"));
    SprayStreamMesh->SetupAttachment(SceneRoot);
    if (CylinderFinder.Succeeded())
    {
        SprayStreamMesh->SetStaticMesh(CylinderFinder.Object);
    }
    SprayStreamMesh->SetRelativeLocation(FVector(-60.0f, 0.0f, 95.0f));
    SprayStreamMesh->SetRelativeRotation(FRotator(72.0f, 0.0f, 0.0f));
    SprayStreamMesh->SetRelativeScale3D(FVector(0.18f, 0.18f, 1.20f));
    SprayStreamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SprayStreamMesh->SetVisibility(false);

    FireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
    FireLight->SetupAttachment(SceneRoot);
    FireLight->SetRelativeLocation(FVector(0.0f, 0.0f, 110.0f));
    FireLight->SetLightColor(FLinearColor(1.0f, 0.42f, 0.08f));
    FireLight->SetUseInverseSquaredFalloff(false);
    FireLight->SetIntensity(3.5f);
    FireLight->SetAttenuationRadius(650.0f);

    FireGlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireGlowLight"));
    FireGlowLight->SetupAttachment(SceneRoot);
    FireGlowLight->SetRelativeLocation(FVector(-12.0f, 0.0f, 88.0f));
    FireGlowLight->SetLightColor(FLinearColor(1.0f, 0.46f, 0.10f));
    FireGlowLight->SetUseInverseSquaredFalloff(false);
    FireGlowLight->SetIntensity(4.5f);
    FireGlowLight->SetAttenuationRadius(680.0f);
    FireGlowLight->SetCastShadows(false);

    Rule.ActionTag = DCGameplayTags::Action_Fire_Extinguish;
    Rule.InputMode = EDCInteractionInputMode::HoldToComplete;
    Rule.PromptText = NSLOCTEXT("DirectorCut", "FireExtinguishPrompt", "EXTINGUISH");
    Rule.InteractionRadius = 195.0f;
    Rule.PromptWorldOffset = FVector(0.0f, 0.0f, 165.0f);
    Rule.RequiredCapabilityTags.AddTag(DCGameplayTags::Capability_ExtinguishFire);
    Rule.MissingCapabilityThoughtText = NSLOCTEXT(
        "DirectorCut",
        "FireMissingToolThought",
        "I can't spit enough water! Need that red bottle by the matchstick kid!");
    CannotExtinguishWhileEscortingThoughtText = NSLOCTEXT(
        "DirectorCut",
        "CannotExtinguishWhileEscortingThought",
        "Can't spray while carrying someone...\nGet them outside first!");
}

void ADCFireHazardActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    ResolveBaseBlockerExtent();
    ApplyFireMaterials();
    UpdateFireVisualsForStage();
    UpdateFireBlockerCollision();
}

void ADCFireHazardActor::BeginPlay()
{
    Super::BeginPlay();

    ResolveBaseBlockerExtent();

    if (UWorld* World = GetWorld())
    {
        if (UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            BoundTakeSubsystem = TakeSubsystem;
            TakeSubsystem->OnSceneStatusChanged.AddUniqueDynamic(this, &ADCFireHazardActor::HandleSceneStatusChanged);
        }
    }

    ApplyFireMaterials();
    UpdateFireVisualsForStage();
    UpdateFireBlockerCollision();
}

void ADCFireHazardActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UDCTakeSubsystem* TakeSubsystem = BoundTakeSubsystem.Get())
    {
        TakeSubsystem->OnSceneStatusChanged.RemoveDynamic(this, &ADCFireHazardActor::HandleSceneStatusChanged);
    }
    BoundTakeSubsystem.Reset();

    Super::EndPlay(EndPlayReason);
}

void ADCFireHazardActor::ResolveBaseBlockerExtent()
{
    if (!FireBlockerBox)
    {
        return;
    }

    const FVector UnscaledExtent = FireBlockerBox->GetUnscaledBoxExtent();
    const bool bIsDefaultExtent = UnscaledExtent.Equals(FVector(32.0f, 42.0f, 90.0f), 0.5f);
    if (FireSpotIndex == 0 && bAutoExpandIndexZeroShortcutBlocker && bIsDefaultExtent)
    {
        BaseBlockerExtent = FVector(32.0f, 80.0f, 90.0f);
        FireBlockerBox->SetBoxExtent(BaseBlockerExtent);
        bHasResolvedBaseBlockerExtent = true;
    }
    else if (!bHasResolvedBaseBlockerExtent)
    {
        BaseBlockerExtent = UnscaledExtent;
        bHasResolvedBaseBlockerExtent = true;
    }
}

void ADCFireHazardActor::ApplyFireMaterials()
{
    if (UMaterialInterface* FireMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Fire.M_Fire")))
    {
        if (FireCoreMesh) { FireCoreMesh->SetMaterial(0, FireMat); }
        if (FireOuterMesh) { FireOuterMesh->SetMaterial(0, FireMat); }
    }
}

void ADCFireHazardActor::SetSpotEnabledInLevel(bool bInEnabled)
{
    bSpotEnabledInLevel = bInEnabled;
    if (!bSpotEnabledInLevel)
    {
        StopSprayingInternal(SprayingInteractor.Get());
    }
    UpdateFireVisualsForStage();
    UpdateFireBlockerCollision();
}

float ADCFireHazardActor::GetEffectiveRequiredSpraySeconds() const
{
    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    const UDCLevelConfigData* Config = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : nullptr;

    float RequiredSec = Config ? Config->ExtinguishActionSeconds : 1.8f;
    if (Config && Config->ExtinguishInterruptPolicy == EDCExtinguishInterruptPolicy::IncreaseRequiredOnStageUp)
    {
        int32 StageSteps = 0;
        if (CurrentStage == EDCFireStage::InjuredCivilian)
        {
            StageSteps = 1;
        }
        else if (CurrentStage == EDCFireStage::KilledCivilian || CurrentStage == EDCFireStage::RoomEngulfed)
        {
            StageSteps = 2;
        }
        RequiredSec += static_cast<float>(StageSteps) * Config->ExtraExtinguishSecondsPerFireStage;
    }

    return FMath::Max(0.0f, RequiredSec);
}

float ADCFireHazardActor::GetHoldProgressNormalized() const
{
    if (CurrentStage == EDCFireStage::Extinguished)
    {
        return 1.0f;
    }

    const float RequiredSec = GetEffectiveRequiredSpraySeconds();
    if (RequiredSec <= KINDA_SMALL_NUMBER)
    {
        return 0.0f;
    }

    return FMath::Clamp(RetainedSpraySeconds / RequiredSec, 0.0f, 1.0f);
}

float ADCFireHazardActor::GetBaseFireGlowIntensity() const
{
    if (!bSpotEnabledInLevel || CurrentStage == EDCFireStage::Extinguished)
    {
        return 0.0f;
    }

    switch (CurrentStage)
    {
        case EDCFireStage::InjuredCivilian:
            return 6.5f;
        case EDCFireStage::KilledCivilian:
            return 9.0f;
        case EDCFireStage::RoomEngulfed:
            return 12.0f;
        case EDCFireStage::Active:
        default:
            return 4.5f;
    }
}

void ADCFireHazardActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    FireFlickerTimeSeconds += FMath::Max(0.0f, DeltaSeconds);

    if (FireGlowLight)
    {
        const bool bGlowActive = bSpotEnabledInLevel && (CurrentStage != EDCFireStage::Extinguished);
        if (bGlowActive)
        {
            const float Phase = FireFlickerTimeSeconds + static_cast<float>(FireSpotIndex) * 1.37f;
            const float Flicker = 1.0f + 0.16f * FMath::Sin(Phase * 11.0f) + 0.08f * FMath::Sin(Phase * 23.0f);
            FireGlowLight->SetVisibility(true);
            FireGlowLight->SetIntensity(GetBaseFireGlowIntensity() * Flicker);
        }
        else
        {
            FireGlowLight->SetVisibility(false);
            FireGlowLight->SetIntensity(0.0f);
        }
    }

    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    if (!TakeSubsystem || !TakeSubsystem->IsActiveTake() || !bSpotEnabledInLevel)
    {
        return;
    }

    if (CurrentStage == EDCFireStage::Extinguished || CurrentStage == EDCFireStage::RoomEngulfed)
    {
        return;
    }

    const UDCLevelConfigData* Config = TakeSubsystem->GetActiveConfig();
    if (!bIsSpraying
        && RetainedSpraySeconds > 0.0f
        && Config
        && Config->ExtinguishInterruptPolicy == EDCExtinguishInterruptPolicy::DecayOverTime)
    {
        const float DecayRate = FMath::Max(0.0f, Config->ExtinguishProgressDecayPerSecond);
        RetainedSpraySeconds = FMath::Max(0.0f, RetainedSpraySeconds - DecayRate * DeltaSeconds);
    }
}

void ADCFireHazardActor::HandleSceneStatusChanged(EDCFireStage FireStage, EDCCivilianState NewCivilianState)
{
    if (!bSpotEnabledInLevel || CurrentStage == EDCFireStage::Extinguished)
    {
        return;
    }

    if (FireStage == EDCFireStage::Extinguished)
    {
        return;
    }

    if (CurrentStage != FireStage)
    {
        CurrentStage = FireStage;
        if (CurrentStage == EDCFireStage::RoomEngulfed && bIsSpraying)
        {
            StopSprayingInternal(SprayingInteractor.Get());
        }
        UpdateFireVisualsForStage();
        UpdateFireBlockerCollision();
    }
}

void ADCFireHazardActor::EvaluateStageForElapsedTime(float ElapsedSeconds)
{
    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    if (!TakeSubsystem || !bSpotEnabledInLevel || CurrentStage == EDCFireStage::Extinguished || CurrentStage == EDCFireStage::RoomEngulfed)
    {
        return;
    }

    const UDCLevelConfigData* Config = TakeSubsystem->GetActiveConfig();
    const float InjureSec = Config ? Config->SecondsToInjureCivilian : 7.2f;
    const float KillSec = Config ? Config->SecondsToKillCivilian : 18.5f;
    const float EngulfSec = TakeSubsystem->GetEffectiveSecondsToEngulf();

    EDCFireStage TargetStage = EDCFireStage::Active;
    if (ElapsedSeconds >= EngulfSec)
    {
        TargetStage = EDCFireStage::RoomEngulfed;
    }
    else if (ElapsedSeconds >= KillSec)
    {
        TargetStage = EDCFireStage::KilledCivilian;
    }
    else if (ElapsedSeconds >= InjureSec)
    {
        TargetStage = EDCFireStage::InjuredCivilian;
    }

    if (CurrentStage != TargetStage)
    {
        CurrentStage = TargetStage;
        if (CurrentStage == EDCFireStage::RoomEngulfed && bIsSpraying)
        {
            StopSprayingInternal(SprayingInteractor.Get());
        }
        UpdateFireVisualsForStage();
        UpdateFireBlockerCollision();
    }
}

bool ADCFireHazardActor::IsActionAvailable(const APawn* Interactor) const
{
    return bSpotEnabledInLevel
        && CurrentStage != EDCFireStage::Extinguished
        && CurrentStage != EDCFireStage::RoomEngulfed;
}

bool ADCFireHazardActor::StartSprayingInternal(APawn* Interactor)
{
    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    const UDCLevelConfigData* Config = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : nullptr;

    if (IsValid(Interactor) && Config)
    {
        if (UDCActionStateComponent* ActionState = Interactor->FindComponentByClass<UDCActionStateComponent>())
        {
            if (Config->EscortToolPolicy == EDCEscortToolPolicy::HideToolAndDisableSpray
                && ActionState->HasCapability(DCGameplayTags::State_EscortingCivilian))
            {
                if (!CannotExtinguishWhileEscortingThoughtText.IsEmpty())
                {
                    ActionState->RequestThoughtBubble(
                        CannotExtinguishWhileEscortingThoughtText,
                        Config->ThoughtBubbleDurationSeconds);
                }
                return false;
            }
        }
    }

    if (GetEffectiveRequiredSpraySeconds() <= KINDA_SMALL_NUMBER)
    {
        CompleteExtinguishNow();
        return false;
    }

    bIsSpraying = true;
    SprayingInteractor = Interactor;
    if (IsValid(Interactor))
    {
        if (UDCActionStateComponent* ActionState = Interactor->FindComponentByClass<UDCActionStateComponent>())
        {
            ActionState->GrantCapability(DCGameplayTags::State_SprayingFire);
        }
    }

    UpdateFireVisualsForStage();
    return true;
}

void ADCFireHazardActor::StopSprayingInternal(APawn* Interactor)
{
    if (!bIsSpraying && !SprayingInteractor.IsValid())
    {
        return;
    }

    bIsSpraying = false;
    APawn* PawnToClear = IsValid(Interactor) ? Interactor : SprayingInteractor.Get();
    if (IsValid(PawnToClear))
    {
        if (UDCActionStateComponent* ActionState = PawnToClear->FindComponentByClass<UDCActionStateComponent>())
        {
            ActionState->RevokeCapability(DCGameplayTags::State_SprayingFire);
        }
    }
    SprayingInteractor.Reset();
    UpdateFireVisualsForStage();
}

bool ADCFireHazardActor::BeginHoldInteraction(APawn* Interactor)
{
    if (!Super::BeginHoldInteraction(Interactor))
    {
        return false;
    }

    return StartSprayingInternal(Interactor);
}

void ADCFireHazardActor::TickHoldInteraction(APawn* Interactor, float DeltaSeconds)
{
    if (!bIsSpraying || !bSpotEnabledInLevel || CurrentStage == EDCFireStage::Extinguished || CurrentStage == EDCFireStage::RoomEngulfed)
    {
        return;
    }

    RetainedSpraySeconds += FMath::Max(0.0f, DeltaSeconds);
    if (RetainedSpraySeconds >= GetEffectiveRequiredSpraySeconds())
    {
        CompleteExtinguishNow();
    }
}

void ADCFireHazardActor::EndHoldInteraction(APawn* Interactor, bool bCancelled)
{
    // Retain RetainedSpraySeconds per spot on interruption so players can resume later.
    StopSprayingInternal(Interactor);
}

bool ADCFireHazardActor::ExecuteInteraction(APawn* Interactor)
{
    if (!IsActionAvailable(Interactor))
    {
        return false;
    }

    return StartSprayingInternal(Interactor);
}

void ADCFireHazardActor::CompleteExtinguishNow()
{
    if (!bSpotEnabledInLevel || CurrentStage == EDCFireStage::Extinguished || CurrentStage == EDCFireStage::RoomEngulfed)
    {
        return;
    }

    APawn* ActivePawn = SprayingInteractor.Get();
    bIsSpraying = false;
    SprayingInteractor.Reset();
    if (IsValid(ActivePawn))
    {
        if (UDCActionStateComponent* ActionState = ActivePawn->FindComponentByClass<UDCActionStateComponent>())
        {
            ActionState->RevokeCapability(DCGameplayTags::State_SprayingFire);
        }
    }

    CurrentStage = EDCFireStage::Extinguished;
    UpdateFireVisualsForStage();
    UpdateFireBlockerCollision();

    if (UWorld* World = GetWorld())
    {
        if (UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            const UDCLevelConfigData* Config = TakeSubsystem->GetActiveConfig();
            if (World->IsGameWorld() && Config && Config->FireExtinguishedSound)
            {
                UGameplayStatics::PlaySound2D(this, Config->FireExtinguishedSound);
            }
            TakeSubsystem->NotifySingleFireSpotExtinguished();
        }
    }
}

void ADCFireHazardActor::UpdateFireBlockerCollision()
{
    if (!FireBlockerBox)
    {
        return;
    }

    const bool bShouldBlock = bSpotEnabledInLevel && (CurrentStage != EDCFireStage::Extinguished);
    if (!bShouldBlock)
    {
        FireBlockerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        return;
    }

    float StageScale = 1.0f;
    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    const UDCLevelConfigData* Config = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : nullptr;

    if (Config && Config->bScaleFireBlockerWithStage)
    {
        const float StepMult = FMath::Clamp(Config->FireBlockerStageScaleMultiplier, 1.0f, 1.35f);
        if (CurrentStage == EDCFireStage::InjuredCivilian)
        {
            StageScale = StepMult;
        }
        else if (CurrentStage == EDCFireStage::KilledCivilian || CurrentStage == EDCFireStage::RoomEngulfed)
        {
            StageScale = StepMult * StepMult;
        }
        // Clamp to preserve safe S-curve corridor clearance (> 52cm pawn diameter) at every stage.
        StageScale = FMath::Clamp(StageScale, 1.0f, 1.45f);
    }

    FireBlockerBox->SetBoxExtent(FVector(
        BaseBlockerExtent.X * StageScale,
        BaseBlockerExtent.Y * StageScale,
        BaseBlockerExtent.Z));
    FireBlockerBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    FireBlockerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    FireBlockerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

void ADCFireHazardActor::UpdateFireVisualsForStage()
{
    const bool bVisible = bSpotEnabledInLevel && (CurrentStage != EDCFireStage::Extinguished);

    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    const UDCLevelConfigData* Config = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : nullptr;
    const EDCFireVisualMode VisualMode = Config ? Config->FireVisualMode : EDCFireVisualMode::Paper2DPlaceholder;

    float ScaleMult = 1.0f;
    float PaperScaleMult = 1.0f;
    float LightIntensity = 3.5f;
    if (CurrentStage == EDCFireStage::InjuredCivilian)
    {
        ScaleMult = 1.35f;
        PaperScaleMult = 1.25f;
        LightIntensity = 5.0f;
    }
    else if (CurrentStage == EDCFireStage::KilledCivilian)
    {
        ScaleMult = 1.75f;
        PaperScaleMult = 1.50f;
        LightIntensity = 6.5f;
    }
    else if (CurrentStage == EDCFireStage::RoomEngulfed)
    {
        ScaleMult = 2.60f;
        PaperScaleMult = 1.90f;
        LightIntensity = 8.5f;
    }

    if (PaperVisual)
    {
        const bool bShowPaper = bVisible && (VisualMode == EDCFireVisualMode::Paper2DPlaceholder);
        PaperVisual->SetCardSize(BasePaperSizeCm * PaperScaleMult);
        if (bIsSpraying)
        {
            PaperVisual->SetPaperState(TEXT("Spraying"));
        }
        else if (CurrentStage == EDCFireStage::InjuredCivilian)
        {
            PaperVisual->SetPaperState(TEXT("InjuredCivilian"));
        }
        else if (CurrentStage == EDCFireStage::KilledCivilian)
        {
            PaperVisual->SetPaperState(TEXT("KilledCivilian"));
        }
        else if (CurrentStage == EDCFireStage::RoomEngulfed)
        {
            PaperVisual->SetPaperState(TEXT("RoomEngulfed"));
        }
        else
        {
            PaperVisual->SetPaperState(NAME_None);
        }
        PaperVisual->SetPaperVisible(bShowPaper);
    }

    if (SprayStreamMesh)
    {
        SprayStreamMesh->SetVisibility(bIsSpraying && bVisible);
    }

    const bool bShow3DFire = bVisible && (VisualMode == EDCFireVisualMode::Stylized3DPlaceholder);
    if (FireCoreMesh)
    {
        FireCoreMesh->SetVisibility(bShow3DFire);
        FireCoreMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f * ScaleMult));
        FireCoreMesh->SetRelativeScale3D(FVector(0.95f * ScaleMult, 0.95f * ScaleMult, 1.30f * ScaleMult));
    }

    if (FireOuterMesh)
    {
        FireOuterMesh->SetVisibility(bShow3DFire);
        FireOuterMesh->SetRelativeLocation(FVector(25.0f * ScaleMult, -32.0f * ScaleMult, 48.0f * ScaleMult));
        FireOuterMesh->SetRelativeScale3D(FVector(0.65f * ScaleMult, 0.65f * ScaleMult, 0.95f * ScaleMult));
    }

    if (FireLight)
    {
        FireLight->SetVisibility(bVisible);
        FireLight->SetIntensity(bVisible ? LightIntensity : 0.0f);
    }

    if (FireGlowLight)
    {
        FireGlowLight->SetVisibility(bVisible);
        FireGlowLight->SetIntensity(bVisible ? GetBaseFireGlowIntensity() : 0.0f);
    }
}

void ADCFireHazardActor::ResetInteractable()
{
    APawn* ActivePawn = SprayingInteractor.Get();
    bIsSpraying = false;
    SprayingInteractor.Reset();
    if (IsValid(ActivePawn))
    {
        if (UDCActionStateComponent* ActionState = ActivePawn->FindComponentByClass<UDCActionStateComponent>())
        {
            ActionState->RevokeCapability(DCGameplayTags::State_SprayingFire);
        }
    }

    RetainedSpraySeconds = 0.0f;
    CurrentStage = EDCFireStage::Active;
    UpdateFireVisualsForStage();
    UpdateFireBlockerCollision();
}

ADCCivilianActor::ADCCivilianActor()
{
    PrimaryActorTick.bCanEverTick = false;

    PaperVisual = CreateDefaultSubobject<UDCPaperVisualComponent>(TEXT("PaperVisual"));
    PaperVisual->SetupAttachment(SceneRoot);
    PaperVisual->CardSizeCm = FVector2D(76.0f, 162.0f);
    PaperVisual->GrayboxTint = FLinearColor(0.10f, 0.80f, 0.62f, 1.0f);
    PaperVisual->StateTints.Add(TEXT("Injured"), FLinearColor(0.95f, 0.65f, 0.15f, 1.0f));
    PaperVisual->StateTints.Add(TEXT("Dead"), FLinearColor::FromSRGBColor(FColor(0x6B, 0x62, 0x5C, 0xFF)));
    PaperVisual->StateTints.Add(TEXT("Rescued"), FLinearColor(0.20f, 0.90f, 0.45f, 1.0f));

    Rule.ActionTag = DCGameplayTags::Action_Civilian_Rescue;
    Rule.InputMode = EDCInteractionInputMode::InstantPress;
    Rule.PromptText = NSLOCTEXT("DirectorCut", "CivilianRescuePrompt", "RESCUE");
    Rule.InteractionRadius = 175.0f;
    Rule.PromptWorldOffset = FVector(0.0f, 0.0f, 185.0f);
    Rule.PersonaBlockedThoughtText = NSLOCTEXT(
        "DirectorCut",
        "CivilianSafetyDogmaBlockedThought",
        "Wait—Rule #1! Clear ALL flames before hugging a guy in pink-heart boxers!");
}

void ADCCivilianActor::BeginPlay()
{
    Super::BeginPlay();

    if (!bHasCapturedInitialLocation)
    {
        InitialLocation = GetActorLocation();
        bHasCapturedInitialLocation = true;
    }

    if (UWorld* World = GetWorld())
    {
        if (UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            BoundTakeSubsystem = TakeSubsystem;
            TakeSubsystem->OnSceneStatusChanged.AddUniqueDynamic(this, &ADCCivilianActor::HandleSceneStatusChanged);
        }
    }
}

void ADCCivilianActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UDCTakeSubsystem* TakeSubsystem = BoundTakeSubsystem.Get())
    {
        TakeSubsystem->OnSceneStatusChanged.RemoveDynamic(this, &ADCCivilianActor::HandleSceneStatusChanged);
    }
    BoundTakeSubsystem.Reset();

    Super::EndPlay(EndPlayReason);
}

bool ADCCivilianActor::IsActionAvailable(const APawn* Interactor) const
{
    return CivilianState == EDCCivilianState::Uninjured
        || CivilianState == EDCCivilianState::Injured;
}

EDCInteractionResult ADCCivilianActor::EvaluateInteraction(const APawn* Interactor) const
{
    const EDCInteractionResult BaseResult = Super::EvaluateInteraction(Interactor);
    if (BaseResult != EDCInteractionResult::Allowed)
    {
        return BaseResult;
    }

    const UDCPersonaComponent* PersonaComp = Interactor ? Interactor->FindComponentByClass<UDCPersonaComponent>() : nullptr;
    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    if (PersonaComp
        && PersonaComp->HasPersonaTag(DCGameplayTags::Persona_Limit_MustClearFireFirst.GetTag())
        && TakeSubsystem
        && TakeSubsystem->GetRemainingFireSpotCount() > 0)
    {
        return EDCInteractionResult::Denied_BlockedByPersona;
    }

    return EDCInteractionResult::Allowed;
}

bool ADCCivilianActor::ExecuteInteraction(APawn* Interactor)
{
    if (!IsValid(Interactor) || !IsActionAvailable(Interactor))
    {
        return false;
    }

    PreEscortState = CivilianState;
    const bool bWasUninjured = (PreEscortState == EDCCivilianState::Uninjured);
    CivilianState = bWasUninjured
        ? EDCCivilianState::EscortingUninjured
        : EDCCivilianState::EscortingInjured;
    EscortingPawn = Interactor;

    if (UDCActionStateComponent* ActionState = Interactor->FindComponentByClass<UDCActionStateComponent>())
    {
        ActionState->GrantCapability(DCGameplayTags::State_EscortingCivilian);
        if (!bWasUninjured)
        {
            ActionState->GrantCapability(DCGameplayTags::State_EscortingInjured);
        }
        ActionState->SetEscortedActor(this);

        const UDCPersonaComponent* PersonaComp = Interactor->FindComponentByClass<UDCPersonaComponent>();
        if (!PersonaComp || !PersonaComp->HasPersonaTag(DCGameplayTags::Persona_Trait_StrongLegs.GetTag()))
        {
            ActionState->RequestThoughtBubble(
                NSLOCTEXT(
                    "DirectorCut",
                    "CivilianNoodleLegsThought",
                    "Oof... noodle legs!! Did I forget all my puppy-carrying stair workouts?!"),
                3.2f);
        }
    }

    // Hide standalone civilian visual while player wears the single Combined Escort paper card.
    if (PaperVisual)
    {
        PaperVisual->SetPaperVisible(false);
    }

    if (UDCTakeSubsystem* TakeSubsystem = BoundTakeSubsystem.Get())
    {
        TakeSubsystem->NotifyCivilianStateChanged(CivilianState);
    }

    return true;
}

void ADCCivilianActor::HandleSceneStatusChanged(EDCFireStage FireStage, EDCCivilianState NewCivilianState)
{
    if (IsEscorting()
        || CivilianState == EDCCivilianState::RescuedUninjured
        || CivilianState == EDCCivilianState::RescuedInjured)
    {
        return;
    }

    if (NewCivilianState == EDCCivilianState::Injured && CivilianState != EDCCivilianState::Injured)
    {
        CivilianState = EDCCivilianState::Injured;
        if (PaperVisual)
        {
            PaperVisual->SetPaperState(TEXT("Injured"));
            PaperVisual->SetPaperVisible(true);
        }
    }
    else if (NewCivilianState == EDCCivilianState::Dead && CivilianState != EDCCivilianState::Dead)
    {
        CivilianState = EDCCivilianState::Dead;
        if (PaperVisual)
        {
            PaperVisual->SetPaperState(TEXT("Dead"));
            PaperVisual->SetPaperVisible(true);
        }
    }
}

void ADCCivilianActor::CompleteRescue(const FVector& SafeParkLocation)
{
    if (CivilianState != EDCCivilianState::Uninjured
        && CivilianState != EDCCivilianState::Injured
        && CivilianState != EDCCivilianState::EscortingUninjured
        && CivilianState != EDCCivilianState::EscortingInjured)
    {
        return;
    }

    const bool bWasUninjured =
        (CivilianState == EDCCivilianState::EscortingUninjured || CivilianState == EDCCivilianState::Uninjured);
    CivilianState = bWasUninjured ? EDCCivilianState::RescuedUninjured : EDCCivilianState::RescuedInjured;
    ClearEscortFromPawn();

    SetActorLocation(FVector(SafeParkLocation.X, SafeParkLocation.Y, InitialLocation.Z));
    if (PaperVisual)
    {
        PaperVisual->SetPaperState(TEXT("Rescued"));
        PaperVisual->SetFacingLeft(false);
        PaperVisual->SetPaperVisible(true);
    }

    if (UWorld* World = GetWorld())
    {
        if (UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            const UDCLevelConfigData* Config = TakeSubsystem->GetActiveConfig();
            int32 RescueStars = Config ? Config->StarsForRescueAlive : 1;
            if (bWasUninjured)
            {
                RescueStars += (Config ? Config->BonusStarsForRescueUninjured : 1);
            }

            const EDCStarChangeReason Reason = bWasUninjured
                ? EDCStarChangeReason::CivilianRescuedUninjured
                : EDCStarChangeReason::CivilianRescuedInjured;

            TakeSubsystem->NotifyCivilianStateChanged(CivilianState);
            TakeSubsystem->AwardStars(RescueStars, Reason);
        }
    }
}

void ADCCivilianActor::ClearEscortFromPawn()
{
    if (APawn* Pawn = EscortingPawn.Get())
    {
        if (UDCActionStateComponent* ActionState = Pawn->FindComponentByClass<UDCActionStateComponent>())
        {
            ActionState->RevokeCapability(DCGameplayTags::State_EscortingCivilian);
            ActionState->RevokeCapability(DCGameplayTags::State_EscortingInjured);
            if (ActionState->GetEscortedActor() == this)
            {
                ActionState->SetEscortedActor(nullptr);
            }
        }
    }
    EscortingPawn.Reset();
}

void ADCCivilianActor::ResetInteractable()
{
    ClearEscortFromPawn();
    CivilianState = EDCCivilianState::Uninjured;
    PreEscortState = EDCCivilianState::Uninjured;

    if (bHasCapturedInitialLocation)
    {
        SetActorLocation(InitialLocation);
    }
    if (PaperVisual)
    {
        PaperVisual->SetPaperState(NAME_None);
        PaperVisual->SetFacingLeft(false);
        PaperVisual->SetPaperVisible(true);
    }
}

ADCSafeZoneVolume::ADCSafeZoneVolume()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    TriggerBox->SetupAttachment(SceneRoot);
    TriggerBox->SetRelativeLocation(FVector(0.0f, 0.0f, 110.0f));
    TriggerBox->SetBoxExtent(FVector(110.0f, 85.0f, 110.0f));
    TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TriggerBox->SetCollisionResponseToAllChannels(ECR_Overlap);

    FloorPadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FloorPadMesh"));
    FloorPadMesh->SetupAttachment(SceneRoot);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded())
    {
        FloorPadMesh->SetStaticMesh(CubeFinder.Object);
    }
    FloorPadMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 1.5f));
    FloorPadMesh->SetRelativeScale3D(FVector(2.2f, 1.7f, 0.03f));
    FloorPadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FloorPadMesh->SetGenerateOverlapEvents(false);
}

void ADCSafeZoneVolume::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyPadMaterial();
}

void ADCSafeZoneVolume::BeginPlay()
{
    Super::BeginPlay();
    ApplyPadMaterial();
}

void ADCSafeZoneVolume::ApplyPadMaterial()
{
    if (!FloorPadMesh)
    {
        return;
    }

    UMaterialInterface* SafeMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_SafeZone.M_SafeZone"));
    if (!SafeMat)
    {
        SafeMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Entry.M_Entry"));
    }
    if (SafeMat)
    {
        FloorPadMesh->SetMaterial(0, SafeMat);
    }
}

bool ADCSafeZoneVolume::IsPointInsideZone2D(const FVector& WorldPoint) const
{
    if (!TriggerBox)
    {
        return false;
    }

    const FVector Center = TriggerBox->GetComponentLocation();
    const FVector Extent = TriggerBox->GetScaledBoxExtent();
    return FMath::Abs(WorldPoint.X - Center.X) <= Extent.X && FMath::Abs(WorldPoint.Y - Center.Y) <= Extent.Y;
}

void ADCSafeZoneVolume::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    if (!TakeSubsystem || !TakeSubsystem->IsActiveTake())
    {
        return;
    }

    for (TActorIterator<APawn> It(World); It; ++It)
    {
        APawn* PlayerPawn = *It;
        if (!IsValid(PlayerPawn))
        {
            continue;
        }

        const UDCActionStateComponent* ActionState = PlayerPawn->FindComponentByClass<UDCActionStateComponent>();
        if (!ActionState || !ActionState->HasCapability(DCGameplayTags::State_EscortingCivilian))
        {
            continue;
        }

        if (!IsPointInsideZone2D(PlayerPawn->GetActorLocation()))
        {
            continue;
        }

        if (ADCCivilianActor* EscortedCivilian = Cast<ADCCivilianActor>(ActionState->GetEscortedActor()))
        {
            if (EscortedCivilian->IsEscorting())
            {
                EscortedCivilian->CompleteRescue(GetActorLocation() + CivilianParkOffset);
                continue;
            }
        }

        for (TActorIterator<ADCCivilianActor> CivIt(World); CivIt; ++CivIt)
        {
            if (ADCCivilianActor* Civilian = *CivIt)
            {
                if (Civilian->IsEscorting() && Civilian->GetEscortingPawn() == PlayerPawn)
                {
                    Civilian->CompleteRescue(GetActorLocation() + CivilianParkOffset);
                }
            }
        }
    }
}

ADCMatchGirlActor::ADCMatchGirlActor()
{
    PrimaryActorTick.bCanEverTick = false;

    PaperVisual = CreateDefaultSubobject<UDCPaperVisualComponent>(TEXT("PaperVisual"));
    PaperVisual->SetupAttachment(SceneRoot);
    PaperVisual->CardSizeCm = FVector2D(132.0f, 152.0f);
    PaperVisual->GrayboxTint = FLinearColor(0.95f, 0.78f, 0.35f, 1.0f);

    Rule.ActionTag = DCGameplayTags::Action_MatchGirl_Talk;
    Rule.InputMode = EDCInteractionInputMode::InstantPress;
    Rule.PromptText = NSLOCTEXT("DirectorCut", "MatchGirlTalkPrompt", "TALK");
    Rule.InteractionRadius = 155.0f;
    Rule.PromptWorldOffset = FVector(0.0f, 0.0f, 168.0f);
    Rule.SuccessThoughtText = NSLOCTEXT(
        "DirectorCut",
        "MatchGirlTalkThought",
        "Kid, quit lighting matches for imaginary roast chicken—pants-guy is cooking for real!!");
    Rule.SuccessThoughtDuration = 2.8f;
}

bool ADCMatchGirlActor::IsActionAvailable(const APawn* Interactor) const
{
    return true;
}

bool ADCMatchGirlActor::ExecuteInteraction(APawn* Interactor)
{
    return true;
}

ADCAngelDogActor::ADCAngelDogActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    PaperVisual = CreateDefaultSubobject<UDCPaperVisualComponent>(TEXT("PaperVisual"));
    PaperVisual->SetupAttachment(SceneRoot);
    PaperVisual->CardSizeCm = FVector2D(92.0f, 96.0f);
    PaperVisual->GrayboxTint = FLinearColor(0.96f, 0.90f, 0.74f, 1.0f);

    SetActorEnableCollision(false);
}

void ADCAngelDogActor::BeginPlay()
{
    Super::BeginPlay();

    if (UWorld* World = GetWorld())
    {
        if (UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            BoundTakeSubsystem = TakeSubsystem;
            TakeSubsystem->OnSceneStatusChanged.AddUniqueDynamic(this, &ADCAngelDogActor::HandleSceneStatusChanged);
            TakeSubsystem->OnTakeReset.AddUniqueDynamic(this, &ADCAngelDogActor::HandleTakeReset);
        }
    }
}

void ADCAngelDogActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UDCTakeSubsystem* TakeSubsystem = BoundTakeSubsystem.Get())
    {
        TakeSubsystem->OnSceneStatusChanged.RemoveDynamic(this, &ADCAngelDogActor::HandleSceneStatusChanged);
        TakeSubsystem->OnTakeReset.RemoveDynamic(this, &ADCAngelDogActor::HandleTakeReset);
    }
    BoundTakeSubsystem.Reset();

    Super::EndPlay(EndPlayReason);
}

void ADCAngelDogActor::HandleSceneStatusChanged(EDCFireStage FireStage, EDCCivilianState CivilianState)
{
    if (!PaperVisual)
    {
        return;
    }

    if (CivilianState == EDCCivilianState::RescuedUninjured
        || CivilianState == EDCCivilianState::RescuedInjured)
    {
        PaperVisual->SetPaperState(TEXT("Happy"));
    }
}

void ADCAngelDogActor::HandleTakeReset()
{
    if (PaperVisual)
    {
        PaperVisual->SetPaperState(NAME_None);
    }
}
