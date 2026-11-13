#include "DCPlayer.h"
#include "DCPersona.h"
#include "DCScene.h"
#include "DCUI.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "UObject/ConstructorHelpers.h"

ADCPlayer::ADCPlayer()
{
    PrimaryActorTick.bCanEverTick = true;
    Persona = CreateDefaultSubobject<UDCPersona>(TEXT("Persona"));
    auto* Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    Arm->SetupAttachment(RootComponent); Arm->TargetArmLength = 1350;
    Arm->SetRelativeRotation(FRotator(-58,-35,0)); Arm->bDoCollisionTest = false;
    auto* Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Arm); Camera->FieldOfView = 65;
    auto* Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FirefighterBody"));
    Body->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Body->SetStaticMesh(Sphere.Object); Body->SetRelativeScale3D(FVector(.65,.65,1.35));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto* Helmet = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Helmet"));
    Helmet->SetupAttachment(RootComponent); Helmet->SetStaticMesh(Sphere.Object);
    Helmet->SetRelativeScale3D(FVector(.75,.75,.3)); Helmet->SetRelativeLocation(FVector(0,0,65));
    Helmet->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ThoughtComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("ThoughtBubble"));
    ThoughtComponent->SetupAttachment(RootComponent); ThoughtComponent->SetRelativeLocation(FVector(0,0,160));
    ThoughtComponent->SetWidgetSpace(EWidgetSpace::Screen); ThoughtComponent->SetDrawSize(FVector2D(410,110));
    ThoughtComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetCharacterMovement()->MaxWalkSpeed = 460;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    bUseControllerRotationYaw = false;
}
void ADCPlayer::BeginPlay()
{
    Super::BeginPlay();
    TActorIterator<ADCDirector> I(GetWorld());
    if (I) Director = *I;
    if (!Director) { UE_LOG(LogTemp, Error, TEXT("DC Missing director in map")); return; }
    if (auto* PC = Cast<APlayerController>(GetController()))
    {
        Overlay = CreateWidget<UDCOverlay>(PC); Overlay->Player = this; Overlay->AddToViewport();
        ThoughtWidget = CreateWidget<UDCThought>(PC); ThoughtComponent->SetWidget(ThoughtWidget);
        ThoughtComponent->SetVisibility(false);
        PC->SetInputMode(FInputModeGameOnly()); PC->bShowMouseCursor = false;
    }
    Persona->OnPersonaChanged.AddUniqueDynamic(this, &ADCPlayer::HandlePersonaChanged);
    if (auto* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Player.M_Player")))
        for (auto* C : TInlineComponentArray<UStaticMeshComponent*>(this)) C->SetMaterial(0, M);
    if (Director->bAwaitingTakeStart) TogglePersona();
}
void ADCPlayer::SetupPlayerInputComponent(UInputComponent* I)
{
    Super::SetupPlayerInputComponent(I);
    I->BindAxis("MoveForward", this, &ADCPlayer::MoveForward);
    I->BindAxis("MoveRight", this, &ADCPlayer::MoveRight);
    I->BindKey(EKeys::E, IE_Pressed, this, &ADCPlayer::Interact);
    I->BindKey(EKeys::Tab, IE_Pressed, this, &ADCPlayer::TogglePersona);
    I->BindKey(EKeys::R, IE_Pressed, this, &ADCPlayer::RestartTake);
    I->BindKey(EKeys::F6, IE_Pressed, this, &ADCPlayer::DebugResetPersona);
    I->BindKey(EKeys::F7, IE_Pressed, this, &ADCPlayer::DebugToggleReason);
    I->BindKey(EKeys::F8, IE_Pressed, this, &ADCPlayer::DebugFinishTake);
}
void ADCPlayer::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Overlay) Overlay->RemoveFromParent();
    ThoughtComponent->SetWidget(nullptr);
    Overlay = nullptr; ThoughtWidget = nullptr;
    Super::EndPlay(Reason);
}
bool ADCPlayer::CanMove() const { return Director && !Director->bFinished && !bPersonaOpen; }
void ADCPlayer::MoveForward(float V) { if (CanMove()) AddMovementInput(FRotator(0,-35,0).Vector(), V); }
void ADCPlayer::MoveRight(float V) { if (CanMove()) AddMovementInput(FRotationMatrix(FRotator(0,-35,0)).GetUnitAxis(EAxis::Y), V); }
void ADCPlayer::Interact()
{
    if (!Director) return;
    if (auto* A = Director->NearestInteraction(this)) A->TryInteract(this);
    else Director->LastBlockReason = TEXT("No available target in range");
}
void ADCPlayer::TogglePersona()
{
    if (!Director || Director->bFinished) return;
    if (!bPersonaOpen && !Director->bAllowEditingDuringTake && !Director->bAwaitingTakeStart)
    { ShowThought(TEXT("TEST: editing during this take is disabled.")); return; }
    bPersonaOpen = !bPersonaOpen;
    if (!bPersonaOpen && Director->bAwaitingTakeStart) Director->bAwaitingTakeStart = false;
    if (auto* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = bPersonaOpen;
        if (bPersonaOpen) PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
        else PC->SetInputMode(FInputModeGameOnly());
    }
    if (Overlay) Overlay->Refresh();
}
void ADCPlayer::Tick(float Dt)
{
    Super::Tick(Dt);
    ThoughtRemaining = FMath::Max(0.f, ThoughtRemaining-Dt);
    ThoughtComponent->SetVisibility(ThoughtRemaining > 0.f && Director && !Director->bFinished);
}
void ADCPlayer::ShowThought(const FString& S)
{
    ThoughtText = S; ThoughtRemaining = 4.f;
    // UWidgetComponent can initialize/rebuild its widget lazily on its first visible frame.
    ThoughtComponent->InitWidget();
    if (!ThoughtWidget && ThoughtComponent->GetUserWidgetObject()) ThoughtWidget = Cast<UDCThought>(ThoughtComponent->GetUserWidgetObject());
    if (ThoughtWidget) ThoughtWidget->SetThought(S);
    ThoughtComponent->SetVisibility(true);
}
void ADCPlayer::RestartTake()
{
    if (!Director) return;
    Director->ResetTake(); Persona->ResetPersona(); bInside = false;
    bPersonaOpen = false; ThoughtText.Reset(); ThoughtRemaining = 0;
    GetCharacterMovement()->StopMovementImmediately();
    SetActorLocation(FVector(-450,0,100));
    if (auto* PC = Cast<APlayerController>(GetController()))
    { PC->bShowMouseCursor = false; PC->SetInputMode(FInputModeGameOnly()); }
    if (Overlay) Overlay->Refresh();
    if (Director->bAwaitingTakeStart) TogglePersona();
}
void ADCPlayer::DebugResetPersona() { if (Director && Director->bEnableDebugControls) Persona->ResetPersona(); }
void ADCPlayer::DebugToggleReason() { if (Director && Director->bEnableDebugControls) Director->bShowMemoryReason = !Director->bShowMemoryReason; }
void ADCPlayer::DebugFinishTake() { if (Director && Director->bEnableDebugControls) Director->FinishTake(); }
void ADCPlayer::HandlePersonaChanged()
{
    ThoughtText.Reset(); ThoughtRemaining = 0.f;
    ThoughtComponent->SetVisibility(false);
    if (Director) Director->LastBlockReason = TEXT("Persona changed; check again at interaction");
}
