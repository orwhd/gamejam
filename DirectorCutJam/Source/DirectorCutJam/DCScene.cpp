#include "DCScene.h"
#include "DCPlayer.h"
#include "DCPersona.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerController.h"

ADCInteraction::ADCInteraction()
{
    Shape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
    RootComponent = Shape;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Shape->SetStaticMesh(Cube.Object);
    Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(RootComponent);
    Label->SetRelativeLocation(FVector(0, 0, 130));
    Label->SetRelativeRotation(FRotator(0, 150, 0));
    Label->SetWorldSize(23);
    Label->SetHorizontalAlignment(EHTA_Center);
}

bool ADCInteraction::TryInteract(ADCPlayer* P)
{
    if (!IsValid(P) || !IsValid(P->Director)) return false;
    ADCDirector* D = P->Director.Get();
    // This is the authority for every caller, including Blueprint and UI.
    if (D->bFinished) { D->LastBlockReason = TEXT("Take finished"); return false; }
    if (P->bPersonaOpen) { D->LastBlockReason = TEXT("Close Persona before interacting"); return false; }
    if (FVector::Dist2D(P->GetActorLocation(), GetActorLocation()) > Range)
    { D->LastBlockReason = TEXT("Out of interaction range"); return false; }
    if (!D->IsActionAvailable(Action) || (Action == EDCAction::Enter ? P->bInside : !P->bInside))
    { D->LastBlockReason = TEXT("Action unavailable in current scene state"); return false; }
    if (Action == EDCAction::Enter && D->bCaptainOrderActive && P->Persona->HasEffect(BlockingEffect))
    {
        D->LastBlockReason = FString::Printf(TEXT("Persona: %s / captain order"), *BlockingEffect.ToString());
        P->ShowThought(D->bShowMemoryReason
            ? TEXT("I must obey my captain's orders.\nBecause of what happened last time.")
            : TEXT("I must obey my captain's orders."));
        UE_LOG(LogTemp, Display, TEXT("DC Interaction blocked: %s"), *D->LastBlockReason);
        return false;
    }
    D->LastBlockReason = TEXT("Allowed");
    switch (Action)
    {
    case EDCAction::Enter:
        P->SetActorLocation(FVector(680, 0, 100)); P->bInside = true;
        P->ShowThought(TEXT("I can choose to go in.")); break;
    case EDCAction::Exit:
        P->SetActorLocation(FVector(150, 0, 100)); P->bInside = false; break;
    case EDCAction::Rescue:
        D->bRescued = true; D->VictimVisual->SetVisibility(false);
        Shape->SetVisibility(false); Label->SetVisibility(false);
        P->ShowThought(TEXT("They are safe now.")); break;
    case EDCAction::Extinguish:
        D->Fire = EDCFire::Extinguished; D->FireVisual->SetVisibility(false);
        Shape->SetVisibility(false); Label->SetVisibility(false);
        P->ShowThought(TEXT("The fire is out.")); break;
    }
    UE_LOG(LogTemp, Display, TEXT("DC Interaction executed: %d"), int32(Action));
    if (D->bRescued && D->Fire == EDCFire::Extinguished) D->FinishTake();
    return true;
}

ADCDirector::ADCDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto Box = [this](const TCHAR* Name, FVector Position, FVector Size, bool Solid = true)
    {
        auto* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        C->SetupAttachment(RootComponent); C->SetStaticMesh(Cube.Object);
        C->SetRelativeLocation(Position); C->SetRelativeScale3D(Size / 100.f);
        C->SetCollisionEnabled(Solid ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
        C->SetCollisionResponseToAllChannels(ECR_Block);
        return C;
    };
    Box(TEXT("Ground"), FVector(450,0,-25), FVector(3300,2400,50));
    Box(TEXT("InteriorFloor"), FVector(1050,0,1), FVector(1200,1200,2), false);
    // Low walls expose the graybox to the camera. The front wall cannot be walked through;
    // crossing it always uses the Persona-gated interaction (it is not a locked-door rule).
    Box(TEXT("FrontWall"), FVector(450,0,90), FVector(40,1240,180));
    Box(TEXT("BackWall"), FVector(1670,0,90), FVector(40,1240,180));
    Box(TEXT("LeftWall"), FVector(1050,-620,90), FVector(1280,40,180));
    Box(TEXT("RightWall"), FVector(1050,620,90), FVector(1280,40,180));
    Box(TEXT("EntryStripe"), FVector(420,0,100), FVector(55,220,200), false);
    FireVisual = Box(TEXT("Fire"), FVector(1300,-300,85), FVector(160,160,170), false);
    VictimVisual = Box(TEXT("Victim"), FVector(1350,300,55), FVector(65,65,110), false);
    CaptainVisual = Box(TEXT("Captain"), FVector(-100,-330,85), FVector(70,70,170), false);
    const FVector FencePositions[] = {FVector(-1200,0,100), FVector(2100,0,100), FVector(450,-1200,100), FVector(450,1200,100)};
    const FVector FenceSizes[] = {FVector(30,2400,200), FVector(30,2400,200), FVector(3300,30,200), FVector(3300,30,200)};
    for (int32 I=0; I<4; ++I) Box(*FString::Printf(TEXT("Boundary%d"), I), FencePositions[I], FenceSizes[I]);
    auto Label = [this](const TCHAR* Name, const TCHAR* Value, FVector Position, float Size)
    {
        auto* T = CreateDefaultSubobject<UTextRenderComponent>(Name);
        T->SetupAttachment(RootComponent); T->SetRelativeLocation(Position);
        T->SetRelativeRotation(FRotator(0,150,0)); T->SetWorldSize(Size);
        T->SetText(FText::FromString(Value)); T->SetHorizontalAlignment(EHTA_Center);
    };
    Label(TEXT("BuildingLabel"), TEXT("FIRE SCENE / TEST SET"), FVector(1100,0,210), 38);
    Label(TEXT("CaptainLabel"), TEXT("CAPTAIN"), FVector(-100,-330,220), 26);
}

