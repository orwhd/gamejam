#include "Characters/DCPlayerCharacter.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Core/DCGameplayTags.h"
#include "Core/DCTakeSubsystem.h"
#include "Data/DCLevelConfigData.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Interaction/DCActionStateComponent.h"
#include "Interaction/DCInteractableBase.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Persona/DCPersonaComponent.h"
#include "Sound/SoundBase.h"
#include "Spatial/DCFixedStageCamera.h"
#include "Spatial/DCPaperVisualComponent.h"
#include "UI/DCHudOverlayWidget.h"
#include "UI/DCInteractPromptWidget.h"
#include "UI/DCThoughtBubbleWidget.h"
#include "UObject/ConstructorHelpers.h"

ADCPlayerCharacter::ADCPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(26.0f, 86.0f);

    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetVisibility(false);
        MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = 320.0f;
        MoveComp->MaxAcceleration = 2048.0f;
        MoveComp->BrakingDecelerationWalking = 2048.0f;
        MoveComp->bOrientRotationToMovement = false;
        MoveComp->JumpZVelocity = 0.0f;
        MoveComp->NavAgentProps.bCanJump = false;
    }

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    PersonaComponent = CreateDefaultSubobject<UDCPersonaComponent>(TEXT("PersonaComponent"));
    ActionStateComponent = CreateDefaultSubobject<UDCActionStateComponent>(TEXT("ActionStateComponent"));

    PaperVisual = CreateDefaultSubobject<UDCPaperVisualComponent>(TEXT("PaperVisual"));
    PaperVisual->SetupAttachment(GetCapsuleComponent());
    PaperVisual->SetRelativeLocation(FVector(0.0f, 0.0f, -86.0f));
    PaperVisual->CardSizeCm = FVector2D(82.0f, 172.0f);
    PaperVisual->GrayboxTint = FLinearColor(0.95f, 0.50f, 0.10f, 1.0f);

    Fallback3DRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Fallback3DRoot"));
    Fallback3DRoot->SetupAttachment(GetCapsuleComponent());
    Fallback3DRoot->SetRelativeLocation(FVector(0.0f, 0.0f, -86.0f));
    Fallback3DRoot->SetVisibility(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UStaticMesh* CubeMesh = CubeMeshFinder.Succeeded() ? CubeMeshFinder.Object : nullptr;
    UStaticMesh* CylinderMesh = CylinderMeshFinder.Succeeded() ? CylinderMeshFinder.Object : nullptr;

    auto MakeFallbackPart = [this](const TCHAR* PartName, UStaticMesh* PartMesh, const FVector& PartRelLoc, const FVector& PartRelScale) -> UStaticMeshComponent*
    {
        UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(PartName);
        Comp->SetupAttachment(Fallback3DRoot);
        if (PartMesh)
        {
            Comp->SetStaticMesh(PartMesh);
        }
        Comp->SetRelativeLocation(PartRelLoc);
        Comp->SetRelativeScale3D(PartRelScale);
        Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Comp->SetGenerateOverlapEvents(false);
        Comp->SetCastShadow(true);
        Comp->SetVisibility(false);
        return Comp;
    };

    Fallback3DBodyMesh = MakeFallbackPart(TEXT("Fallback3DBodyMesh"), CubeMesh, FVector(0.0f, 0.0f, 52.0f), FVector(0.36f, 0.44f, 0.86f));
    Fallback3DHeadMesh = MakeFallbackPart(TEXT("Fallback3DHeadMesh"), CubeMesh, FVector(0.0f, 0.0f, 114.0f), FVector(0.36f, 0.36f, 0.36f));
    Fallback3DHelmetMesh = MakeFallbackPart(TEXT("Fallback3DHelmetMesh"), CubeMesh, FVector(0.0f, 0.0f, 138.0f), FVector(0.44f, 0.44f, 0.14f));
    Fallback3DToolMesh = MakeFallbackPart(TEXT("Fallback3DToolMesh"), CylinderMesh, FVector(12.0f, 28.0f, 66.0f), FVector(0.14f, 0.14f, 0.46f));
    Fallback3DBuddyMesh = MakeFallbackPart(TEXT("Fallback3DBuddyMesh"), CubeMesh, FVector(-20.0f, 0.0f, 88.0f), FVector(0.30f, 0.36f, 0.58f));

    ThoughtWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("ThoughtWidgetComponent"));
    ThoughtWidgetComponent->SetupAttachment(GetCapsuleComponent());
    ThoughtWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 142.0f));
    ThoughtWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    ThoughtWidgetComponent->SetDrawAtDesiredSize(true);
    ThoughtWidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));
    ThoughtWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ThoughtWidgetComponent->SetVisibility(false);

    InteractPromptWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractPromptWidgetComponent"));
    InteractPromptWidgetComponent->SetupAttachment(GetCapsuleComponent());
    InteractPromptWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 96.0f));
    InteractPromptWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    InteractPromptWidgetComponent->SetDrawAtDesiredSize(true);
    InteractPromptWidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));
    InteractPromptWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    InteractPromptWidgetComponent->SetVisibility(false);
}

void ADCPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (UWorld* World = GetWorld())
    {
        if (UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            TakeSubsystem->OnTakePhaseChanged.AddUniqueDynamic(this, &ADCPlayerCharacter::HandleTakePhaseChanged);
            TakeSubsystem->OnTakeReset.AddUniqueDynamic(this, &ADCPlayerCharacter::HandleTakeReset);
        }
    }

    if (ActionStateComponent)
    {
        ActionStateComponent->OnThoughtRequested.AddUniqueDynamic(this, &ADCPlayerCharacter::HandleThoughtRequested);
        ActionStateComponent->OnThoughtCleared.AddUniqueDynamic(this, &ADCPlayerCharacter::HandleThoughtCleared);
        ActionStateComponent->OnPersonaPanelToggled.AddUniqueDynamic(this, &ADCPlayerCharacter::HandlePersonaPanelToggled);
    }

    ApplyFallback3DColors();
    RefreshConfigAndCamera();
    SpawnStageWidgets();

    if (PaperVisual)
    {
        PaperVisual->SetPaperState(NAME_None);
    }

    if (const UWorld* World = GetWorld())
    {
        if (const UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            if (TakeSubsystem->IsPreTake() && ActionStateComponent)
            {
                ActionStateComponent->SetPersonaPanelOpen(true);
            }
        }
    }

    UpdateInputModeForState();
}

void ADCPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            TakeSubsystem->OnTakePhaseChanged.RemoveDynamic(this, &ADCPlayerCharacter::HandleTakePhaseChanged);
            TakeSubsystem->OnTakeReset.RemoveDynamic(this, &ADCPlayerCharacter::HandleTakeReset);
        }
    }

    if (ActionStateComponent)
    {
        ActionStateComponent->OnThoughtRequested.RemoveDynamic(this, &ADCPlayerCharacter::HandleThoughtRequested);
        ActionStateComponent->OnThoughtCleared.RemoveDynamic(this, &ADCPlayerCharacter::HandleThoughtCleared);
        ActionStateComponent->OnPersonaPanelToggled.RemoveDynamic(this, &ADCPlayerCharacter::HandlePersonaPanelToggled);
    }

    CancelActiveHoldInteraction();

    if (HudOverlayWidget)
    {
        HudOverlayWidget->RemoveFromParent();
        HudOverlayWidget = nullptr;
    }

    if (ThoughtWidgetComponent)
    {
        ThoughtWidgetComponent->SetWidget(nullptr);
    }
    ThoughtWidget = nullptr;

    if (InteractPromptWidgetComponent)
    {
        InteractPromptWidgetComponent->SetWidget(nullptr);
    }
    InteractPromptWidget = nullptr;

    Super::EndPlay(EndPlayReason);
}

void ADCPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (!PlayerInputComponent)
    {
        return;
    }

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ADCPlayerCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ADCPlayerCharacter::MoveRight);

    PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &ADCPlayerCharacter::StartInteractInput);
    PlayerInputComponent->BindKey(EKeys::E, IE_Released, this, &ADCPlayerCharacter::StopInteractInput);
    PlayerInputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ADCPlayerCharacter::TogglePersona);
    PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ADCPlayerCharacter::StartTakeAction);
    PlayerInputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ADCPlayerCharacter::StartTakeAction);
    PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &ADCPlayerCharacter::RestartTakeAction);
    PlayerInputComponent->BindKey(EKeys::V, IE_Pressed, this, &ADCPlayerCharacter::ToggleVisualMode);
}

void ADCPlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    const UDCLevelConfigData* Config = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : nullptr;

    if (ThoughtRemainingSeconds > 0.0f)
    {
        ThoughtRemainingSeconds = FMath::Max(0.0f, ThoughtRemainingSeconds - DeltaSeconds);
    }

    if (BlockedReactionRemainingSeconds > 0.0f)
    {
        BlockedReactionRemainingSeconds = FMath::Max(0.0f, BlockedReactionRemainingSeconds - DeltaSeconds);
    }

    if (ThoughtWidgetComponent)
    {
        const bool bShouldShowThought = (ThoughtRemainingSeconds > 0.0f) && (!TakeSubsystem || !TakeSubsystem->IsTakeResult());
        if (ThoughtWidgetComponent->IsVisible() != bShouldShowThought)
        {
            ThoughtWidgetComponent->SetVisibility(bShouldShowThought);
        }
    }

    UpdateActiveHoldInteraction(TakeSubsystem, Config, DeltaSeconds);
    UpdateMovementAndVisualState(Config);
    UpdateAudioFeedback(TakeSubsystem, Config, DeltaSeconds);
    UpdateFocusedInteractable();
    UpdateInteractPromptWidget(TakeSubsystem);
}

