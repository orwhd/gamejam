#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "DCPlayer.h"
#include "DCPersona.h"
#include "DCScene.h"
#include "DCUI.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "PlayInEditorDataTypes.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDCAcceptance, "DirectorCut.PIE.Acceptance", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

DEFINE_LATENT_AUTOMATION_COMMAND(FDCStartFloatingPIE);
bool FDCStartFloatingPIE::Update()
{
    FRequestPlaySessionParams Params;
    Params.SessionDestination = EPlaySessionDestinationType::InProcess;
    Params.WorldType = EPlaySessionWorldType::PlayInEditor;
    // No DestinationSlateViewport: use an independent PIE window. Embedded PIE on this
    // engine/macOS combination intermittently asserts GameViewport.IsUnique on shutdown.
    GEditor->RequestPlaySession(Params);
    return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FDCCheck, FAutomationTestBase*, Test, int32, Stage);
bool FDCCheck::Update()
{
    UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
    if (!Test->TestNotNull(TEXT("PIE world actually exists"), World)) return true;
    TActorIterator<ADCPlayer> PlayerIt(World);
    TActorIterator<ADCDirector> DirectorIt(World);
    ADCPlayer* P = PlayerIt ? *PlayerIt : nullptr;
    ADCDirector* D = DirectorIt ? *DirectorIt : nullptr;
    if (!Test->TestNotNull(TEXT("Possessed firefighter spawned"), P) || !Test->TestNotNull(TEXT("Map director spawned"), D)) return true;
    auto Action = [D](EDCAction A) -> ADCInteraction* { for (const auto& I : D->Interactions) if (I->Action == A) return I.Get(); return nullptr; };
    auto* Enter = Action(EDCAction::Enter);
    if (!Test->TestNotNull(TEXT("Actual entrance actor"), Enter)) return true;
    auto Reset = [P]() { P->RestartTake(); };
    if (Stage == 0)
    {
        Reset();
        Test->TestNotNull(TEXT("Player controller exists"), P->GetController());
        Test->TestTrue(TEXT("UMG overlay attached to viewport"), P->Overlay && P->Overlay->IsInViewport());
        Test->TestTrue(TEXT("UMG root generated before Slate rebuild"), P->Overlay && P->Overlay->WidgetTree->RootWidget != nullptr);
        Test->TestTrue(TEXT("Original memory has psychological effect"), P->Persona->HasEffect(TEXT("FearDisobeyCaptain")));
        Test->TestFalse(TEXT("Fixed firefighter identity cannot be deleted"), P->Persona->DeleteBlock(TEXT("Identity")));
        Test->TestFalse(TEXT("Unknown block cannot be deleted"), P->Persona->DeleteBlock(TEXT("Missing")));
        Test->TestFalse(TEXT("Direct call checks distance"), Enter->TryInteract(P));
        P->SetActorLocation(FVector(170,0,100));
        Test->TestFalse(TEXT("Original Persona rejects entrance"), Enter->TryInteract(P));
        Test->TestFalse(TEXT("Denied entry leaves player outside"), P->bInside);
        Test->TestEqual(TEXT("Short thought is actual refusal reason"), P->ThoughtText, FString(TEXT("I must obey my captain's orders.")));
        Test->TestTrue(TEXT("Thought has visible lifetime"), P->ThoughtRemaining > 0);
        Test->TestTrue(TEXT("Debug identifies Persona restriction"), D->LastBlockReason.Contains(TEXT("FearDisobeyCaptain")));
        D->bShowMemoryReason = true; Enter->TryInteract(P);
        Test->TestTrue(TEXT("Configurable expanded thought"), P->ThoughtText.Contains(TEXT("Because of what happened last time.")));
        D->bShowMemoryReason = false; Enter->TryInteract(P);
    }
    if (Stage == 1)
    {
        TArray<UWidget*> ThoughtWidgets;
        P->ThoughtWidget->WidgetTree->GetAllWidgets(ThoughtWidgets);
        bool bActualThoughtText = false;
        for (auto* W : ThoughtWidgets) if (auto* T = Cast<UTextBlock>(W)) bActualThoughtText |= T->GetText().ToString().Contains(TEXT("I must obey"));
        Test->TestTrue(TEXT("Thought text survives lazy widget rebuild"), bActualThoughtText);
        P->TogglePersona();
        Test->TestTrue(TEXT("Persona panel opened"), P->bPersonaOpen);
        Test->TestFalse(TEXT("Cannot act through open panel"), Enter->TryInteract(P));
    }
    if (Stage == 2)
    {
        TArray<UWidget*> Widgets; P->Overlay->WidgetTree->GetAllWidgets(Widgets);
        UDCDeleteButton* Delete = nullptr;
        for (auto* W : Widgets) if (auto* B = Cast<UDCDeleteButton>(W)) { Delete = B; break; }
        if (Test->TestNotNull(TEXT("Real UMG deletion widget created"), Delete))
        {
            auto* Button = Cast<UButton>(Delete->WidgetTree->RootWidget);
            if (Test->TestNotNull(TEXT("Deletion widget contains clickable button"), Button)) Button->OnClicked.Broadcast();
        }
        Test->TestFalse(TEXT("UMG callback deletes memory"), P->Persona->QueryBlockActive(TEXT("PunishmentMemory")));
        Test->TestFalse(TEXT("Deleted memory removes effect"), P->Persona->HasEffect(TEXT("FearDisobeyCaptain")));
        Test->TestTrue(TEXT("Deleting memory clears stale refusal bubble"), P->ThoughtRemaining == 0.f && P->ThoughtText.IsEmpty());
        Test->TestFalse(TEXT("Repeat deletion is rejected"), P->Persona->DeleteBlock(TEXT("PunishmentMemory")));
        Test->TestTrue(TEXT("Identity survives deletion"), P->Persona->QueryBlockActive(TEXT("Identity")));
    }
    if (Stage == 3)
    {
        P->TogglePersona();
        Test->TestTrue(TEXT("Entry succeeds after deletion"), Enter->TryInteract(P));
        Test->TestTrue(TEXT("Same firefighter is now inside"), P->bInside);
        auto* Rescue = Action(EDCAction::Rescue);
        P->SetActorLocation(Rescue->GetActorLocation() + FVector(0,0,100));
        Test->TestTrue(TEXT("Rescue interaction executes"), Rescue->TryInteract(P));
        Test->TestTrue(TEXT("World records rescue"), D->bRescued);
        Test->TestFalse(TEXT("Rescued victim visual removed"), D->VictimVisual->IsVisible());
        Test->TestFalse(TEXT("Rescue cannot execute twice"), Rescue->TryInteract(P));
        auto* Extinguish = Action(EDCAction::Extinguish);
        P->SetActorLocation(Extinguish->GetActorLocation() + FVector(0,0,100));
        Test->TestTrue(TEXT("Extinguish interaction executes"), Extinguish->TryInteract(P));
        Test->TestTrue(TEXT("Both objectives complete take"), D->bFinished);
        Test->TestEqual(TEXT("Rescue and extinguish yield two stars"), D->EvaluateStars(), 2);
        Test->TestFalse(TEXT("Fire visual removed"), D->FireVisual->IsVisible());
        Test->TestFalse(TEXT("No actions after result"), Extinguish->TryInteract(P));
    }
    if (Stage == 4)
    {
        Reset();
        D->Elapsed = D->FireExpansionSeconds + 1; D->AdvanceScript();
        const auto OriginalTrace = D->ScriptTrace;
        const auto OriginalCaptainPosition = D->CaptainVisual->GetRelativeLocation();
        Reset(); P->Persona->DeleteBlock(TEXT("PunishmentMemory"));
        D->Elapsed = D->FireExpansionSeconds + 1; D->AdvanceScript();
        Test->TestTrue(TEXT("Deleting memory preserves exact NPC script"), OriginalTrace == D->ScriptTrace);
        Test->TestTrue(TEXT("Deleting memory preserves captain's scripted move"), OriginalCaptainPosition.Equals(D->CaptainVisual->GetRelativeLocation()));
        Test->TestTrue(TEXT("Captain's order remains active"), D->bCaptainOrderActive);
        Test->TestTrue(TEXT("Active fire expands at test event"), D->bFireExpanded);
        Reset(); D->Elapsed = D->TakeDurationSeconds - .01f; D->Tick(.02f);
        Test->TestTrue(TEXT("Accelerated deadline triggers original failed ending"), D->bFinished);
        Test->TestEqual(TEXT("Original failure yields zero stars"), D->EvaluateStars(), 0);
        Test->TestTrue(TEXT("Failure records uncontrolled fire"), D->Fire == EDCFire::OutOfControl);
        Reset(); P->Persona->DeleteBlock(TEXT("PunishmentMemory")); P->SetActorLocation(FVector(170,0,100)); Enter->TryInteract(P);
        auto* Extinguish = Action(EDCAction::Extinguish);
        P->SetActorLocation(Extinguish->GetActorLocation() + FVector(0,0,100)); Extinguish->TryInteract(P);
        Test->TestFalse(TEXT("Extinguish alone leaves time to rescue"), D->bFinished);
        D->Elapsed = D->TakeDurationSeconds - .01f; D->Tick(.02f);
        Test->TestEqual(TEXT("Extinguish without rescue yields one star"), D->EvaluateStars(), 1);
        Reset();
        Test->TestTrue(TEXT("Reset restores initial memory"), P->Persona->QueryBlockActive(TEXT("PunishmentMemory")));
        Test->TestFalse(TEXT("Reset restores trapped victim"), D->bRescued);
        Test->TestTrue(TEXT("Reset restores visible victim"), D->VictimVisual->IsVisible());
        Test->TestTrue(TEXT("Reset restores active fire"), D->Fire == EDCFire::Active && D->FireVisual->IsVisible());
        Test->TestEqual(TEXT("Reset restores one initial NPC event"), D->ScriptTrace.Num(), 1);
        Test->TestTrue(TEXT("Reset clears thoughts"), P->ThoughtRemaining <= 0);
        Test->TestFalse(TEXT("Reset puts player outside"), P->bInside);
        Test->TestTrue(TEXT("Reset puts player at start"), P->GetActorLocation().Equals(FVector(-450,0,100),1));
        P->SetActorLocation(FVector(170,0,100));
        Test->TestFalse(TEXT("Reset restores blocked entrance"), Enter->TryInteract(P));
        D->bAllowEditingDuringTake = false; Reset();
        Test->TestTrue(TEXT("Before-take editing opens panel"), P->bPersonaOpen && D->bAwaitingTakeStart);
        D->Tick(1); Test->TestEqual(TEXT("Before-take editing does not advance scene"), D->Elapsed,0.f);
        P->TogglePersona();
        Test->TestFalse(TEXT("Closing pre-take panel starts scene"), D->bAwaitingTakeStart);
        P->TogglePersona();
        Test->TestFalse(TEXT("Before-take-only config prevents reopening"), P->bPersonaOpen);
        D->bAllowEditingDuringTake = true; Reset();
    }
    auto Key = [P](FKey K, EInputEvent Event)
    {
        if (auto* PC = Cast<APlayerController>(P->GetController())) PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,Event,Event == IE_Released ? 0.f : 1.f));
    };
    if (Stage == 5) { Reset(); Key(EKeys::W,IE_Pressed); }
    if (Stage == 6)
    {
        Key(EKeys::W,IE_Released);
        Test->TestTrue(TEXT("Injected W passes through configured movement input and moves firefighter"), FVector::Dist2D(P->GetActorLocation(),FVector(-450,0,100)) > 100);
        Reset(); P->SetActorLocation(FVector(170,0,100)); Key(EKeys::E,IE_Pressed);
    }
    if (Stage == 7)
    {
        Key(EKeys::E,IE_Released);
        Test->TestTrue(TEXT("Injected E reaches interaction binding and displays refusal"), P->ThoughtText.Contains(TEXT("I must obey")));
        Key(EKeys::Tab,IE_Pressed);
    }
    if (Stage == 8)
    {
        Key(EKeys::Tab,IE_Released);
        Test->TestTrue(TEXT("Injected Tab opens Persona via input binding"), P->bPersonaOpen);
        Reset();
    }
    if (FParse::Param(FCommandLine::Get(),TEXT("DCCaptureEvidence")) && Stage < 4)
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation")/FString::Printf(TEXT("PIE_Stage%d.png"),Stage),true,false);
    return true;
}
bool FDCAcceptance::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/Maps/Firefighter")));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
    ADD_LATENT_AUTOMATION_COMMAND(FDCStartFloatingPIE());
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.f));
    for (int32 Stage=0; Stage<9; ++Stage)
    {
        ADD_LATENT_AUTOMATION_COMMAND(FDCCheck(this,Stage));
        ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
    }
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