void ADCDirector::BeginPlay()
{
    Super::BeginPlay();
    const EDCAction Actions[] = {EDCAction::Enter, EDCAction::Exit, EDCAction::Rescue, EDCAction::Extinguish};
    const FVector Positions[] = {FVector(260,0,5), FVector(560,0,5), FVector(1350,300,5), FVector(1300,-300,5)};
    const TCHAR* Captions[] = {TEXT("ENTER"), TEXT("EXIT"), TEXT("RESCUE"), TEXT("EXTINGUISH")};
    for (int32 I=0; I<4; ++I)
    {
        auto* A = GetWorld()->SpawnActor<ADCInteraction>(Positions[I], FRotator::ZeroRotator);
        A->Action = Actions[I]; A->Prompt = FText::FromString(Captions[I]);
        A->Shape->SetWorldScale3D(FVector(1.4,1.4,0.05));
        // Keep label size independent of the flat marker's scale.
        A->Label->SetAbsolute(false, false, true); A->Label->SetWorldScale3D(FVector(1));
        A->Label->SetWorldLocation(Positions[I] + FVector(0,0,120));
        A->Label->SetText(A->Prompt);
        A->Shape->SetMaterial(0, I == 3 ? FireVisual->GetMaterial(0) : VictimVisual->GetMaterial(0));
        Interactions.Add(A);
    }
    ResetTake();
}
void ADCDirector::Tick(float Dt)
{
    Super::Tick(Dt);
    if (bFinished || bAwaitingTakeStart) return;
    Elapsed += Dt;
    AdvanceScript();
    if (Elapsed >= TakeDurationSeconds) FinishTake();
}
void ADCDirector::AddScriptLine(int32 Step, const FString& Line)
{
    ScriptStep = Step; CaptainLine = Line;
    ScriptTrace.Add(FString::Printf(TEXT("%d:%s"), Step, *Line));
    UE_LOG(LogTemp, Display, TEXT("DC Script %d: %s"), Step, *Line);
}
void ADCDirector::AdvanceScript()
{
    // This sequence never reads Persona. World consequences are checked independently.
    if (ScriptStep < 2 && Elapsed >= CaptainFollowupSeconds)
    {
        CaptainVisual->SetRelativeLocation(FVector(-100,-100,85));
        AddScriptLine(2, TEXT("Hold position. Wait for backup."));
    }
    if (ScriptStep < 3 && Elapsed >= FMath::Max(CaptainFollowupSeconds, FireExpansionSeconds))
    {
        AddScriptLine(3, TEXT("Backup is still on its way."));
        if (Fire == EDCFire::Active)
        {
            bFireExpanded = true;
            FireVisual->SetRelativeScale3D(FVector(2.2,2.2,2.5));
        }
    }
}
void ADCDirector::ResetTake()
{
    Elapsed = 0; ScriptStep = 0; bRescued = false; bFinished = false; bFireExpanded = false;
    Fire = EDCFire::Active; bCaptainOrderActive = true; ScriptTrace.Reset(); LastBlockReason = TEXT("None");
    bAwaitingTakeStart = !bAllowEditingDuringTake;
    FireVisual->SetVisibility(true); FireVisual->SetRelativeScale3D(FVector(1.6,1.6,1.7));
    VictimVisual->SetVisibility(true); CaptainVisual->SetRelativeLocation(FVector(-100,-330,85));
    for (const auto& A : Interactions) { A->Shape->SetVisibility(true); A->Label->SetVisibility(true); }
    AddScriptLine(1, TEXT("Do not enter the building."));
}
void ADCDirector::FinishTake()
{
    if (bFinished) return;
    bFinished = true;
    if (auto* PC = GetWorld()->GetFirstPlayerController())
    { PC->bShowMouseCursor = true; PC->SetInputMode(FInputModeGameAndUI()); }
    if (Fire == EDCFire::Active) Fire = EDCFire::OutOfControl;
    UE_LOG(LogTemp, Display, TEXT("DC Result: fire=%d rescued=%d stars=%d"), int32(Fire), bRescued, EvaluateStars());
}
int32 ADCDirector::EvaluateStars() const
{
    if (Fire != EDCFire::Extinguished) return 0;
    if (!bRescued) return 1;
    return QualifiesForDesignedThirdStar() ? 3 : 2;
}
bool ADCDirector::IsActionAvailable(EDCAction A) const
{
    if (bFinished) return false;
    if (A == EDCAction::Rescue) return !bRescued;
    if (A == EDCAction::Extinguish) return Fire == EDCFire::Active;
    return true;
}
ADCInteraction* ADCDirector::NearestInteraction(const ADCPlayer* P) const
{
    if (!P) return nullptr;
    ADCInteraction* Best = nullptr; float Distance = MAX_flt;
    for (const auto& A : Interactions)
    {
        if (!IsValid(A) || !IsActionAvailable(A->Action)) continue;
        if (A->Action == EDCAction::Enter ? P->bInside : !P->bInside) continue;
        const float R = FVector::Dist2D(P->GetActorLocation(), A->GetActorLocation());
        if (R <= A->Range && R < Distance) { Best = A; Distance = R; }
    }
    return Best;
}
ADCGameMode::ADCGameMode() { DefaultPawnClass = ADCPlayer::StaticClass(); }