void ADCPlayerCharacter::ApplyPlayerPaperTexturesFromConfig(const UDCLevelConfigData* Config)
{
    if (!PaperVisual || !Config)
    {
        return;
    }

    auto MakeSeq = [](UTexture2D* IdleTex, const TArray<TObjectPtr<UTexture2D>>& WalkTexs)
    {
        FDCStateTextureSequence Seq;
        if (IdleTex)
        {
            Seq.IdleFrames.Add(IdleTex);
        }
        Seq.MoveFrames = WalkTexs;
        return Seq;
    };

    if (Config->PlayerDefaultPaperTexture)
    {
        PaperVisual->DefaultPaperTexture = Config->PlayerDefaultPaperTexture;
    }
    if (Config->PlayerIdleCycleTextures.Num() > 0)
    {
        PaperVisual->IdleCycleTextures = Config->PlayerIdleCycleTextures;
    }
    if (Config->PlayerWalkCycleTextures.Num() > 0)
    {
        PaperVisual->WalkCycleTextures = Config->PlayerWalkCycleTextures;
    }
    if (Config->PlayerWaterJetOverlayTexture)
    {
        PaperVisual->SprayOverlayTexture = Config->PlayerWaterJetOverlayTexture;
    }
    if (Config->BuddyUninjuredTexture)
    {
        PaperVisual->BuddyUninjuredTexture = Config->BuddyUninjuredTexture;
    }
    if (Config->BuddyInjuredTexture)
    {
        PaperVisual->BuddyInjuredTexture = Config->BuddyInjuredTexture;
    }

    if (Config->PlayerScaredTexture)
    {
        PaperVisual->StateTextures.Add(TEXT("Scared"), Config->PlayerScaredTexture);
    }

    if (Config->PlayerToolIdleTexture || Config->PlayerToolWalkCycleTextures.Num() > 0)
    {
        PaperVisual->StateTextureSequences.Add(
            TEXT("HoldingTool"),
            MakeSeq(Config->PlayerToolIdleTexture.Get(), Config->PlayerToolWalkCycleTextures));
    }

    if (Config->PlayerSprayCycleTextures.Num() > 0)
    {
        FDCStateTextureSequence SpraySeq;
        SpraySeq.IdleFrames = Config->PlayerSprayCycleTextures;
        SpraySeq.MoveFrames = Config->PlayerSprayCycleTextures;
        PaperVisual->StateTextureSequences.Add(TEXT("Spraying"), MoveTemp(SpraySeq));
    }

    if (Config->PlayerEscortUninjuredIdleTexture || Config->PlayerEscortUninjuredWalkCycleTextures.Num() > 0)
    {
        const FDCStateTextureSequence EscortUninjuredSeq = MakeSeq(
            Config->PlayerEscortUninjuredIdleTexture.Get(),
            Config->PlayerEscortUninjuredWalkCycleTextures);
        PaperVisual->StateTextureSequences.Add(TEXT("CombinedEscortUninjured"), EscortUninjuredSeq);
        PaperVisual->StateTextureSequences.Add(TEXT("CombinedEscortUninjuredWithTool"), EscortUninjuredSeq);
    }

    if (Config->PlayerEscortInjuredIdleTexture || Config->PlayerEscortInjuredWalkCycleTextures.Num() > 0)
    {
        const FDCStateTextureSequence EscortInjuredSeq = MakeSeq(
            Config->PlayerEscortInjuredIdleTexture.Get(),
            Config->PlayerEscortInjuredWalkCycleTextures);
        PaperVisual->StateTextureSequences.Add(TEXT("CombinedEscortInjured"), EscortInjuredSeq);
        PaperVisual->StateTextureSequences.Add(TEXT("CombinedEscortInjuredWithTool"), EscortInjuredSeq);
    }

    PaperVisual->RefreshVisualAppearance();
}

void ADCPlayerCharacter::RefreshConfigAndCamera()
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>();
    const UDCLevelConfigData* Config = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : nullptr;

    if (Config && GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = Config->PlayerWalkSpeed;
    }

    if (Config)
    {
        SetVisualMode(Config->ProtagonistVisualMode);
    }
    else
    {
        ApplyVisualMode();
    }

    ApplyPlayerPaperTexturesFromConfig(Config);

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        for (TActorIterator<ADCFixedStageCamera> It(const_cast<UWorld*>(World)); It; ++It)
        {
            if (ADCFixedStageCamera* StageCam = *It)
            {
                StageCam->ApplyConfig(Config);
                StageCam->ActivateForPlayer(PC);
                break;
            }
        }
    }
}

void ADCPlayerCharacter::SetVisualMode(EDCCharacterVisualMode NewMode)
{
    VisualMode = NewMode;
    ApplyVisualMode();
}

void ADCPlayerCharacter::ToggleVisualMode()
{
    SetVisualMode(VisualMode == EDCCharacterVisualMode::Paper2D
        ? EDCCharacterVisualMode::Mesh3D
        : EDCCharacterVisualMode::Paper2D);
}

void ADCPlayerCharacter::ApplyVisualMode()
{
    const bool bUseMesh3D = (VisualMode == EDCCharacterVisualMode::Mesh3D);
    if (PaperVisual)
    {
        PaperVisual->SetPaperVisible(!bUseMesh3D);
    }

    if (Fallback3DRoot)
    {
        Fallback3DRoot->SetVisibility(bUseMesh3D, true);
    }

    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    const UDCLevelConfigData* Config = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : nullptr;
    UpdateFallback3DState(Config);
}

void ADCPlayerCharacter::ApplyFallback3DColors()
{
    UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (!BaseMat)
    {
        BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Wall.M_Wall"));
    }
    if (!BaseMat)
    {
        return;
    }

    auto ColorPart = [this, BaseMat](UStaticMeshComponent* Comp, const FLinearColor& Color)
    {
        if (!Comp)
        {
            return;
        }
        UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
        if (!DynMat)
        {
            return;
        }
        FLinearColor DummyColor;
        if (BaseMat->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Color")), DummyColor))
        {
            DynMat->SetVectorParameterValue(TEXT("Color"), Color);
        }
        if (BaseMat->GetVectorParameterValue(FMaterialParameterInfo(TEXT("TintColor")), DummyColor))
        {
            DynMat->SetVectorParameterValue(TEXT("TintColor"), Color);
        }
        if (BaseMat->GetVectorParameterValue(FMaterialParameterInfo(TEXT("BaseColor")), DummyColor))
        {
            DynMat->SetVectorParameterValue(TEXT("BaseColor"), Color);
        }
        Comp->SetMaterial(0, DynMat);
    };

    ColorPart(Fallback3DBodyMesh.Get(), FLinearColor(0.95f, 0.45f, 0.10f, 1.0f));
    ColorPart(Fallback3DHeadMesh.Get(), FLinearColor(0.98f, 0.80f, 0.66f, 1.0f));
    ColorPart(Fallback3DHelmetMesh.Get(), FLinearColor(0.88f, 0.16f, 0.14f, 1.0f));
    ColorPart(Fallback3DToolMesh.Get(), FLinearColor(0.82f, 0.12f, 0.14f, 1.0f));
    ColorPart(Fallback3DBuddyMesh.Get(), FLinearColor(0.95f, 0.65f, 0.72f, 1.0f));
}

void ADCPlayerCharacter::UpdateFallback3DState(const UDCLevelConfigData* Config)
{
    const bool bUseMesh3D = (VisualMode == EDCCharacterVisualMode::Mesh3D);
    if (!bUseMesh3D)
    {
        if (Fallback3DRoot)
        {
            Fallback3DRoot->SetVisibility(false, true);
        }
        return;
    }

    if (Fallback3DRoot)
    {
        Fallback3DRoot->SetVisibility(true, false);
        const bool bFacingLeft = PaperVisual ? PaperVisual->IsFacingLeft() : false;
        Fallback3DRoot->SetRelativeRotation(FRotator(0.0f, bFacingLeft ? 180.0f : 0.0f, 0.0f));
    }

    const bool bEscorting = ActionStateComponent && ActionStateComponent->HasCapability(DCGameplayTags::State_EscortingCivilian);
    const bool bHasTool = ActionStateComponent && ActionStateComponent->HasCapability(DCGameplayTags::Capability_ExtinguishFire);
    const bool bAllowEscortTool = Config && (Config->EscortToolPolicy == EDCEscortToolPolicy::ShowToolAndAllowSpray);

    if (Fallback3DBodyMesh) { Fallback3DBodyMesh->SetVisibility(true); }
    if (Fallback3DHeadMesh) { Fallback3DHeadMesh->SetVisibility(true); }
    if (Fallback3DHelmetMesh) { Fallback3DHelmetMesh->SetVisibility(true); }
    if (Fallback3DToolMesh) { Fallback3DToolMesh->SetVisibility(bHasTool && (!bEscorting || bAllowEscortTool)); }
    if (Fallback3DBuddyMesh) { Fallback3DBuddyMesh->SetVisibility(bEscorting); }
}

void ADCPlayerCharacter::SpawnStageWidgets()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return;
    }

    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    const UDCLevelConfigData* Config = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : nullptr;

    TSubclassOf<UUserWidget> OverlayClass = (Config && Config->HudOverlayWidgetClass)
        ? Config->HudOverlayWidgetClass
        : TSubclassOf<UUserWidget>(UDCHudOverlayWidget::StaticClass());

    if (HudOverlayWidget && HudOverlayWidget->GetClass() != OverlayClass)
    {
        HudOverlayWidget->RemoveFromParent();
        HudOverlayWidget = nullptr;
    }

    if (!HudOverlayWidget && OverlayClass)
    {
        HudOverlayWidget = CreateWidget<UUserWidget>(PC, OverlayClass);
        if (HudOverlayWidget)
        {
            if (UDCHudOverlayWidget* Overlay = Cast<UDCHudOverlayWidget>(HudOverlayWidget))
            {
                Overlay->InitializeForPlayer(this);
            }
            HudOverlayWidget->AddToViewport(0);
        }
    }

    TSubclassOf<UUserWidget> ThoughtClass = (Config && Config->ThoughtBubbleWidgetClass)
        ? Config->ThoughtBubbleWidgetClass
        : TSubclassOf<UUserWidget>(UDCThoughtBubbleWidget::StaticClass());

    if ((!ThoughtWidget || ThoughtWidget->GetClass() != ThoughtClass) && ThoughtClass && ThoughtWidgetComponent)
    {
        ThoughtWidget = CreateWidget<UUserWidget>(PC, ThoughtClass);
        ThoughtWidgetComponent->SetWidget(ThoughtWidget);
        ThoughtWidgetComponent->SetVisibility(false);
    }

    TSubclassOf<UUserWidget> PromptClass = (Config && Config->InteractPromptWidgetClass)
        ? Config->InteractPromptWidgetClass
        : TSubclassOf<UUserWidget>(UDCInteractPromptWidget::StaticClass());

    if ((!InteractPromptWidget || InteractPromptWidget->GetClass() != PromptClass) && PromptClass && InteractPromptWidgetComponent)
    {
        InteractPromptWidget = CreateWidget<UUserWidget>(PC, PromptClass);
        InteractPromptWidgetComponent->SetWidget(InteractPromptWidget);
        InteractPromptWidgetComponent->SetVisibility(false);
    }
}

void ADCPlayerCharacter::GetCameraPlaneAxes(FVector& OutForward, FVector& OutRight) const
{
    OutForward = FVector(1.0f, 0.0f, 0.0f);
    OutRight = FVector(0.0f, 1.0f, 0.0f);

    if (const APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (PC->PlayerCameraManager)
        {
            const FRotator YawOnly(0.0f, PC->PlayerCameraManager->GetCameraRotation().Yaw, 0.0f);
            OutForward = YawOnly.Vector().GetSafeNormal();
            OutRight = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y).GetSafeNormal();
            return;
        }
    }

    if (const UWorld* World = GetWorld())
    {
        for (TActorIterator<ADCFixedStageCamera> It(const_cast<UWorld*>(World)); It; ++It)
        {
            if (const ADCFixedStageCamera* StageCam = *It)
            {
                OutForward = StageCam->GetCameraPlaneForward();
                OutRight = StageCam->GetCameraPlaneRight();
                return;
            }
        }
    }
}

bool ADCPlayerCharacter::CanMove() const
{
    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    if (!TakeSubsystem || !TakeSubsystem->IsActiveTake())
    {
        return false;
    }

    if (ActionStateComponent && ActionStateComponent->IsPersonaPanelOpen())
    {
        return false;
    }

    return true;
}

void ADCPlayerCharacter::MoveForward(float Value)
{
    if (!CanMove() || FMath::IsNearlyZero(Value))
    {
        return;
    }

    FVector CamForward;
    FVector CamRight;
    GetCameraPlaneAxes(CamForward, CamRight);
    AddMovementInput(CamForward, Value);
}

void ADCPlayerCharacter::MoveRight(float Value)
{
    if (!CanMove() || FMath::IsNearlyZero(Value))
    {
        return;
    }

    FVector CamForward;
    FVector CamRight;
    GetCameraPlaneAxes(CamForward, CamRight);
    AddMovementInput(CamRight, Value);
}

float ADCPlayerCharacter::GetInteractableRadius(const ADCInteractableBase* Candidate, const UDCLevelConfigData* Config) const
{
    const float DefaultRadius = Config ? Config->DefaultInteractionRadius : 175.0f;
    return (Candidate && Candidate->Rule.InteractionRadius > 0.0f) ? Candidate->Rule.InteractionRadius : DefaultRadius;
}

ADCInteractableBase* ADCPlayerCharacter::FindNearestAvailableInteractable() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    const UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>();
    const UDCLevelConfigData* Config = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : nullptr;

    ADCInteractableBase* BestTarget = nullptr;
    float BestDistSq = MAX_flt;
    const FVector MyLoc = GetActorLocation();

    for (TActorIterator<ADCInteractableBase> It(World); It; ++It)
    {
        ADCInteractableBase* Candidate = *It;
        if (!IsValid(Candidate) || !Candidate->IsActionAvailable(this))
        {
            continue;
        }

        const float Radius = GetInteractableRadius(Candidate, Config);
        const float Dist2D = FVector::Dist2D(MyLoc, Candidate->GetActorLocation());
        if (Dist2D <= Radius)
        {
            const float DistSq = Dist2D * Dist2D;
            if (DistSq < BestDistSq)
            {
                BestDistSq = DistSq;
                BestTarget = Candidate;
            }
        }
    }

    return BestTarget;
}

void ADCPlayerCharacter::CancelActiveHoldInteraction()
{
    if (!ActionStateComponent)
    {
        return;
    }

    if (ADCInteractableBase* ActiveHold = ActionStateComponent->GetActiveHoldInteractable())
    {
        if (IsValid(ActiveHold))
        {
            ActiveHold->EndHoldInteraction(this, true);
        }
        ActionStateComponent->SetActiveHoldInteractable(nullptr);
    }
}

void ADCPlayerCharacter::UpdateActiveHoldInteraction(
    const UDCTakeSubsystem* TakeSubsystem,
    const UDCLevelConfigData* Config,
    float DeltaSeconds)
{
    if (!ActionStateComponent)
    {
        return;
    }

    ADCInteractableBase* ActiveHold = ActionStateComponent->GetActiveHoldInteractable();
    if (!ActiveHold)
    {
        return;
    }

    const bool bCanContinueHold =
        TakeSubsystem &&
        TakeSubsystem->IsActiveTake() &&
        !ActionStateComponent->IsPersonaPanelOpen() &&
        IsValid(ActiveHold) &&
        ActiveHold->IsActionAvailable(this) &&
        (FVector::Dist2D(GetActorLocation(), ActiveHold->GetActorLocation()) <= GetInteractableRadius(ActiveHold, Config));

    if (!bCanContinueHold)
    {
        CancelActiveHoldInteraction();
        return;
    }

    ActiveHold->TickHoldInteraction(this, DeltaSeconds);
    if (!IsValid(ActiveHold) || !ActiveHold->IsHoldActive())
    {
        ActionStateComponent->SetActiveHoldInteractable(nullptr);
    }
}

void ADCPlayerCharacter::UpdateAudioFeedback(
    const UDCTakeSubsystem* TakeSubsystem,
    const UDCLevelConfigData* Config,
    float DeltaSeconds)
{
    if (FootstepCooldownSeconds > 0.0f)
    {
        FootstepCooldownSeconds = FMath::Max(0.0f, FootstepCooldownSeconds - DeltaSeconds);
    }
    if (WaterSpraySoundCooldownSeconds > 0.0f)
    {
        WaterSpraySoundCooldownSeconds = FMath::Max(0.0f, WaterSpraySoundCooldownSeconds - DeltaSeconds);
    }

    const UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld() || !Config)
    {
        return;
    }

    const bool bInLiveTake =
        TakeSubsystem &&
        TakeSubsystem->IsActiveTake() &&
        (!ActionStateComponent || !ActionStateComponent->IsPersonaPanelOpen());

    const bool bIsMovingHoriz = bInLiveTake && (GetVelocity().SizeSquared2D() > 25.0f);
    if (bIsMovingHoriz && FootstepCooldownSeconds <= 0.0f && Config->FootstepSound)
    {
        UGameplayStatics::PlaySound2D(this, Config->FootstepSound);
        FootstepCooldownSeconds = 0.26f;
    }

    const bool bIsSprayingNow =
        bInLiveTake &&
        ActionStateComponent &&
        ActionStateComponent->HasCapability(DCGameplayTags::State_SprayingFire);

    if (bIsSprayingNow)
    {
        if ((!bWasSprayingForAudio || WaterSpraySoundCooldownSeconds <= 0.0f) && Config->WaterSpraySound)
        {
            UGameplayStatics::PlaySound2D(this, Config->WaterSpraySound);
            WaterSpraySoundCooldownSeconds = 0.45f;
        }
    }
    bWasSprayingForAudio = bIsSprayingNow;
}

void ADCPlayerCharacter::UpdateMovementAndVisualState(const UDCLevelConfigData* Config)
{
    const bool bEscorting = ActionStateComponent && ActionStateComponent->HasCapability(DCGameplayTags::State_EscortingCivilian);
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        const float BaseWalkSpeed = Config ? Config->PlayerWalkSpeed : 320.0f;
        const bool bHasStrongLegs = PersonaComponent && PersonaComponent->HasPersonaTag(DCGameplayTags::Persona_Trait_StrongLegs.GetTag());
        const float EscortWalkSpeed = Config
            ? (bHasStrongLegs ? Config->EscortMoveSpeed : Config->WeakEscortMoveSpeed)
            : (bHasStrongLegs ? 295.0f : 130.0f);
        MoveComp->MaxWalkSpeed = bEscorting ? EscortWalkSpeed : BaseWalkSpeed;
    }

    if (PaperVisual)
    {
        const FVector Velocity = GetVelocity();
        const bool bIsMovingHoriz = Velocity.SizeSquared2D() > 25.0f;
        if (bIsMovingHoriz)
        {
            BlockedReactionRemainingSeconds = 0.0f;
        }

        FVector CamForward;
        FVector CamRight;
        GetCameraPlaneAxes(CamForward, CamRight);
        PaperVisual->UpdateMovementFlip(Velocity, CamRight);

        FName DesiredPaperState = NAME_None;
        if (ActionStateComponent)
        {
            const bool bEscortingInjured = ActionStateComponent->HasCapability(DCGameplayTags::State_EscortingInjured);
            const bool bHasTool = ActionStateComponent->HasCapability(DCGameplayTags::Capability_ExtinguishFire);
            const bool bSpraying = ActionStateComponent->HasCapability(DCGameplayTags::State_SprayingFire);
            const bool bAllowEscortTool = Config && (Config->EscortToolPolicy == EDCEscortToolPolicy::ShowToolAndAllowSpray);

            if (bSpraying && (!bEscorting || bAllowEscortTool))
            {
                DesiredPaperState = FName(TEXT("Spraying"));
            }
            else if (bEscorting)
            {
                if (bHasTool && bAllowEscortTool)
                {
                    DesiredPaperState = bEscortingInjured
                        ? FName(TEXT("CombinedEscortInjuredWithTool"))
                        : FName(TEXT("CombinedEscortUninjuredWithTool"));
                }
                else
                {
                    DesiredPaperState = bEscortingInjured
                        ? FName(TEXT("CombinedEscortInjured"))
                        : FName(TEXT("CombinedEscortUninjured"));
                }
            }
            else if (BlockedReactionRemainingSeconds > 0.0f && !bIsMovingHoriz)
            {
                DesiredPaperState = FName(TEXT("Scared"));
            }
            else if (bHasTool)
            {
                DesiredPaperState = FName(TEXT("HoldingTool"));
            }
        }
        else if (BlockedReactionRemainingSeconds > 0.0f && !bIsMovingHoriz)
        {
            DesiredPaperState = FName(TEXT("Scared"));
        }

        PaperVisual->SetPaperState(DesiredPaperState);
    }

    UpdateFallback3DState(Config);
}

void ADCPlayerCharacter::UpdateFocusedInteractable()
{
    if (!ActionStateComponent)
    {
        return;
    }

    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    if (!TakeSubsystem || !TakeSubsystem->IsActiveTake() || ActionStateComponent->IsPersonaPanelOpen())
    {
        ActionStateComponent->SetFocusedInteractable(nullptr);
        return;
    }

    ActionStateComponent->SetFocusedInteractable(FindNearestAvailableInteractable());
}

void ADCPlayerCharacter::UpdateInteractPromptWidget(const UDCTakeSubsystem* TakeSubsystem)
{
    if (!InteractPromptWidgetComponent)
    {
        return;
    }

    const bool bPanelOpen = ActionStateComponent && ActionStateComponent->IsPersonaPanelOpen();
    ADCInteractableBase* Focused = ActionStateComponent ? ActionStateComponent->GetFocusedInteractable() : nullptr;
    const bool bShouldShowPrompt =
        TakeSubsystem &&
        TakeSubsystem->IsActiveTake() &&
        !bPanelOpen &&
        IsValid(Focused) &&
        Focused->IsActionAvailable(this);

    if (!bShouldShowPrompt)
    {
        if (InteractPromptWidgetComponent->IsVisible())
        {
            InteractPromptWidgetComponent->SetVisibility(false);
        }
        return;
    }

    InteractPromptWidgetComponent->SetWorldLocation(Focused->GetPromptWorldLocation());
    if (!InteractPromptWidget && InteractPromptWidgetComponent->GetUserWidgetObject())
    {
        InteractPromptWidget = InteractPromptWidgetComponent->GetUserWidgetObject();
    }

    if (UDCInteractPromptWidget* PromptWidget = Cast<UDCInteractPromptWidget>(InteractPromptWidget))
    {
        const bool bIsHoldMode = (Focused->GetInteractionInputMode() == EDCInteractionInputMode::HoldToComplete);
        PromptWidget->SetPromptState(
            Focused->GetPromptText(this),
            Focused->GetHoldProgressNormalized(),
            bIsHoldMode,
            Focused->IsHoldActive());
    }

    if (!InteractPromptWidgetComponent->IsVisible())
    {
        InteractPromptWidgetComponent->SetVisibility(true);
    }
}

void ADCPlayerCharacter::Interact()
{
    StartInteractInput();
}

void ADCPlayerCharacter::StartInteractInput()
{
    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    if (!TakeSubsystem || !TakeSubsystem->IsActiveTake())
    {
        return;
    }

    if (ActionStateComponent && ActionStateComponent->IsPersonaPanelOpen())
    {
        return;
    }

    if (ADCInteractableBase* Target = FindNearestAvailableInteractable())
    {
        const EDCInteractionResult PreEval = Target->EvaluateInteraction(this);
        const UDCLevelConfigData* Config = TakeSubsystem->GetActiveConfig();

        if (Target->GetInteractionInputMode() == EDCInteractionInputMode::HoldToComplete)
        {
            if (ActionStateComponent && ActionStateComponent->GetActiveHoldInteractable() != Target)
            {
                CancelActiveHoldInteraction();
            }

            if (Target->BeginHoldInteraction(this) && ActionStateComponent)
            {
                ActionStateComponent->SetActiveHoldInteractable(Target);
            }
        }
        else
        {
            CancelActiveHoldInteraction();
            Target->TryInteract(this);
        }

        if (PreEval == EDCInteractionResult::Denied_BlockedByPersona)
        {
            BlockedReactionRemainingSeconds = 1.0f;
            UpdateMovementAndVisualState(Config);
        }

        UpdateFocusedInteractable();
        UpdateInteractPromptWidget(TakeSubsystem);
    }
    else if (ActionStateComponent)
    {
        ActionStateComponent->SetLastDebugReason(TEXT("No available target in range"));
    }
}

void ADCPlayerCharacter::StopInteractInput()
{
    CancelActiveHoldInteraction();
}

void ADCPlayerCharacter::TogglePersona()
{
    if (!ActionStateComponent)
    {
        return;
    }

    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    if (!TakeSubsystem || TakeSubsystem->IsTakeResult())
    {
        return;
    }

    if (TakeSubsystem->IsPreTake())
    {
        ActionStateComponent->SetPersonaPanelOpen(true);
        UpdateInputModeForState();
        return;
    }

    if (TakeSubsystem->IsActiveTake())
    {
        const UDCLevelConfigData* Config = TakeSubsystem->GetActiveConfig();
        const EDCActiveTakePersonaViewPolicy Policy = Config
            ? Config->ActiveTakePersonaViewPolicy
            : EDCActiveTakePersonaViewPolicy::AllowTabReadOnlyPeek;

        if (Policy != EDCActiveTakePersonaViewPolicy::AllowTabReadOnlyPeek)
        {
            ActionStateComponent->SetPersonaPanelOpen(false);
            if (Config && !Config->PersonaViewDisallowedHintText.IsEmpty())
            {
                ActionStateComponent->RequestHudHint(Config->PersonaViewDisallowedHintText, -1.0f);
            }
            UpdateInputModeForState();
            return;
        }

        if (!ActionStateComponent->IsPersonaPanelOpen())
        {
            CancelActiveHoldInteraction();
        }
    }

    ActionStateComponent->SetPersonaPanelOpen(!ActionStateComponent->IsPersonaPanelOpen());
}

void ADCPlayerCharacter::StartTakeAction()
{
    UWorld* World = GetWorld();
    UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    if (!TakeSubsystem || !TakeSubsystem->IsPreTake())
    {
        return;
    }

    if (ActionStateComponent)
    {
        ActionStateComponent->SetPersonaPanelOpen(false);
    }

    TakeSubsystem->StartTake();
}

void ADCPlayerCharacter::RestartTakeAction()
{
    UWorld* World = GetWorld();
    if (UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr)
    {
        TakeSubsystem->RestartTake();
    }
}

void ADCPlayerCharacter::HandleTakePhaseChanged(EDCTakePhase NewPhase)
{
    if (NewPhase == EDCTakePhase::ActiveTake || NewPhase == EDCTakePhase::TakeResult)
    {
        CancelActiveHoldInteraction();

        if (ActionStateComponent)
        {
            ActionStateComponent->SetPersonaPanelOpen(false);
            ActionStateComponent->SetFocusedInteractable(nullptr);
        }

        if (InteractPromptWidgetComponent)
        {
            InteractPromptWidgetComponent->SetVisibility(false);
        }
    }
    else if (NewPhase == EDCTakePhase::PreTake)
    {
        CancelActiveHoldInteraction();
        if (ActionStateComponent)
        {
            ActionStateComponent->SetPersonaPanelOpen(true);
            ActionStateComponent->SetFocusedInteractable(nullptr);
        }
        if (InteractPromptWidgetComponent)
        {
            InteractPromptWidgetComponent->SetVisibility(false);
        }
    }

    if (NewPhase == EDCTakePhase::TakeResult)
    {
        ThoughtRemainingSeconds = 0.0f;
        BlockedReactionRemainingSeconds = 0.0f;
        FootstepCooldownSeconds = 0.0f;
        WaterSpraySoundCooldownSeconds = 0.0f;
        bWasSprayingForAudio = false;
        if (ThoughtWidgetComponent)
        {
            ThoughtWidgetComponent->SetVisibility(false);
        }
    }
    else if (NewPhase == EDCTakePhase::PreTake)
    {
        BlockedReactionRemainingSeconds = 0.0f;
        FootstepCooldownSeconds = 0.0f;
        WaterSpraySoundCooldownSeconds = 0.0f;
        bWasSprayingForAudio = false;
    }

    UpdateInputModeForState();
}

void ADCPlayerCharacter::HandleTakeReset()
{
    CancelActiveHoldInteraction();

    ThoughtRemainingSeconds = 0.0f;
    BlockedReactionRemainingSeconds = 0.0f;
    FootstepCooldownSeconds = 0.0f;
    WaterSpraySoundCooldownSeconds = 0.0f;
    bWasSprayingForAudio = false;
    if (ThoughtWidgetComponent)
    {
        ThoughtWidgetComponent->SetVisibility(false);
    }
    if (InteractPromptWidgetComponent)
    {
        InteractPromptWidgetComponent->SetVisibility(false);
    }

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->StopMovementImmediately();
    }

    if (PaperVisual)
    {
        PaperVisual->SetFacingLeft(false);
        PaperVisual->SetPaperState(NAME_None);
    }

    RefreshConfigAndCamera();
    SpawnStageWidgets();

    if (ActionStateComponent)
    {
        ActionStateComponent->SetPersonaPanelOpen(true);
    }

    UpdateInputModeForState();
}

void ADCPlayerCharacter::HandleThoughtRequested(FText Text, float DurationSeconds)
{
    ThoughtRemainingSeconds = (DurationSeconds > 0.0f) ? DurationSeconds : 4.0f;
    if (const UWorld* World = GetWorld())
    {
        if (World->IsGameWorld())
        {
            const UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>();
            const UDCLevelConfigData* Config = TakeSubsystem ? TakeSubsystem->GetActiveConfig() : nullptr;
            if (Config && Config->ThoughtBubbleSound)
            {
                UGameplayStatics::PlaySound2D(this, Config->ThoughtBubbleSound);
            }
        }
    }

    if (!ThoughtWidgetComponent)
    {
        return;
    }

    ThoughtWidgetComponent->InitWidget();
    if (!ThoughtWidget && ThoughtWidgetComponent->GetUserWidgetObject())
    {
        ThoughtWidget = ThoughtWidgetComponent->GetUserWidgetObject();
    }

    if (UDCThoughtBubbleWidget* Bubble = Cast<UDCThoughtBubbleWidget>(ThoughtWidget))
    {
        Bubble->SetThoughtText(Text);
    }

    ThoughtWidgetComponent->SetVisibility(true);
}

void ADCPlayerCharacter::HandleThoughtCleared()
{
    ThoughtRemainingSeconds = 0.0f;
    if (ThoughtWidgetComponent)
    {
        ThoughtWidgetComponent->SetVisibility(false);
    }
}

void ADCPlayerCharacter::HandlePersonaPanelToggled(bool bIsOpen)
{
    if (bIsOpen)
    {
        CancelActiveHoldInteraction();
        if (InteractPromptWidgetComponent)
        {
            InteractPromptWidgetComponent->SetVisibility(false);
        }
    }

    UpdateInputModeForState();
}

void ADCPlayerCharacter::UpdateInputModeForState()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return;
    }

    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    const bool bTakeResult = TakeSubsystem && TakeSubsystem->IsTakeResult();
    const bool bPanelOpen = ActionStateComponent && ActionStateComponent->IsPersonaPanelOpen();
    const bool bNeedCursor = bTakeResult || bPanelOpen;

    PC->bShowMouseCursor = bNeedCursor;
    if (bNeedCursor)
    {
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(Mode);
    }
    else
    {
        PC->SetInputMode(FInputModeGameOnly());
        if (FSlateApplication::IsInitialized())
        {
            FSlateApplication::Get().SetAllUserFocusToGameViewport();
        }
    }
}
