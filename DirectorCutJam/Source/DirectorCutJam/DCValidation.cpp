#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Characters/DCPlayerCharacter.h"
#include "Core/DCGameplayTags.h"
#include "Core/DCTakeSubsystem.h"
#include "Core/DCTypes.h"
#include "Data/DCLevelConfigData.h"
#include "Interaction/DCActionStateComponent.h"
#include "Level1/DCFirefighterActors.h"
#include "Level1/DCStageDirector.h"
#include "Persona/DCPersonaComponent.h"
#include "Spatial/DCFixedStageCamera.h"
#include "Spatial/DCPaperVisualComponent.h"
#include "UI/DCHudOverlayWidget.h"
#include "UI/DCInteractPromptWidget.h"
#include "UI/DCPersonaPanelWidget.h"
#include "UI/DCStarPopupWidget.h"
#include "UI/DCTakeResultWidget.h"
#include "UI/DCThoughtBubbleWidget.h"
#include "UI/DCWidgetTheme.h"

#include "Blueprint/WidgetTree.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/Border.h"
#include "Components/BoxComponent.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Editor.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "PlayInEditorDataTypes.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "UnrealClient.h"
#include "Widgets/SWindow.h"

namespace DCValidationInternal
{
    struct FAcceptanceRuntimeState
    {
        float SavedF1RetainedSpraySeconds = 0.0f;
        FVector PorchCameraLocation = FVector::ZeroVector;
    };

    static FAcceptanceRuntimeState GAcceptanceState;

    struct FStageActors
    {
        UWorld* World = nullptr;
        ADCPlayerCharacter* Player = nullptr;
        ADCStageDirector* Director = nullptr;
        ADCFixedStageCamera* Camera = nullptr;
        UDCTakeSubsystem* TakeSub = nullptr;
        ADCCaptainActor* Captain = nullptr;
        ADCDoorwayInteractable* Doorway = nullptr;
        ADCExtinguisherPickup* Extinguisher = nullptr;
        ADCFireHazardActor* FireF1 = nullptr;
        ADCFireHazardActor* FireF2 = nullptr;
        ADCFireHazardActor* FireF3 = nullptr;
        TArray<ADCFireHazardActor*> AllFires;
        ADCCivilianActor* Civilian = nullptr;
        ADCSafeZoneVolume* SafeZone = nullptr;
        ADCMatchGirlActor* MatchGirl = nullptr;
        ADCAngelDogActor* AngelDog = nullptr;

        bool Gather(FAutomationTestBase* Test)
        {
            World = GEditor ? GEditor->PlayWorld : nullptr;
            if (!Test->TestNotNull(TEXT("PIE world exists"), World))
            {
                return false;
            }

            TakeSub = World->GetSubsystem<UDCTakeSubsystem>();
            for (TActorIterator<ADCPlayerCharacter> It(World); It; ++It) { Player = *It; break; }
            for (TActorIterator<ADCStageDirector> It(World); It; ++It) { Director = *It; break; }
            for (TActorIterator<ADCFixedStageCamera> It(World); It; ++It) { Camera = *It; break; }
            for (TActorIterator<ADCCaptainActor> It(World); It; ++It) { Captain = *It; break; }
            for (TActorIterator<ADCDoorwayInteractable> It(World); It; ++It) { Doorway = *It; break; }
            for (TActorIterator<ADCExtinguisherPickup> It(World); It; ++It) { Extinguisher = *It; break; }
            for (TActorIterator<ADCCivilianActor> It(World); It; ++It) { Civilian = *It; break; }
            for (TActorIterator<ADCSafeZoneVolume> It(World); It; ++It) { SafeZone = *It; break; }
            for (TActorIterator<ADCMatchGirlActor> It(World); It; ++It) { MatchGirl = *It; break; }
            for (TActorIterator<ADCAngelDogActor> It(World); It; ++It) { AngelDog = *It; break; }

            AllFires.Reset();
            for (TActorIterator<ADCFireHazardActor> It(World); It; ++It)
            {
                ADCFireHazardActor* FireSpot = *It;
                AllFires.Add(FireSpot);
                if (FireSpot->FireSpotIndex == 0 && !FireF1)
                {
                    FireF1 = FireSpot;
                }
                else if (FireSpot->FireSpotIndex == 1 && !FireF2)
                {
                    FireF2 = FireSpot;
                }
                else if (FireSpot->FireSpotIndex == 2 && !FireF3)
                {
                    FireF3 = FireSpot;
                }
            }

            AllFires.Sort([](const ADCFireHazardActor& A, const ADCFireHazardActor& B)
            {
                return A.FireSpotIndex < B.FireSpotIndex;
            });

            return Test->TestNotNull(TEXT("UDCTakeSubsystem exists"), TakeSub)
                && Test->TestNotNull(TEXT("ADCPlayerCharacter spawned"), Player)
                && Test->TestNotNull(TEXT("ADCStageDirector exists"), Director)
                && Test->TestNotNull(TEXT("ADCFixedStageCamera exists"), Camera)
                && Test->TestNotNull(TEXT("ADCCaptainActor exists"), Captain)
                && Test->TestNotNull(TEXT("ADCDoorwayInteractable exists"), Doorway)
                && Test->TestNotNull(TEXT("ADCExtinguisherPickup exists"), Extinguisher)
                && Test->TestNotNull(TEXT("ADCFireHazard_F1 (index 0) exists"), FireF1)
                && Test->TestNotNull(TEXT("ADCFireHazard_F2 (index 1) exists"), FireF2)
                && Test->TestNotNull(TEXT("ADCFireHazard_F3 (index 2) exists"), FireF3)
                && Test->TestEqual(TEXT("Exactly 3 active fire actors spawned"), AllFires.Num(), 3)
                && Test->TestNotNull(TEXT("ADCCivilianActor exists"), Civilian)
                && Test->TestNotNull(TEXT("ADCSafeZoneVolume exists"), SafeZone)
                && Test->TestNotNull(TEXT("ADCMatchGirlActor exists"), MatchGirl)
                && Test->TestNotNull(TEXT("ADCAngelDogActor exists"), AngelDog);
        }
    };

    static void CapturePieWindowEvidence(UWorld* World, const FString& Filename)
    {
        if (!FParse::Param(FCommandLine::Get(), TEXT("DCCaptureEvidence")) || !World)
        {
            return;
        }

        const FString ValidationDir = FPaths::ProjectSavedDir() / TEXT("Validation");
        IFileManager::Get().MakeDirectory(*ValidationDir, true);
        const FString FullPath = ValidationDir / Filename;

        if (UGameViewportClient* GVC = World->GetGameViewport())
        {
            if (TSharedPtr<SWindow> WindowPtr = GVC->GetWindow())
            {
                if (FSlateApplication::IsInitialized())
                {
                    WindowPtr->BringToFront();
                    TArray<FColor> Bitmap;
                    FIntVector Size(0, 0, 0);
                    if (FSlateApplication::Get().TakeScreenshot(WindowPtr.ToSharedRef(), Bitmap, Size)
                        && Size.X > 0 && Size.Y > 0 && Bitmap.Num() >= Size.X * Size.Y)
                    {
                        for (FColor& Color : Bitmap)
                        {
                            Color.A = 255;
                        }
                        const FImageView ImageView(Bitmap.GetData(), Size.X, Size.Y);
                        if (APlayerController* PC = World->GetFirstPlayerController())
                        {
                            for (TActorIterator<ADCPlayerCharacter> It(World); It; ++It)
                            {
                                ADCPlayerCharacter* P = *It;
                                UStaticMeshComponent* Card = (P && P->PaperVisual) ? P->PaperVisual->CardMeshComp.Get() : nullptr;
                                if (Card)
                                {
                                    const FVector CardCenter = Card->GetComponentLocation();
                                    FVector2D ScreenPos = FVector2D::ZeroVector;
                                    const bool bProjected = PC->ProjectWorldLocationToScreen(CardCenter, ScreenPos, true);
                                    FColor Sampled(0, 0, 0, 0);
                                    const int32 PX = FMath::Clamp(FMath::RoundToInt(ScreenPos.X), 0, Size.X - 1);
                                    const int32 PY = FMath::Clamp(FMath::RoundToInt(ScreenPos.Y), 0, Size.Y - 1);
                                    if (bProjected && Size.X > 0 && Size.Y > 0)
                                    {
                                        Sampled = Bitmap[PY * Size.X + PX];
                                    }
                                    UMaterialInterface* Mat0 = Card->GetMaterial(0);
                                    UE_LOG(LogTemp, Display,
                                        TEXT("DC_DIAG [%s] Loc=%s FacingLeft=%d PVRot=%s CardWorldRot=%s CardScale=%s Mat=%s TwoSided=%d Vis=%d Proj=(%.1f,%.1f ok=%d) RGB=(%d,%d,%d)"),
                                        *Filename,
                                        *P->GetActorLocation().ToString(),
                                        P->PaperVisual->IsFacingLeft() ? 1 : 0,
                                        *P->PaperVisual->GetComponentRotation().ToString(),
                                        *Card->GetComponentRotation().ToString(),
                                        *Card->GetComponentScale().ToString(),
                                        *GetNameSafe(Mat0),
                                        (Mat0 && Mat0->IsTwoSided()) ? 1 : 0,
                                        Card->IsVisible() ? 1 : 0,
                                        ScreenPos.X, ScreenPos.Y, bProjected ? 1 : 0,
                                        Sampled.R, Sampled.G, Sampled.B);
                                }
                                break;
                            }
                        }
                        if (FImageUtils::SaveImageByExtension(*FullPath, ImageView))
                        {
                            return;
                        }
                    }
                }
            }
        }

        FScreenshotRequest::RequestScreenshot(FullPath, true, false, false, FIntRect(), true);
    }

    static void CollectTextsRecursive(UUserWidget* RootUserWidget, TArray<FString>& OutTexts, TSet<const UUserWidget*>& Visited)
    {
        if (!RootUserWidget || Visited.Contains(RootUserWidget) || !RootUserWidget->WidgetTree)
        {
            return;
        }
        Visited.Add(RootUserWidget);

        TArray<UWidget*> Widgets;
        RootUserWidget->WidgetTree->GetAllWidgets(Widgets);
        for (UWidget* Widget : Widgets)
        {
            if (const UTextBlock* TB = Cast<UTextBlock>(Widget))
            {
                const FString TextStr = TB->GetText().ToString();
                if (!TextStr.IsEmpty())
                {
                    OutTexts.Add(TextStr);
                }
            }
            else if (UUserWidget* Nested = Cast<UUserWidget>(Widget))
            {
                CollectTextsRecursive(Nested, OutTexts, Visited);
            }
        }
    }

    static void AssertNoClapperboardStrings(FAutomationTestBase* Test, const TCHAR* ContextLabel, UUserWidget* RootWidget)
    {
        if (!RootWidget)
        {
            return;
        }

        TArray<FString> CollectedTexts;
        TSet<const UUserWidget*> Visited;
        CollectTextsRecursive(RootWidget, CollectedTexts, Visited);

        static const TCHAR* ForbiddenSubstrings[] = {
            TEXT("SCENE 01"),
            TEXT("TAKE #"),
            TEXT("TAKE COMPLETE"),
            TEXT("CUT!"),
            TEXT("NEXT SCENE"),
            TEXT("NEXT LEVEL")
        };

        for (const FString& Text : CollectedTexts)
        {
            const FString Upper = Text.ToUpper().Replace(TEXT("DIRECTOR'S CUT"), TEXT(""));
            for (const TCHAR* Forbidden : ForbiddenSubstrings)
            {
                if (Upper.Contains(Forbidden))
                {
                    Test->AddError(FString::Printf(
                        TEXT("[%s] Forbidden clapperboard/studio string '%s' found in widget text: '%s'"),
                        ContextLabel, Forbidden, *Text));
                }
            }
        }
    }

    static bool ClickCutBlockButton(UDCPersonaPanelWidget* Panel, FName BlockID)
    {
        if (!Panel || !Panel->WidgetTree)
        {
            return false;
        }
        TArray<UWidget*> Widgets;
        Panel->WidgetTree->GetAllWidgets(Widgets);
        for (UWidget* Widget : Widgets)
        {
            if (UDCBlockCutButton* CutWidget = Cast<UDCBlockCutButton>(Widget))
            {
                if (CutWidget->GetBlockID() == BlockID && CutWidget->WidgetTree)
                {
                    TArray<UWidget*> InnerWidgets;
                    CutWidget->WidgetTree->GetAllWidgets(InnerWidgets);
                    for (UWidget* Inner : InnerWidgets)
                    {
                        if (UButton* Btn = Cast<UButton>(Inner))
                        {
                            Btn->OnClicked.Broadcast();
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    static bool ClickButtonWithCaption(UUserWidget* RootWidget, const FString& ExactCaption)
    {
        if (!RootWidget || !RootWidget->WidgetTree)
        {
            return false;
        }
        TArray<UWidget*> Widgets;
        RootWidget->WidgetTree->GetAllWidgets(Widgets);
        for (UWidget* Widget : Widgets)
        {
            if (UButton* Btn = Cast<UButton>(Widget))
            {
                TArray<UWidget*> Stack;
                Stack.Add(Btn->GetContent());
                while (Stack.Num() > 0)
                {
                    UWidget* Cur = Stack.Pop();
                    if (!Cur)
                    {
                        continue;
                    }
                    if (const UTextBlock* TB = Cast<UTextBlock>(Cur))
                    {
                        if (TB->GetText().ToString().TrimStartAndEnd().Equals(ExactCaption, ESearchCase::IgnoreCase))
                        {
                            Btn->OnClicked.Broadcast();
                            return true;
                        }
                    }
                    if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(Cur))
                    {
                        for (int32 ChildIdx = 0; ChildIdx < PanelWidget->GetChildrenCount(); ++ChildIdx)
                        {
                            Stack.Add(PanelWidget->GetChildAt(ChildIdx));
                        }
                    }
                }
            }
        }
        return false;
    }

    static void CollectButtonCaptions(UUserWidget* RootWidget, TArray<FString>& OutCaptions)
    {
        if (!RootWidget || !RootWidget->WidgetTree)
        {
            return;
        }
        TArray<UWidget*> Widgets;
        RootWidget->WidgetTree->GetAllWidgets(Widgets);
        for (UWidget* Widget : Widgets)
        {
            if (UButton* Btn = Cast<UButton>(Widget))
            {
                FString CombinedCaption;
                TArray<UWidget*> Stack;
                Stack.Add(Btn->GetContent());
                while (Stack.Num() > 0)
                {
                    UWidget* Cur = Stack.Pop();
                    if (!Cur)
                    {
                        continue;
                    }
                    if (const UTextBlock* TB = Cast<UTextBlock>(Cur))
                    {
                        if (!CombinedCaption.IsEmpty())
                        {
                            CombinedCaption += TEXT(" ");
                        }
                        CombinedCaption += TB->GetText().ToString();
                    }
                    if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(Cur))
                    {
                        for (int32 ChildIdx = 0; ChildIdx < PanelWidget->GetChildrenCount(); ++ChildIdx)
                        {
                            Stack.Add(PanelWidget->GetChildAt(ChildIdx));
                        }
                    }
                }
                OutCaptions.Add(CombinedCaption.TrimStartAndEnd());
            }
        }
    }

    static TArray<UDCJournalStarWidget*> CollectResultStars(UDCTakeResultWidget* ResultWidget)
    {
        TArray<UDCJournalStarWidget*> Stars;
        if (ResultWidget && ResultWidget->WidgetTree)
        {
            TArray<UWidget*> Widgets;
            ResultWidget->WidgetTree->GetAllWidgets(Widgets);
            for (UWidget* Widget : Widgets)
            {
                if (UDCJournalStarWidget* StarWidget = Cast<UDCJournalStarWidget>(Widget))
                {
                    Stars.Add(StarWidget);
                }
            }
        }
        return Stars;
    }

    static void SendSimulatedKey(ADCPlayerCharacter* Player, FKey Key, EInputEvent Event)
    {
        if (Player)
        {
            if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
            {
                PC->InputKey(FInputKeyEventArgs::CreateSimulated(
                    Key,
                    Event,
                    Event == IE_Released ? 0.0f : 1.0f));
            }
        }
    }

    static void CheckSubMeshPositiveScaleIfVisible(FAutomationTestBase* Test, const TCHAR* Label, const UStaticMeshComponent* MeshComp)
    {
        if (MeshComp && MeshComp->IsVisible())
        {
            const FVector Scale = MeshComp->GetRelativeScale3D();
            Test->TestTrue(
                FString::Printf(TEXT("%s uses strictly positive scale (%.2f, %.2f, %.2f)"), Label, Scale.X, Scale.Y, Scale.Z),
                Scale.X > 0.0f && Scale.Y > 0.0f && Scale.Z > 0.0f);
        }
    }

    static void CheckPaperUprightAndGrounded(FAutomationTestBase* Test, const TCHAR* Label, const UDCPaperVisualComponent* Paper, float ExpectedFootZ, float ToleranceZ)
    {
        if (!Test->TestNotNull(Label, Paper))
        {
            return;
        }
        const FRotator WorldRot = Paper->GetComponentRotation();
        const float AbsPitch = FMath::Abs(FRotator::NormalizeAxis(WorldRot.Pitch));
        const float AbsRoll = FMath::Abs(FRotator::NormalizeAxis(WorldRot.Roll));
        Test->TestTrue(FString::Printf(TEXT("%s paper card upright (Pitch=%.2f, Roll=%.2f)"), Label, AbsPitch, AbsRoll),
            AbsPitch <= 0.5f && AbsRoll <= 0.5f);
        const float ActualFootZ = Paper->GetComponentLocation().Z;
        Test->TestTrue(FString::Printf(TEXT("%s paper card bottom grounded (Z=%.2f expected=%.2f)"), Label, ActualFootZ, ExpectedFootZ),
            FMath::Abs(ActualFootZ - ExpectedFootZ) <= ToleranceZ);
        if (Test->TestNotNull(FString::Printf(TEXT("%s CardMeshComp exists"), Label), Paper->CardMeshComp.Get()))
        {
            const FVector CardScale = Paper->CardMeshComp->GetRelativeScale3D();
            Test->TestTrue(
                FString::Printf(TEXT("%s CardMeshComp uses strictly positive scale (%.2f, %.2f, %.2f)"), Label, CardScale.X, CardScale.Y, CardScale.Z),
                CardScale.X > 0.0f && CardScale.Y > 0.0f && CardScale.Z > 0.0f);
            Test->TestTrue(FString::Printf(TEXT("%s CardMeshComp visible"), Label),
                Paper->CardMeshComp->IsVisible());
        }
        CheckSubMeshPositiveScaleIfVisible(Test, Label, Paper->BuddyCardMeshComp.Get());
        CheckSubMeshPositiveScaleIfVisible(Test, Label, Paper->ToolCardMeshComp.Get());
        CheckSubMeshPositiveScaleIfVisible(Test, Label, Paper->SprayCardMeshComp.Get());
    }

    static void CheckAllVisiblePaperCards(FAutomationTestBase* Test, const FStageActors& S, const TCHAR* ContextLabel)
    {
        CheckPaperUprightAndGrounded(Test,
            *FString::Printf(TEXT("%s_Player"), ContextLabel),
            S.Player->PaperVisual,
            S.Player->GetActorLocation().Z - 86.0f,
            2.0f);

        if (S.Player->ActionStateComponent && S.Player->ActionStateComponent->HasCapability(DCGameplayTags::State_EscortingCivilian))
        {
            const bool bHasBuddyCardFallback =
                S.Player->PaperVisual &&
                S.Player->PaperVisual->BuddyCardMeshComp &&
                S.Player->PaperVisual->BuddyCardMeshComp->IsVisible();
            const bool bHasCombinedEscortTexture =
                S.Player->PaperVisual &&
                S.Player->PaperVisual->HasCustomTextureForCurrentState() &&
                S.Player->PaperVisual->CardMeshComp &&
                S.Player->PaperVisual->CardMeshComp->IsVisible();
            Test->TestTrue(
                FString::Printf(TEXT("%s: Escort visual visible on Player (BuddyCardMeshComp fallback or single-sheet combined texture)"), ContextLabel),
                bHasBuddyCardFallback || bHasCombinedEscortTexture);
        }

        CheckPaperUprightAndGrounded(Test, *FString::Printf(TEXT("%s_Captain"), ContextLabel), S.Captain->PaperVisual, 0.0f, 1.0f);
        CheckPaperUprightAndGrounded(Test, *FString::Printf(TEXT("%s_Doorway"), ContextLabel), S.Doorway->PaperVisual, 0.0f, 1.0f);

        if (S.Extinguisher && S.Extinguisher->PaperVisual && S.Extinguisher->PaperVisual->CardMeshComp && S.Extinguisher->PaperVisual->CardMeshComp->IsVisible())
        {
            CheckPaperUprightAndGrounded(Test, *FString::Printf(TEXT("%s_Extinguisher"), ContextLabel), S.Extinguisher->PaperVisual, 0.0f, 1.0f);
        }

        if (S.Civilian && S.Civilian->IsEscorting())
        {
            Test->TestFalse(
                FString::Printf(TEXT("%s: Standalone Civilian CardMeshComp hidden during escort"), ContextLabel),
                S.Civilian->PaperVisual && S.Civilian->PaperVisual->CardMeshComp && S.Civilian->PaperVisual->CardMeshComp->IsVisible());
        }
        else if (S.Civilian)
        {
            CheckPaperUprightAndGrounded(Test, *FString::Printf(TEXT("%s_Civilian"), ContextLabel), S.Civilian->PaperVisual, 0.0f, 1.0f);
        }

        for (ADCFireHazardActor* FireSpot : S.AllFires)
        {
            if (FireSpot
                && FireSpot->IsSpotEnabledInLevel()
                && FireSpot->GetCurrentFireStage() != EDCFireStage::Extinguished
                && FireSpot->PaperVisual
                && FireSpot->PaperVisual->CardMeshComp
                && FireSpot->PaperVisual->CardMeshComp->IsVisible())
            {
                CheckPaperUprightAndGrounded(Test,
                    *FString::Printf(TEXT("%s_FireSpot_%d"), ContextLabel, FireSpot->FireSpotIndex),
                    FireSpot->PaperVisual,
                    0.0f,
                    1.0f);
            }
        }

        if (S.MatchGirl)
        {
            CheckPaperUprightAndGrounded(Test,
                *FString::Printf(TEXT("%s_MatchGirl"), ContextLabel),
                S.MatchGirl->PaperVisual,
                0.0f,
                1.0f);
        }
        if (S.AngelDog)
        {
            CheckPaperUprightAndGrounded(Test,
                *FString::Printf(TEXT("%s_AngelDog"), ContextLabel),
                S.AngelDog->PaperVisual,
                0.0f,
                1.0f);
        }
    }

    static bool MovePlayerAlongWaypointsSwept(
        FAutomationTestBase* Test,
        ADCPlayerCharacter* Player,
        const TArray<FVector>& Waypoints,
        const TCHAR* RouteLabel)
    {
        if (!Player)
        {
            return false;
        }
        bool bAllOk = true;
        for (int32 WPIdx = 0; WPIdx < Waypoints.Num(); ++WPIdx)
        {
            const FVector Target = Waypoints[WPIdx];
            const FVector Start = Player->GetActorLocation();
            const float Dist = FVector::Distance(Start, Target);
            const int32 SubSteps = FMath::Max(1, FMath::CeilToInt(Dist / 20.0f));
            for (int32 StepIdx = 1; StepIdx <= SubSteps; ++StepIdx)
            {
                const float Alpha = static_cast<float>(StepIdx) / static_cast<float>(SubSteps);
                const FVector StepTarget = FMath::Lerp(Start, Target, Alpha);
                FHitResult Hit;
                Player->SetActorLocation(StepTarget, true, &Hit);
                if (Hit.bBlockingHit)
                {
                    bAllOk = false;
                    Test->AddError(FString::Printf(
                        TEXT("%s: Swept S-curve waypoint %d blocked by %s (%s) at %s toward %s"),
                        RouteLabel,
                        WPIdx,
                        *GetNameSafe(Hit.GetActor()),
                        *GetNameSafe(Hit.GetComponent()),
                        *Player->GetActorLocation().ToString(),
                        *Target.ToString()));
                    break;
                }
            }
        }
        return bAllOk;
    }

    static void SetupAndExtinguishFireViaConfig(
        const FStageActors& S,
        const FVector& NearExtinguisherLoc,
        const TArray<ADCFireHazardActor*>& SpotsToExtinguish)
    {
        S.Player->SetActorLocation(NearExtinguisherLoc);
        S.Extinguisher->TryInteract(S.Player);
        for (ADCFireHazardActor* Spot : SpotsToExtinguish)
        {
            if (Spot && Spot->IsSpotEnabledInLevel() && Spot->GetCurrentFireStage() != EDCFireStage::Extinguished)
            {
                Spot->CompleteExtinguishNow();
            }
        }
    }

    static bool WidgetContainsText(UUserWidget* RootWidget, const FString& Substring)
    {
        if (!RootWidget)
        {
            return false;
        }
        TArray<FString> Texts;
        TSet<const UUserWidget*> Visited;
        CollectTextsRecursive(RootWidget, Texts, Visited);
        for (const FString& Entry : Texts)
        {
            if (Entry.Contains(Substring))
            {
                return true;
            }
        }
        return false;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDCAcceptance,
    "DirectorCut.PIE.Acceptance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

DEFINE_LATENT_AUTOMATION_COMMAND(FDCStartFloatingPIE);
bool FDCStartFloatingPIE::Update()
{
    FRequestPlaySessionParams Params;
    Params.SessionDestination = EPlaySessionDestinationType::InProcess;
    Params.WorldType = EPlaySessionWorldType::PlayInEditor;
    GEditor->RequestPlaySession(Params);
    return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FDCCaptureEvidenceCommand, FString, Filename);
bool FDCCaptureEvidenceCommand::Update()
{
    UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
    DCValidationInternal::CapturePieWindowEvidence(PlayWorld, Filename);
    return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FDCAcceptanceStep, FAutomationTestBase*, Test, int32, Step);
bool FDCAcceptanceStep::Update()
{
    using namespace DCValidationInternal;

    FStageActors S;
    if (!S.Gather(Test))
    {
        return true;
    }

    auto* Overlay = Cast<UDCHudOverlayWidget>(S.Player->HudOverlayWidget);

    if (Step == 0)
    {
        GAcceptanceState = FAcceptanceRuntimeState();
        S.TakeSub->RestartTake();

        // 1. Spatial, Camera, Config, & S-Curve Architecture Contract
        APlayerController* PC = Cast<APlayerController>(S.Player->GetController());
        Test->TestNotNull(TEXT("PlayerController exists"), PC);
        if (PC)
        {
            Test->TestTrue(TEXT("Active view target is ADCFixedStageCamera"), PC->GetViewTarget() == S.Camera);
        }

        UCameraComponent* CamComp = S.Camera->GetCameraComponent();
        if (Test->TestNotNull(TEXT("Fixed stage camera has CameraComponent"), CamComp))
        {
            Test->TestEqual(TEXT("Camera projection is Perspective"), CamComp->ProjectionMode, ECameraProjectionMode::Perspective);
            Test->TestTrue(TEXT("Camera FOV in [55, 65]"), CamComp->FieldOfView >= 55.0f && CamComp->FieldOfView <= 65.0f);
        }
        const FRotator CamRot = S.Camera->GetActorRotation();
        Test->TestTrue(
            FString::Printf(TEXT("Camera Pitch in [-20, -10] (actual=%.2f)"), CamRot.Pitch),
            CamRot.Pitch >= -20.0f && CamRot.Pitch <= -10.0f);
        Test->TestTrue(TEXT("Camera Roll is near 0"), FMath::Abs(FRotator::NormalizeAxis(CamRot.Roll)) <= 0.1f);

        Test->TestTrue(TEXT("Closed ceiling visible on StageDirector"),
            S.Director->CeilingMesh != nullptr && S.Director->CeilingMesh->IsVisible());
        Test->TestTrue(TEXT("PartitionB1 visible on StageDirector"),
            S.Director->PartitionB1Mesh != nullptr && S.Director->PartitionB1Mesh->IsVisible());
        Test->TestTrue(TEXT("PartitionB2 visible on StageDirector"),
            S.Director->PartitionB2Mesh != nullptr && S.Director->PartitionB2Mesh->IsVisible());
        Test->TestTrue(TEXT("FrontFloorApron visible on StageDirector"),
            S.Director->FrontFloorApronMesh != nullptr && S.Director->FrontFloorApronMesh->IsVisible());
        Test->TestTrue(TEXT("ExteriorBackWall visible on StageDirector"),
            S.Director->ExteriorBackWallMesh != nullptr && S.Director->ExteriorBackWallMesh->IsVisible());
        Test->TestTrue(TEXT("ExteriorLeftWall visible on StageDirector"),
            S.Director->ExteriorLeftWallMesh != nullptr && S.Director->ExteriorLeftWallMesh->IsVisible());
        Test->TestNull(TEXT("No USpringArmComponent on player"),
            S.Player->FindComponentByClass<USpringArmComponent>());

        Test->TestTrue(TEXT("MatchGirl placed near (225, 335, 0)"),
            S.MatchGirl->GetActorLocation().Equals(FVector(225.0f, 335.0f, 0.0f), 10.0f));
        Test->TestTrue(TEXT("AngelDog placed near (415, -460, 0)"),
            S.AngelDog->GetActorLocation().Equals(FVector(415.0f, -460.0f, 0.0f), 10.0f));

        // Verify v3.1 Fire Call config & initial counts
        const UDCLevelConfigData* Config = S.TakeSub->GetActiveConfig();
        if (Test->TestNotNull(TEXT("Active LevelConfig bound"), Config))
        {
            Test->TestTrue(TEXT("SecondsToInjureCivilian ~= 9.5f"), FMath::IsNearlyEqual(Config->SecondsToInjureCivilian, 9.5f, 0.05f));
            Test->TestTrue(TEXT("SecondsToKillCivilian ~= 19.5f"), FMath::IsNearlyEqual(Config->SecondsToKillCivilian, 19.5f, 0.05f));
            Test->TestTrue(TEXT("SecondsToEngulfRoom ~= 20.5f"), FMath::IsNearlyEqual(Config->SecondsToEngulfRoom, 20.5f, 0.05f));
            Test->TestTrue(TEXT("ExtinguishActionSeconds ~= 1.6f"), FMath::IsNearlyEqual(Config->ExtinguishActionSeconds, 1.6f, 0.05f));
            Test->TestTrue(TEXT("EngulfDelayPerSpotSeconds ~= 3.5f"), FMath::IsNearlyEqual(Config->EngulfDelayPerSpotSeconds, 3.5f, 0.05f));
            Test->TestTrue(TEXT("PlayerWalkSpeed ~= 320.0f"), FMath::IsNearlyEqual(Config->PlayerWalkSpeed, 320.0f, 0.5f));
            Test->TestTrue(TEXT("EscortMoveSpeed ~= 295.0f"), FMath::IsNearlyEqual(Config->EscortMoveSpeed, 295.0f, 0.5f));
            Test->TestTrue(TEXT("WeakEscortMoveSpeed ~= 130.0f"), FMath::IsNearlyEqual(Config->WeakEscortMoveSpeed, 130.0f, 0.5f));
            Test->TestTrue(TEXT("Smooth pan follow enabled"), Config->bEnableSmoothPanFollow);
            Test->TestEqual(TEXT("ProtagonistVisualMode is Paper2D"), Config->ProtagonistVisualMode, EDCCharacterVisualMode::Paper2D);
            Test->TestEqual(TEXT("ActiveTakePersonaViewPolicy is AllowTabReadOnlyPeek"),
                Config->ActiveTakePersonaViewPolicy, EDCActiveTakePersonaViewPolicy::AllowTabReadOnlyPeek);
            Test->TestNotNull(TEXT("StageBgmSound assigned"), Config->StageBgmSound.Get());
            Test->TestNotNull(TEXT("BuddyUninjuredTexture assigned"), Config->BuddyUninjuredTexture.Get());
            Test->TestNotNull(TEXT("BuddyInjuredTexture assigned"), Config->BuddyInjuredTexture.Get());
            Test->TestTrue(TEXT("BgmAudioComponent wired with StageBgmSound"),
                S.Director->BgmAudioComponent != nullptr && S.Director->BgmAudioComponent->Sound == Config->StageBgmSound.Get());
        }

        CheckAllVisiblePaperCards(Test, S, TEXT("Step0_Init"));

        // Verify horizontal flip on movement preserves upright, grounded, positive scale
        const FVector CamRight = S.Camera->GetCameraPlaneRight();
        S.Player->PaperVisual->UpdateMovementFlip(CamRight * 120.0f, CamRight);
        Test->TestFalse(TEXT("Moving camera-right faces right"), S.Player->PaperVisual->IsFacingLeft());
        CheckPaperUprightAndGrounded(Test, TEXT("PlayerFacingRight"), S.Player->PaperVisual, S.Player->GetActorLocation().Z - 86.0f, 2.0f);
        S.Player->PaperVisual->UpdateMovementFlip(CamRight * -120.0f, CamRight);
        Test->TestTrue(TEXT("Moving camera-left flips horizontally"), S.Player->PaperVisual->IsFacingLeft());
        S.Player->PaperVisual->UpdateMovementFlip(FVector::ZeroVector, CamRight);
        CheckPaperUprightAndGrounded(Test, TEXT("PlayerPostFlip"), S.Player->PaperVisual, S.Player->GetActorLocation().Z - 86.0f, 2.0f);

        // PreTake state & HUD checks
        Test->TestTrue(TEXT("Take starts in PreTake"), S.TakeSub->IsPreTake());
        Test->TestEqual(TEXT("Initial fire spot count == 3"), S.TakeSub->GetInitialFireSpotCount(), 3);
        Test->TestEqual(TEXT("Remaining fire spot count == 3"), S.TakeSub->GetRemainingFireSpotCount(), 3);
        Test->TestEqual(TEXT("Initial star containers == 3"), S.TakeSub->GetMaxStarContainers(), 3);
        Test->TestEqual(TEXT("Initial earned stars == 0"), S.TakeSub->GetEarnedStars(), 0);
        Test->TestFalse(TEXT("Player cannot move during PreTake"), S.Player->CanMove());
        S.Director->Tick(1.0f);
        Test->TestEqual(TEXT("PreTake pauses elapsed take time"), S.TakeSub->GetElapsedTakeSeconds(), 0.0f);

        Test->TestFalse(TEXT("Captain has no dialogue"), S.Captain->bHasDialogue);
        Test->TestEqual(TEXT("Persona starts with 6 v3.1 Firefighter blocks"), S.Player->PersonaComponent->GetBlocks().Num(), 6);
        Test->TestTrue(TEXT("Identity block present"), S.Player->PersonaComponent->IsBlockPresent(TEXT("Identity")));
        Test->TestTrue(TEXT("Motivation block present"), S.Player->PersonaComponent->IsBlockPresent(TEXT("Motivation")));
        Test->TestTrue(TEXT("AngelPupWorkout block present"), S.Player->PersonaComponent->IsBlockPresent(TEXT("AngelPupWorkout")));
        Test->TestTrue(TEXT("WitchCaptainTrauma block present"), S.Player->PersonaComponent->IsBlockPresent(TEXT("WitchCaptainTrauma")));
        Test->TestTrue(TEXT("ExtinguisherTraining block present"), S.Player->PersonaComponent->IsBlockPresent(TEXT("ExtinguisherTraining")));
        Test->TestTrue(TEXT("SafetyDogma block present"), S.Player->PersonaComponent->IsBlockPresent(TEXT("SafetyDogma")));

        Test->TestTrue(TEXT("AngelPupWorkout grants Persona_Trait_StrongLegs"),
            S.Player->PersonaComponent->HasPersonaTag(DCGameplayTags::Persona_Trait_StrongLegs));
        Test->TestTrue(TEXT("WitchCaptainTrauma grants Persona_Limit_FearDisobeyCaptain"),
            S.Player->PersonaComponent->HasPersonaTag(DCGameplayTags::Persona_Limit_FearDisobeyCaptain));
        Test->TestTrue(TEXT("ExtinguisherTraining grants Persona_KnowHow_UseExtinguisher"),
            S.Player->PersonaComponent->HasPersonaTag(DCGameplayTags::Persona_KnowHow_UseExtinguisher));
        Test->TestTrue(TEXT("SafetyDogma grants Persona_Limit_MustClearFireFirst"),
            S.Player->PersonaComponent->HasPersonaTag(DCGameplayTags::Persona_Limit_MustClearFireFirst));
        Test->TestFalse(TEXT("Non-deletable Identity cannot be deleted"),
            S.Player->PersonaComponent->TryDeleteBlock(TEXT("Identity")));
        Test->TestFalse(TEXT("Non-deletable Motivation cannot be deleted"),
            S.Player->PersonaComponent->TryDeleteBlock(TEXT("Motivation")));
        Test->TestFalse(TEXT("Unknown block cannot be deleted"),
            S.Player->PersonaComponent->TryDeleteBlock(TEXT("Missing")));

        if (Test->TestNotNull(TEXT("UDCHudOverlayWidget active"), Overlay))
        {
            Test->TestTrue(TEXT("HudOverlay is in viewport"), Overlay->IsInViewport());
            UDCPersonaPanelWidget* Panel = Overlay->GetPersonaPanel();
            if (Test->TestNotNull(TEXT("PersonaPanel exists"), Panel))
            {
                Test->TestTrue(TEXT("Full-screen Persona sheet visible in PreTake"), Panel->IsVisible());
                UBorder* RootBackdrop = Panel->WidgetTree ? Cast<UBorder>(Panel->WidgetTree->RootWidget.Get()) : nullptr;
                Test->TestTrue(TEXT("PreTake journal backdrop is opaque (alpha >= 0.99)"),
                    RootBackdrop != nullptr && RootBackdrop->IsVisible() && RootBackdrop->GetBrushColor().A >= 0.99f);

                TArray<UWidget*> PanelWidgets;
                int32 BlockRowCount = 0;
                if (Panel->WidgetTree)
                {
                    Panel->WidgetTree->GetAllWidgets(PanelWidgets);
                    for (UWidget* W : PanelWidgets)
                    {
                        if (Cast<UDCBlockCutButton>(W))
                        {
                            ++BlockRowCount;
                        }
                    }
                }
                Test->TestEqual(TEXT("Persona sheet renders 6 block rows"), BlockRowCount, 6);
                Test->TestTrue(TEXT("Persona sheet displays Director's Cut badge"),
                    WidgetContainsText(Panel, TEXT("Director's Cut")));
            }
            Test->TestFalse(TEXT("StarPopup starts non-persistent (Collapsed)"),
                Overlay->GetStarPopup() && Overlay->GetStarPopup()->IsPopupVisible());
            Test->TestFalse(TEXT("ResultWidget hidden in PreTake"),
                Overlay->GetResultWidget() && Overlay->GetResultWidget()->IsVisible());

            // In PreTake, Tab keeps full-screen Persona sheet open
            S.Player->TogglePersona();
            Test->TestTrue(TEXT("PreTake keeps PersonaPanelOpen == true on Tab"),
                S.Player->ActionStateComponent->IsPersonaPanelOpen());

            // Cut SafetyDogma block via UMG so V31_PreTake.png shows cut strikethrough + Director's Cut badge + ACTION!
            const bool bCutSafetyDogma = ClickCutBlockButton(Overlay->GetPersonaPanel(), TEXT("SafetyDogma"));
            Test->TestTrue(TEXT("Clicked SafetyDogma CutButton in PreTake"), bCutSafetyDogma);
            Test->TestFalse(TEXT("SafetyDogma marked cut (bIsPresent == false)"),
                S.Player->PersonaComponent->IsBlockPresent(TEXT("SafetyDogma")));
            Test->TestTrue(TEXT("WitchCaptainTrauma STILL present after cutting SafetyDogma"),
                S.Player->PersonaComponent->IsBlockPresent(TEXT("WitchCaptainTrauma")));
            Test->TestTrue(TEXT("FearDisobeyCaptain STILL active after cutting SafetyDogma"),
                S.Player->PersonaComponent->HasPersonaTag(DCGameplayTags::Persona_Limit_FearDisobeyCaptain));

            AssertNoClapperboardStrings(Test, TEXT("PreTake"), Overlay);
        }
    }
    else if (Step == 1)
    {
        // Click ACTION! button in PreTake sheet to enter ActiveTake
        UDCPersonaPanelWidget* Panel = Overlay ? Overlay->GetPersonaPanel() : nullptr;
        AssertNoClapperboardStrings(Test, TEXT("PreTake_PostCut"), Overlay);
        Test->TestTrue(TEXT("Clicked ACTION! button in PersonaPanel"),
            ClickButtonWithCaption(Panel, TEXT("ACTION!")));
        Test->TestTrue(TEXT("ACTION! starts ActiveTake"), S.TakeSub->IsActiveTake());
        Test->TestFalse(TEXT("Persona editing locked in ActiveTake"), S.Player->PersonaComponent->CanEditNow());
        Test->TestFalse(TEXT("TryDeleteBlock returns false in ActiveTake"),
            S.Player->PersonaComponent->TryDeleteBlock(TEXT("WitchCaptainTrauma")));
        Test->TestTrue(TEXT("WitchCaptainTrauma still active after locked delete attempt"),
            S.Player->PersonaComponent->IsBlockPresent(TEXT("WitchCaptainTrauma")));
        Test->TestFalse(TEXT("Persona panel hidden on entering ActiveTake"),
            Panel && Panel->IsVisible());

        // v3.1 AllowTabReadOnlyPeek: Tab opens read-only peek next tick
        SendSimulatedKey(S.Player, EKeys::Tab, IE_Pressed);
        SendSimulatedKey(S.Player, EKeys::Tab, IE_Released);
    }
    else if (Step == 101)
    {
        Test->TestTrue(TEXT("Tab during ActiveTake opens read-only Persona peek"),
            S.Player->ActionStateComponent->IsPersonaPanelOpen() && Overlay && Overlay->GetPersonaPanel() && Overlay->GetPersonaPanel()->IsVisible());
        Test->TestTrue(TEXT("Read-only peek shows READ-ONLY badge and peek prompt"),
            Overlay && WidgetContainsText(Overlay->GetPersonaPanel(), TEXT("READ-ONLY"))
            && WidgetContainsText(Overlay->GetPersonaPanel(), TEXT("READ-ONLY PEEK")));
        Test->TestFalse(TEXT("Read-only peek still forbids deleting blocks"),
            S.Player->PersonaComponent->TryDeleteBlock(TEXT("WitchCaptainTrauma")));

        SendSimulatedKey(S.Player, EKeys::Tab, IE_Pressed);
        SendSimulatedKey(S.Player, EKeys::Tab, IE_Released);
    }
    else if (Step == 102)
    {
        UDCPersonaPanelWidget* Panel = Overlay ? Overlay->GetPersonaPanel() : nullptr;
        Test->TestFalse(TEXT("Second Tab closes Persona peek"),
            S.Player->ActionStateComponent->IsPersonaPanelOpen() || (Panel && Panel->IsVisible()));

        AssertNoClapperboardStrings(Test, TEXT("ActiveTake_Entry"), Overlay);

        // Move to exterior doorway and trigger Chief Witch refusal + dot-tail thought bubble
        Test->TestFalse(TEXT("Doorway starts locked"), S.Doorway->IsDoorUnlocked());
        S.Player->SetActorLocation(FVector(350.0f, -345.0f, 92.0f));
        S.Player->Tick(0.02f);
        SendSimulatedKey(S.Player, EKeys::E, IE_Pressed);
    }
    else if (Step == 2)
    {
        SendSimulatedKey(S.Player, EKeys::E, IE_Released);

        Test->TestFalse(TEXT("Doorway remains locked after denial"), S.Doorway->IsDoorUnlocked());
        Test->TestTrue(TEXT("Debug reason records Denied_BlockedByPersona"),
            S.Player->ActionStateComponent->GetLastDebugReason().Contains(TEXT("Denied_BlockedByPersona")));
        Test->TestEqual(TEXT("ThoughtWidgetComponent uses Screen space"),
            S.Player->ThoughtWidgetComponent->GetWidgetSpace(), EWidgetSpace::Screen);
        Test->TestTrue(TEXT("ThoughtWidgetComponent visible on denial"),
            S.Player->ThoughtWidgetComponent->IsVisible());

        auto* Bubble = Cast<UDCThoughtBubbleWidget>(S.Player->ThoughtWidget);
        if (Test->TestNotNull(TEXT("ThoughtBubble widget created"), Bubble))
        {
            const FString BubbleStr = Bubble->GetThoughtText().ToString();
            Test->TestTrue(TEXT("ThoughtBubble displays Chief Witch refusal + red heels clue"),
                BubbleStr.Contains(TEXT("Chief Witch")) && BubbleStr.ToLower().Contains(TEXT("red heels")));
        }

        Test->TestTrue(TEXT("[E] ENTER InteractPromptWidgetComponent visible at doorway"),
            S.Player->InteractPromptWidgetComponent->IsVisible());
        auto* PromptW = Cast<UDCInteractPromptWidget>(S.Player->InteractPromptWidget);
        if (Test->TestNotNull(TEXT("InteractPromptWidget created"), PromptW))
        {
            Test->TestEqual(TEXT("Doorway prompt label is ENTER"),
                PromptW->GetPromptLabelText().ToString(), FString(TEXT("ENTER")));
            Test->TestFalse(TEXT("Doorway prompt is not HoldMode"), PromptW->IsHoldMode());
        }

        Test->TestFalse(TEXT("Captain stays silent"), S.Captain->bHasDialogue);
        Test->TestTrue(TEXT("Captain remains at blocking post (370,-375,0)"),
            S.Captain->GetActorLocation().Equals(FVector(370.0f, -375.0f, 0.0f), 2.0f));
        GAcceptanceState.PorchCameraLocation = S.Camera->GetActorLocation();
        CheckAllVisiblePaperCards(Test, S, TEXT("Step2_Blocked"));
    }
    else if (Step == 3)
    {
        // Trigger full [R] Retake via simulated R key
        SendSimulatedKey(S.Player, EKeys::R, IE_Pressed);
    }
    else if (Step == 4)
    {
        SendSimulatedKey(S.Player, EKeys::R, IE_Released);

        Test->TestTrue(TEXT("R Retake returns to PreTake"), S.TakeSub->IsPreTake());
        Test->TestTrue(TEXT("R Retake restores SafetyDogma"), S.Player->PersonaComponent->IsBlockPresent(TEXT("SafetyDogma")));
        Test->TestTrue(TEXT("R Retake restores WitchCaptainTrauma"), S.Player->PersonaComponent->IsBlockPresent(TEXT("WitchCaptainTrauma")));
        Test->TestTrue(TEXT("R Retake restores ExtinguisherTraining"), S.Player->PersonaComponent->IsBlockPresent(TEXT("ExtinguisherTraining")));
        Test->TestTrue(TEXT("R Retake restores AngelPupWorkout"), S.Player->PersonaComponent->IsBlockPresent(TEXT("AngelPupWorkout")));
        Test->TestTrue(TEXT("R Retake restores FearDisobeyCaptain tag"),
            S.Player->PersonaComponent->HasPersonaTag(DCGameplayTags::Persona_Limit_FearDisobeyCaptain));
        Test->TestFalse(TEXT("R Retake clears thought bubble"), S.Player->ThoughtWidgetComponent->IsVisible());
        Test->TestFalse(TEXT("R Retake hides interact prompt"), S.Player->InteractPromptWidgetComponent->IsVisible());
        Test->TestFalse(TEXT("R Retake relocks doorway"), S.Doorway->IsDoorUnlocked());
        Test->TestEqual(TEXT("R Retake restores 3 star containers"), S.TakeSub->GetMaxStarContainers(), 3);
        Test->TestEqual(TEXT("R Retake resets earned stars to 0"), S.TakeSub->GetEarnedStars(), 0);
        Test->TestEqual(TEXT("R Retake restores 3 remaining fires"), S.TakeSub->GetRemainingFireSpotCount(), 3);
        for (ADCFireHazardActor* FireSpot : S.AllFires)
        {
            Test->TestEqual(TEXT("R Retake restores fire stage to Active"), FireSpot->GetCurrentFireStage(), EDCFireStage::Active);
            Test->TestTrue(TEXT("R Retake resets RetainedSpraySeconds"), FMath::IsNearlyZero(FireSpot->GetRetainedSpraySeconds()));
        }
        Test->TestEqual(TEXT("R Retake restores civilian to Uninjured"), S.Civilian->GetCivilianState(), EDCCivilianState::Uninjured);
        Test->TestEqual(TEXT("R Retake resets AngelDog PaperState to None"),
            S.AngelDog->PaperVisual ? S.AngelDog->PaperVisual->GetPaperState() : FName(TEXT("INVALID")), NAME_None);
        Test->TestTrue(TEXT("R Retake returns player to spawn (500,-345,92)"),
            S.Player->GetActorLocation().Equals(S.Director->PlayerSpawnLocation, 5.0f));
        Test->TestFalse(TEXT("R Retake resets PaperVisual facing right"), S.Player->PaperVisual->IsFacingLeft());
        AssertNoClapperboardStrings(Test, TEXT("Post_R_Retake"), Overlay);

        // Cut ONLY WitchCaptainTrauma via UMG -> click ACTION! -> unlock doorway (leaving SafetyDogma active for Rule #1 test)
        UDCPersonaPanelWidget* Panel = Overlay ? Overlay->GetPersonaPanel() : nullptr;
        Test->TestTrue(TEXT("Cut WitchCaptainTrauma via UMG"),
            ClickCutBlockButton(Panel, TEXT("WitchCaptainTrauma")));
        Test->TestFalse(TEXT("WitchCaptainTrauma cut"), S.Player->PersonaComponent->IsBlockPresent(TEXT("WitchCaptainTrauma")));
        Test->TestFalse(TEXT("FearDisobeyCaptain removed"),
            S.Player->PersonaComponent->HasPersonaTag(DCGameplayTags::Persona_Limit_FearDisobeyCaptain));
        Test->TestFalse(TEXT("Cannot cut WitchCaptainTrauma twice"),
            S.Player->PersonaComponent->TryDeleteBlock(TEXT("WitchCaptainTrauma")));
        Test->TestTrue(TEXT("Identity block preserved"), S.Player->PersonaComponent->IsBlockPresent(TEXT("Identity")));
        Test->TestTrue(TEXT("Motivation block preserved"), S.Player->PersonaComponent->IsBlockPresent(TEXT("Motivation")));
        Test->TestTrue(TEXT("SafetyDogma preserved for Rule #1 test"), S.Player->PersonaComponent->IsBlockPresent(TEXT("SafetyDogma")));

        Test->TestTrue(TEXT("Clicked ACTION! after cutting WitchCaptainTrauma"),
            ClickButtonWithCaption(Panel, TEXT("ACTION!")));
        Test->TestTrue(TEXT("ActiveTake started after breakthrough"), S.TakeSub->IsActiveTake());

        S.Player->SetActorLocation(FVector(350.0f, -345.0f, 92.0f));
        S.Player->Tick(0.02f);
        SendSimulatedKey(S.Player, EKeys::E, IE_Pressed);
    }
    else if (Step == 5)
    {
        SendSimulatedKey(S.Player, EKeys::E, IE_Released);
        Test->TestTrue(TEXT("Doorway unlocked after cutting WitchCaptainTrauma"), S.Doorway->IsDoorUnlocked());
        Test->TestEqual(TEXT("DoorwayBlocker collision disabled"),
            S.Doorway->DoorwayBlocker->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
        auto* BreakthroughBubble = Cast<UDCThoughtBubbleWidget>(S.Player->ThoughtWidget);
        Test->TestTrue(TEXT("Doorway breakthrough thought mentions Fab red heels"),
            BreakthroughBubble && BreakthroughBubble->GetThoughtText().ToString().Contains(TEXT("Fab red heels")));

        // Physically walk through left-wall doorway (+Y = CameraRight = D key)
        SendSimulatedKey(S.Player, EKeys::D, IE_Pressed);
    }
    else if (Step == 6)
    {
        SendSimulatedKey(S.Player, EKeys::D, IE_Released);
        Test->TestTrue(
            FString::Printf(TEXT("Player physically walked through doorway into interior (Y=%.1f > -260)"), S.Player->GetActorLocation().Y),
            S.Player->GetActorLocation().Y > -260.0f);

        // 6a. Conditional SafetyDogma check: Civilian refused while fires burn
        S.Player->SetActorLocation(FVector(830.0f, -95.0f, 92.0f));
        S.Player->Tick(0.02f);
        Test->TestEqual(TEXT("Civilian rescue blocked by SafetyDogma while flames burn"),
            S.Civilian->EvaluateInteraction(S.Player), EDCInteractionResult::Denied_BlockedByPersona);
        S.Player->StartInteractInput();
        S.Player->StopInteractInput();
        auto* Rule1Bubble = Cast<UDCThoughtBubbleWidget>(S.Player->ThoughtWidget);
        Test->TestTrue(TEXT("SafetyDogma thought mentions Rule #1"),
            Rule1Bubble && Rule1Bubble->GetThoughtText().ToString().Contains(TEXT("Rule #1")));

        // 6b. Smooth-pan camera check inside far right interior
        S.Player->SetActorLocation(FVector(830.0f, 180.0f, 92.0f));
        S.Camera->Tick(0.8f);
        const FVector IndoorCamLoc = S.Camera->GetActorLocation();
        Test->TestTrue(
            FString::Printf(TEXT("Smooth pan shifted camera right indoors (IndoorY=%.1f > PorchY=%.1f + 40)"),
                IndoorCamLoc.Y, GAcceptanceState.PorchCameraLocation.Y),
            IndoorCamLoc.Y > GAcceptanceState.PorchCameraLocation.Y + 40.0f);

        // 6c. No-tool thought bubble at F1
        S.Player->SetActorLocation(FVector(695.0f, -220.0f, 92.0f));
        S.Player->Tick(0.02f);
        Test->TestEqual(TEXT("Extinguish F1 without extinguisher returns Denied_MissingCapability"),
            S.FireF1->EvaluateInteraction(S.Player), EDCInteractionResult::Denied_MissingCapability);
        S.Player->StartInteractInput();
        S.Player->StopInteractInput();
        Test->TestTrue(TEXT("Debug reason records Denied_MissingCapability"),
            S.Player->ActionStateComponent->GetLastDebugReason().Contains(TEXT("Denied_MissingCapability")));
        Test->TestTrue(TEXT("ThoughtWidgetComponent visible on missing capability"),
            S.Player->ThoughtWidgetComponent->IsVisible());
        auto* MissingToolBubble = Cast<UDCThoughtBubbleWidget>(S.Player->ThoughtWidget);
        Test->TestTrue(TEXT("Missing tool thought mentions red bottle & matchstick kid"),
            MissingToolBubble
            && MissingToolBubble->GetThoughtText().ToString().Contains(TEXT("red bottle"))
            && MissingToolBubble->GetThoughtText().ToString().Contains(TEXT("matchstick kid")));
        Test->TestEqual(TEXT("F1 still Active without tool"), S.FireF1->GetCurrentFireStage(), EDCFireStage::Active);
        Test->TestTrue(TEXT("Identity intact after missing tool"), S.Player->PersonaComponent->IsBlockPresent(TEXT("Identity")));

        // 6d. MatchGirl [E] interaction near (225, 335, 0) & frame room + MatchGirl for V31_MatchGirlAndRoom.png
        S.Player->SetActorLocation(FVector(185.0f, 335.0f, 92.0f));
        S.Camera->Tick(0.5f);
        S.Player->Tick(0.02f);
        Test->TestTrue(TEXT("Nearest interactable beside MatchGirl is ADCMatchGirlActor"),
            S.Player->FindNearestAvailableInteractable() == S.MatchGirl);
        S.Player->StartInteractInput();
        S.Player->StopInteractInput();
        auto* MatchGirlBubble = Cast<UDCThoughtBubbleWidget>(S.Player->ThoughtWidget);
        Test->TestTrue(TEXT("MatchGirl [E] thought mentions roast chicken"),
            S.Player->ThoughtWidgetComponent->IsVisible()
            && MatchGirlBubble
            && MatchGirlBubble->GetThoughtText().ToString().Contains(TEXT("roast chicken")));
        Test->TestFalse(TEXT("MatchGirl talk does not pick up extinguisher"), S.Extinguisher->IsPickedUp());
        Test->TestEqual(TEXT("MatchGirl talk leaves all 3 fires active"), S.TakeSub->GetRemainingFireSpotCount(), 3);
        CheckAllVisiblePaperCards(Test, S, TEXT("Step6_MatchGirl"));
    }
    else if (Step == 7)
    {
        // 7a. ExtinguisherTraining ketchup gate + AngelPupWorkout weak escort speed (130 vs 295)
        S.TakeSub->RestartTake();
        S.Player->PersonaComponent->TryDeleteBlock(TEXT("WitchCaptainTrauma"));
        S.Player->PersonaComponent->TryDeleteBlock(TEXT("ExtinguisherTraining"));
        S.Player->PersonaComponent->TryDeleteBlock(TEXT("SafetyDogma"));
        S.Player->PersonaComponent->TryDeleteBlock(TEXT("AngelPupWorkout"));
        S.TakeSub->StartTake();
        S.Player->SetActorLocation(FVector(350.0f, -345.0f, 92.0f));
        Test->TestTrue(TEXT("Ketchup test: Doorway unlocked"), S.Doorway->TryInteract(S.Player));

        S.Player->SetActorLocation(FVector(320.0f, 220.0f, 92.0f));
        S.Player->Tick(0.02f);
        Test->TestEqual(TEXT("Extinguisher blocked without ExtinguisherTraining (Denied_MissingPersonaTag)"),
            S.Extinguisher->EvaluateInteraction(S.Player), EDCInteractionResult::Denied_MissingPersonaTag);
        S.Player->StartInteractInput();
        S.Player->StopInteractInput();
        Test->TestFalse(TEXT("Extinguisher remains unpicked without training"), S.Extinguisher->IsPickedUp());
        auto* KetchupBubble = Cast<UDCThoughtBubbleWidget>(S.Player->ThoughtWidget);
        Test->TestTrue(TEXT("Missing ExtinguisherTraining thought mentions ketchup"),
            KetchupBubble && KetchupBubble->GetThoughtText().ToString().Contains(TEXT("ketchup")));

        S.Player->SetActorLocation(FVector(830.0f, -95.0f, 92.0f));
        Test->TestTrue(TEXT("Weak escort check: start escort without AngelPupWorkout"), S.Civilian->TryInteract(S.Player));
        S.Player->Tick(0.05f);
        const float WeakSpeed = S.Player->GetCharacterMovement()->MaxWalkSpeed;
        Test->TestTrue(
            FString::Printf(TEXT("Escort speed without AngelPupWorkout ~= 130.0f (actual=%.1f)"), WeakSpeed),
            FMath::IsNearlyEqual(WeakSpeed, 130.0f, 1.0f));
        auto* NoodleBubble = Cast<UDCThoughtBubbleWidget>(S.Player->ThoughtWidget);
        Test->TestTrue(TEXT("Weak escort thought mentions noodle legs"),
            NoodleBubble && NoodleBubble->GetThoughtText().ToString().Contains(TEXT("noodle legs")));

        // 7b. Prepare F1 partial hold-E test with ExtinguisherTraining & AngelPupWorkout kept
        S.TakeSub->RestartTake();
        S.Player->PersonaComponent->TryDeleteBlock(TEXT("WitchCaptainTrauma"));
        S.TakeSub->StartTake();
        S.Player->SetActorLocation(FVector(350.0f, -345.0f, 92.0f));
        Test->TestTrue(TEXT("F1 partial test: Doorway unlocked"), S.Doorway->TryInteract(S.Player));

        S.Player->SetActorLocation(FVector(320.0f, 220.0f, 92.0f));
        S.Player->Tick(0.02f);
        S.Player->StartInteractInput();
        S.Player->StopInteractInput();
        Test->TestTrue(TEXT("Extinguisher picked up"), S.Extinguisher->IsPickedUp());
        Test->TestTrue(TEXT("Player gained Capability.ExtinguishFire"),
            S.Player->ActionStateComponent->HasCapability(DCGameplayTags::Capability_ExtinguishFire));
        Test->TestTrue(
            FString::Printf(TEXT("Base walk speed ~= 320.0f (actual=%.1f)"), S.Player->GetCharacterMovement()->MaxWalkSpeed),
            FMath::IsNearlyEqual(S.Player->GetCharacterMovement()->MaxWalkSpeed, 320.0f, 1.0f));

        // Begin partial hold-E (~0.70s out of 1.6s) on F1
        S.Player->SetActorLocation(FVector(695.0f, -220.0f, 92.0f));
        S.Player->Tick(0.02f);
        S.Player->StartInteractInput();
        Test->TestTrue(TEXT("F1 Hold interaction started"), S.FireF1->IsHoldActive());
    }
    else if (Step == 8)
    {
        // Release early (~0.70s out of 1.6s) and verify KeepStatic partial progress
        S.Player->StopInteractInput();
        Test->TestFalse(TEXT("F1 spray stopped on release"), S.FireF1->IsHoldActive());
        Test->TestEqual(TEXT("F1 still Active after partial hold"), S.FireF1->GetCurrentFireStage(), EDCFireStage::Active);

        const float Retained = S.FireF1->GetRetainedSpraySeconds();
        const UDCLevelConfigData* Config = S.TakeSub->GetActiveConfig();
        const float Required = Config ? Config->ExtinguishActionSeconds : 1.6f;
        Test->TestTrue(
            FString::Printf(TEXT("F1 RetainedSpraySeconds in (0.15, %.2f): actual=%.3f"), Required, Retained),
            Retained > 0.15f && Retained < Required);
        GAcceptanceState.SavedF1RetainedSpraySeconds = Retained;
    }
    else if (Step == 9)
    {
        // After 0.25s idle wait, verify KeepStatic retained exact progress
        Test->TestTrue(
            FString::Printf(TEXT("F1 spray progress retained after 0.25s pause (%.3f == %.3f)"),
                S.FireF1->GetRetainedSpraySeconds(), GAcceptanceState.SavedF1RetainedSpraySeconds),
            FMath::IsNearlyEqual(S.FireF1->GetRetainedSpraySeconds(), GAcceptanceState.SavedF1RetainedSpraySeconds, 0.01f));

        // Hold E again to finish F1
        S.Player->StartInteractInput();
    }
    else if (Step == 10)
    {
        S.Player->StopInteractInput();
        Test->TestEqual(TEXT("F1 Extinguished after resuming hold"), S.FireF1->GetCurrentFireStage(), EDCFireStage::Extinguished);
        Test->TestEqual(TEXT("F1 shortcut FireBlockerBox collision disabled"),
            S.FireF1->FireBlockerBox->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
        Test->TestEqual(TEXT("RemainingFireSpotCount dropped 3 -> 2"), S.TakeSub->GetRemainingFireSpotCount(), 2);
        Test->TestTrue(
            FString::Printf(TEXT("AccumulatedEngulfDelaySeconds == 3.5f (actual=%.2f)"), S.TakeSub->GetAccumulatedEngulfDelaySeconds()),
            FMath::IsNearlyEqual(S.TakeSub->GetAccumulatedEngulfDelaySeconds(), 3.5f, 0.05f));
        Test->TestTrue(
            FString::Printf(TEXT("EffectiveSecondsToEngulf == 24.0f (actual=%.2f)"), S.TakeSub->GetEffectiveSecondsToEngulf()),
            FMath::IsNearlyEqual(S.TakeSub->GetEffectiveSecondsToEngulf(), 24.0f, 0.05f));

        // Walk east (+X = CameraForward = W key) through newly unlocked F1 shortcut into Z3
        SendSimulatedKey(S.Player, EKeys::W, IE_Pressed);
    }
    else if (Step == 11)
    {
        SendSimulatedKey(S.Player, EKeys::W, IE_Released);
        Test->TestTrue(
            FString::Printf(TEXT("Player physically traversed unlocked F1 shortcut into Z3 (X=%.1f > 780)"), S.Player->GetActorLocation().X),
            S.Player->GetActorLocation().X > 780.0f);

        // Fresh retake for full S-curve 3/3 route: cut WitchCaptainTrauma + SafetyDogma, KEEP AngelPupWorkout + ExtinguisherTraining
        S.TakeSub->RestartTake();
        UDCPersonaPanelWidget* Panel = Overlay ? Overlay->GetPersonaPanel() : nullptr;
        Test->TestTrue(TEXT("3/3 Route: Cut WitchCaptainTrauma via UMG"),
            ClickCutBlockButton(Panel, TEXT("WitchCaptainTrauma")));
        Test->TestTrue(TEXT("3/3 Route: Cut SafetyDogma via UMG"),
            ClickCutBlockButton(Panel, TEXT("SafetyDogma")));
        Test->TestTrue(TEXT("3/3 Route: AngelPupWorkout preserved"),
            S.Player->PersonaComponent->IsBlockPresent(TEXT("AngelPupWorkout")));
        Test->TestTrue(TEXT("3/3 Route: Click ACTION! via UMG"),
            ClickButtonWithCaption(Panel, TEXT("ACTION!")));

        S.Player->SetActorLocation(FVector(350.0f, -345.0f, 92.0f));
        Test->TestTrue(TEXT("3/3 Route: Doorway unlocked"), S.Doorway->TryInteract(S.Player));
        S.Player->SetActorLocation(FVector(420.0f, -220.0f, 92.0f));

        // Swept S-curve route while all 3 fires active: G1 -> Z2 (west of F2) -> G2 -> Z3 (west of F3) -> Civilian
        const TArray<FVector> SCurveToCivilian = {
            FVector(601.5f, -220.0f, 92.0f),
            FVector(601.5f, 290.0f, 92.0f),
            FVector(812.0f, 290.0f, 92.0f),
            FVector(812.0f, -95.0f, 92.0f)
        };
        Test->TestTrue(TEXT("3/3 Route: Swept S-curve navigation to Civilian unobstructed"),
            MovePlayerAlongWaypointsSwept(Test, S.Player, SCurveToCivilian, TEXT("SCurve_To_Civilian")));

        Test->TestTrue(TEXT("3/3 Route: Start escorting uninjured civilian"), S.Civilian->TryInteract(S.Player));
        S.Player->Tick(0.05f);
        Test->TestTrue(
            FString::Printf(TEXT("3/3 Route: Escort speed with AngelPupWorkout ~= 295.0f (actual=%.1f)"),
                S.Player->GetCharacterMovement()->MaxWalkSpeed),
            FMath::IsNearlyEqual(S.Player->GetCharacterMovement()->MaxWalkSpeed, 295.0f, 1.0f));
        Test->TestEqual(TEXT("3/3 Route: CivilianState == EscortingUninjured"),
            S.Civilian->GetCivilianState(), EDCCivilianState::EscortingUninjured);
        Test->TestTrue(TEXT("3/3 Route: TakeSub reports civilian state frozen by escort"),
            S.TakeSub->IsCivilianStateFrozenByEscort());
        Test->TestTrue(TEXT("3/3 Route: Player gained State.EscortingCivilian"),
            S.Player->ActionStateComponent->HasCapability(DCGameplayTags::State_EscortingCivilian));
        CheckAllVisiblePaperCards(Test, S, TEXT("Step11_EscortUninjured"));

        // Advance past SecondsToInjureCivilian (9.5f) WHILE escorting: must remain frozen!
        S.TakeSub->AdvanceElapsedTime(9.9f);
        for (ADCFireHazardActor* FireSpot : S.AllFires)
        {
            FireSpot->EvaluateStageForElapsedTime(S.TakeSub->GetElapsedTakeSeconds());
        }
        Test->TestEqual(TEXT("3/3 Route: CivilianState stays FROZEN at EscortingUninjured past T_injure (9.5s)"),
            S.Civilian->GetCivilianState(), EDCCivilianState::EscortingUninjured);
        Test->TestEqual(TEXT("3/3 Route: MaxStarContainers stays 3 while escorting past T_injure"),
            S.TakeSub->GetMaxStarContainers(), 3);

        // Return along reverse S-curve into SafeZone
        const TArray<FVector> SCurveToPorch = {
            FVector(812.0f, 290.0f, 92.0f),
            FVector(601.5f, 290.0f, 92.0f),
            FVector(601.5f, -220.0f, 92.0f),
            FVector(460.0f, -220.0f, 92.0f),
            FVector(460.0f, -385.0f, 92.0f),
            FVector(440.0f, -385.0f, 92.0f)
        };
        Test->TestTrue(TEXT("3/3 Route: Swept S-curve return to SafeZone unobstructed"),
            MovePlayerAlongWaypointsSwept(Test, S.Player, SCurveToPorch, TEXT("SCurve_To_SafeZone")));
    }
    else if (Step == 12)
    {
        // SafeZoneVolume::Tick should have completed auto-rescue on arrival in Step 11
        Test->TestEqual(TEXT("3/3 Route: Civilian RescuedUninjured in SafeZone"),
            S.Civilian->GetCivilianState(), EDCCivilianState::RescuedUninjured);
        Test->TestEqual(TEXT("3/3 Route: AngelDog switches to Happy state on rescue"),
            S.AngelDog->PaperVisual ? S.AngelDog->PaperVisual->GetPaperState() : FName(TEXT("INVALID")),
            FName(TEXT("Happy")));
        Test->TestEqual(TEXT("3/3 Route: +2 stars awarded for uninjured rescue"),
            S.TakeSub->GetEarnedStars(), 2);
        Test->TestEqual(TEXT("3/3 Route: MaxStarContainers still 3"),
            S.TakeSub->GetMaxStarContainers(), 3);
        Test->TestFalse(TEXT("3/3 Route: BuddyCardMeshComp hidden after rescue"),
            S.Player->PaperVisual->BuddyCardMeshComp && S.Player->PaperVisual->BuddyCardMeshComp->IsVisible());
        Test->TestTrue(TEXT("3/3 Route: Standalone Civilian card restored & grounded in Z_Safe"),
            S.Civilian->PaperVisual && S.Civilian->PaperVisual->CardMeshComp && S.Civilian->PaperVisual->CardMeshComp->IsVisible());
        Test->TestTrue(TEXT("3/3 Route: StarPopup flashed on uninjured rescue"),
            Overlay && Overlay->GetStarPopup() && Overlay->GetStarPopup()->IsPopupVisible());

        // Pick up Extinguisher, extinguish F1 & F3, then hold E on final spot F2 for live Sky arc screenshot
        SetupAndExtinguishFireViaConfig(S, FVector(320.0f, 220.0f, 92.0f), { S.FireF1, S.FireF3 });
        Test->TestEqual(TEXT("3/3 Route: RemainingFireSpotCount == 1 (only F2 left)"),
            S.TakeSub->GetRemainingFireSpotCount(), 1);
        Test->TestTrue(TEXT("3/3 Route: Take remains active until final spot extinguished"),
            S.TakeSub->IsActiveTake());

        S.Player->SetActorLocation(FVector(602.0f, 35.0f, 92.0f));
        S.Camera->Tick(0.6f);
        S.Player->Tick(0.02f);
        S.Player->StartInteractInput();
    }
    else if (Step == 13)
    {
        // Verify live hold arc on [E] inside S-curve room BEFORE extinguishing last spot F2
        Test->TestTrue(TEXT("3/3 Route: F2 Hold active during V31_IndoorSpray3Star"), S.FireF2->IsHoldActive());
        const float HoldProgress = S.FireF2->GetHoldProgressNormalized();
        Test->TestTrue(
            FString::Printf(TEXT("3/3 Route: F2 hold progress in (0.15, 0.99): actual=%.3f"), HoldProgress),
            HoldProgress > 0.15f && HoldProgress < 0.99f);
        Test->TestTrue(TEXT("3/3 Route: InteractPromptWidget visible during F2 hold"),
            S.Player->InteractPromptWidgetComponent->IsVisible());
        auto* PromptW = Cast<UDCInteractPromptWidget>(S.Player->InteractPromptWidget);
        if (Test->TestNotNull(TEXT("3/3 Route: InteractPromptWidget cast valid"), PromptW))
        {
            Test->TestTrue(TEXT("3/3 Route: InteractPromptWidget shows HoldMode + Spraying"),
                PromptW->IsHoldMode() && PromptW->IsSpraying());
            Test->TestTrue(TEXT("3/3 Route: InteractPromptWidget HoldProgress01 > 0.15"),
                PromptW->GetHoldProgress01() > 0.15f);
        }
        CheckAllVisiblePaperCards(Test, S, TEXT("Step13_Inside3Star"));
    }
    else if (Step == 14)
    {
        // Holding continued for another 1.20s -> F2 finished naturally via hold tick!
        S.Player->StopInteractInput();

        Test->TestEqual(TEXT("3/3 Route: F2 Extinguished naturally via hold"),
            S.FireF2->GetCurrentFireStage(), EDCFireStage::Extinguished);
        Test->TestEqual(TEXT("3/3 Route: RemainingFireSpotCount == 0"), S.TakeSub->GetRemainingFireSpotCount(), 0);
        Test->TestEqual(TEXT("3/3 Route: Final EarnedStars == 3"), S.TakeSub->GetEarnedStars(), 3);
        Test->TestEqual(TEXT("3/3 Route: Final MaxStarContainers == 3"), S.TakeSub->GetMaxStarContainers(), 3);
        Test->TestTrue(TEXT("3/3 Route: Take auto-finished"), S.TakeSub->IsTakeResult());
        Test->TestEqual(TEXT("3/3 Route: EndReason == AllRemainingContainersFilled"),
            S.TakeSub->GetLastEndReason(), EDCTakeEndReason::AllRemainingContainersFilled);
        Test->TestFalse(TEXT("Thought bubble hidden at TakeResult"), S.Player->ThoughtWidgetComponent->IsVisible());

        UDCTakeResultWidget* ResultW = Overlay ? Overlay->GetResultWidget() : nullptr;
        Test->TestTrue(TEXT("Result modal visible at TakeResult"), ResultW && ResultW->IsVisible());
        if (ResultW)
        {
            const TArray<UDCJournalStarWidget*> ResultStars = CollectResultStars(ResultW);
            Test->TestEqual(TEXT("TakeResult displays 3 star containers"), ResultStars.Num(), 3);
            int32 FilledCount = 0;
            for (const UDCJournalStarWidget* StarW : ResultStars)
            {
                if (StarW && StarW->GetStarState() == EDCJournalStarState::Filled)
                {
                    ++FilledCount;
                }
            }
            Test->TestEqual(TEXT("3/3 TakeResult shows 3 Filled gold stars"), FilledCount, 3);

            TArray<FString> ResultTexts;
            TSet<const UUserWidget*> Visited;
            CollectTextsRecursive(ResultW, ResultTexts, Visited);
            const FString Joined = FString::Join(ResultTexts, TEXT(" | "));
            Test->TestTrue(TEXT("3/3 TakeResult displays ALL OUT"), Joined.Contains(TEXT("ALL OUT")));
            Test->TestTrue(TEXT("3/3 TakeResult displays SAVED UNINJURED"), Joined.Contains(TEXT("SAVED UNINJURED")));
            Test->TestTrue(TEXT("3/3 TakeResult displays 3 / 3"), Joined.Contains(TEXT("3 / 3")));
            Test->TestTrue(TEXT("3/3 TakeResult displays PERFECT DIRECTOR'S CUT review quip"),
                Joined.Contains(TEXT("PERFECT DIRECTOR'S CUT")));

            TArray<FString> BtnCaptions;
            CollectButtonCaptions(ResultW, BtnCaptions);
            Test->TestEqual(TEXT("TakeResult has ONLY 1 button ([R] Retake; no CUT! or Next Level)"),
                BtnCaptions.Num(), 1);
            if (BtnCaptions.Num() > 0)
            {
                Test->TestTrue(TEXT("TakeResult sole button is Retake"),
                    BtnCaptions[0].Contains(TEXT("Retake")));
            }
            AssertNoClapperboardStrings(Test, TEXT("Result_3Star"), Overlay);
        }
    }
    else if (Step == 15)
    {
        // Degraded Route A (2/2): Rule #1 kept (or injured) -> Escort Freeze past T_dead -> Rescue Injured + Extinguish all 3 spots
        S.TakeSub->RestartTake();
        S.Player->PersonaComponent->TryDeleteBlock(TEXT("WitchCaptainTrauma"));
        S.TakeSub->StartTake();
        S.Player->SetActorLocation(FVector(350.0f, -345.0f, 92.0f));
        Test->TestTrue(TEXT("2/2 Route: Doorway unlocked"), S.Doorway->TryInteract(S.Player));

        const UDCLevelConfigData* Config = S.TakeSub->GetActiveConfig();
        const float InjureThreshold = Config ? Config->SecondsToInjureCivilian : 9.5f;
        const float KillThreshold = Config ? Config->SecondsToKillCivilian : 19.5f;

        S.TakeSub->AdvanceElapsedTime(InjureThreshold + 0.4f);
        for (ADCFireHazardActor* FireSpot : S.AllFires)
        {
            FireSpot->EvaluateStageForElapsedTime(S.TakeSub->GetElapsedTakeSeconds());
        }
        Test->TestEqual(TEXT("2/2 Route: Civilian became Injured"),
            S.Civilian->GetCivilianState(), EDCCivilianState::Injured);
        Test->TestEqual(TEXT("2/2 Route: Star containers reduced 3 -> 2"),
            S.TakeSub->GetMaxStarContainers(), 2);

        // Extinguish fires or cut SafetyDogma? Extinguish 3 fires -> Rule #1 satisfied -> Escort Injured -> Test freeze past T_dead!
        // Note: Start escorting before Extinguish using SafetyDogma cut OR verify Rule #1 note! Actually, let's test freeing SafetyDogma for escort freeze check,
        // wait: BuildReviewNoteText checks `if (CivState == EDCCivilianState::RescuedInjured && RemainingFires <= 0)` -> returns "Textbook Rule #1!" regardless of SafetyDogma!
        // Wait: if SafetyDogma IS kept, extinguishing all 3 fires first would end take ONLY when rescued! Let's start escorting after cutting SafetyDogma cleanly in this freeze sub-check, OR extinguish 2 fires, cut? Cannot cut during ActiveTake.
        // Wait: why not extinguish all 3 fires FIRST? If all 3 fires are out while Civilian is Injured, Take does NOT end until EarnedStars (0) == MaxContainers (2), wait - extinguishing 3 fires gives 1 star! Still < 2 max containers, so SafetyDogma kept + fires out STILL allows rescue!
        SetupAndExtinguishFireViaConfig(S, FVector(320.0f, 220.0f, 92.0f), S.AllFires);
        Test->TestEqual(TEXT("2/2 Route: Still active after extinguishing fires while Injured awaits rescue"),
            S.TakeSub->IsActiveTake(), true);

        S.Player->SetActorLocation(FVector(830.0f, -95.0f, 92.0f));
        Test->TestTrue(TEXT("2/2 Route: Start escorting injured civilian after fires out"), S.Civilian->TryInteract(S.Player));
        Test->TestEqual(TEXT("2/2 Route: CivilianState == EscortingInjured"),
            S.Civilian->GetCivilianState(), EDCCivilianState::EscortingInjured);

        const float DeltaPastDead = FMath::Max(0.5f, (KillThreshold + 0.6f) - S.TakeSub->GetElapsedTakeSeconds());
        S.TakeSub->AdvanceElapsedTime(DeltaPastDead);
        for (ADCFireHazardActor* FireSpot : S.AllFires)
        {
            FireSpot->EvaluateStageForElapsedTime(S.TakeSub->GetElapsedTakeSeconds());
        }
        Test->TestEqual(TEXT("2/2 Route: EscortingInjured stays FROZEN across T_dead (19.5s)"),
            S.Civilian->GetCivilianState(), EDCCivilianState::EscortingInjured);
        Test->TestEqual(TEXT("2/2 Route: MaxStarContainers remains 2 across T_dead while escorting"),
            S.TakeSub->GetMaxStarContainers(), 2);

        // Deliver into SafeZone for SafeZoneVolume::Tick on next frame
        S.Player->SetActorLocation(FVector(420.0f, -385.0f, 92.0f));
    }
    else if (Step == 16)
    {
        Test->TestEqual(TEXT("2/2 Route: Civilian RescuedInjured"),
            S.Civilian->GetCivilianState(), EDCCivilianState::RescuedInjured);
        Test->TestEqual(TEXT("2/2 Route: AngelDog Happy on injured rescue"),
            S.AngelDog->PaperVisual ? S.AngelDog->PaperVisual->GetPaperState() : FName(TEXT("INVALID")),
            FName(TEXT("Happy")));
        Test->TestEqual(TEXT("2/2 Route: Final EarnedStars == 2"), S.TakeSub->GetEarnedStars(), 2);
        Test->TestEqual(TEXT("2/2 Route: Final MaxStarContainers == 2"), S.TakeSub->GetMaxStarContainers(), 2);
        Test->TestTrue(TEXT("2/2 Route: Auto-finished TakeResult"), S.TakeSub->IsTakeResult());
        Test->TestEqual(TEXT("2/2 Route: EndReason == AllRemainingContainersFilled"),
            S.TakeSub->GetLastEndReason(), EDCTakeEndReason::AllRemainingContainersFilled);

        if (UDCTakeResultWidget* ResultW = Overlay ? Overlay->GetResultWidget() : nullptr)
        {
            const TArray<UDCJournalStarWidget*> ResultStars = CollectResultStars(ResultW);
            int32 FilledCount = 0;
            int32 BurntCount = 0;
            for (const UDCJournalStarWidget* StarW : ResultStars)
            {
                if (StarW && StarW->GetStarState() == EDCJournalStarState::Filled) { ++FilledCount; }
                else if (StarW && StarW->GetStarState() == EDCJournalStarState::Burnt) { ++BurntCount; }
            }
            Test->TestEqual(TEXT("2/2 TakeResult shows 2 Filled stars"), FilledCount, 2);
            Test->TestEqual(TEXT("2/2 TakeResult preserves 1 Burnt star remnant"), BurntCount, 1);
            Test->TestTrue(TEXT("2/2 TakeResult shows Textbook Rule #1 note"),
                WidgetContainsText(ResultW, TEXT("Textbook Rule #1")));
        }

        // Degraded Route B (1/1): Wait past T_dead -> verify Dead non-interactable + [E] hidden -> extinguish all 3 fires -> 1/1
        const UDCLevelConfigData* Config = S.TakeSub->GetActiveConfig();
        const float KillThreshold = Config ? Config->SecondsToKillCivilian : 19.5f;
        const float EngulfThreshold = Config ? Config->SecondsToEngulfRoom : 20.5f;

        S.TakeSub->RestartTake();
        S.Player->PersonaComponent->TryDeleteBlock(TEXT("WitchCaptainTrauma"));
        S.TakeSub->StartTake();
        S.TakeSub->AdvanceElapsedTime(KillThreshold + 0.6f);
        for (ADCFireHazardActor* FireSpot : S.AllFires)
        {
            FireSpot->EvaluateStageForElapsedTime(S.TakeSub->GetElapsedTakeSeconds());
        }

        Test->TestEqual(TEXT("1/1 Route: Civilian is Dead"), S.Civilian->GetCivilianState(), EDCCivilianState::Dead);
        Test->TestEqual(TEXT("1/1 Route: MaxStarContainers reduced to 1"), S.TakeSub->GetMaxStarContainers(), 1);

        // Stand immediately east of Dead civilian (915, -95) so only Civilian is in range and F1 is out of range.
        S.Player->SetActorLocation(FVector(915.0f, -95.0f, 92.0f));
        S.Player->Tick(0.02f);
        Test->TestFalse(TEXT("1/1 Route: Dead civilian IsActionAvailable == false"),
            S.Civilian->IsActionAvailable(S.Player));
        Test->TestFalse(TEXT("1/1 Route: [E] prompt hidden next to Dead civilian"),
            S.Player->InteractPromptWidgetComponent->IsVisible());
        S.Player->StartInteractInput();
        S.Player->StopInteractInput();
        Test->TestEqual(TEXT("1/1 Route: Pressing E next to Dead civilian does nothing"),
            S.Civilian->GetCivilianState(), EDCCivilianState::Dead);
        CheckAllVisiblePaperCards(Test, S, TEXT("Step16_DeadCivilian"));

        SetupAndExtinguishFireViaConfig(S, FVector(320.0f, 220.0f, 92.0f), S.AllFires);
        Test->TestEqual(TEXT("1/1 Route: EarnedStars == 1"), S.TakeSub->GetEarnedStars(), 1);
        Test->TestEqual(TEXT("1/1 Route: MaxStarContainers == 1"), S.TakeSub->GetMaxStarContainers(), 1);
        Test->TestTrue(TEXT("1/1 Route: Take auto-finished"), S.TakeSub->IsTakeResult());
        Test->TestEqual(TEXT("1/1 Route: EndReason == AllRemainingContainersFilled"),
            S.TakeSub->GetLastEndReason(), EDCTakeEndReason::AllRemainingContainersFilled);
        if (UDCTakeResultWidget* ResultW = Overlay ? Overlay->GetResultWidget() : nullptr)
        {
            Test->TestTrue(TEXT("1/1 TakeResult displays screamed his last scream note"),
                WidgetContainsText(ResultW, TEXT("screamed his last scream")));
        }

        // Degraded Route C (0/1): RoomEngulfed with WitchCaptainTrauma still present -> 0/1
        S.TakeSub->RestartTake();
        S.TakeSub->StartTake();
        S.TakeSub->AdvanceElapsedTime(EngulfThreshold + 0.6f);
        for (ADCFireHazardActor* FireSpot : S.AllFires)
        {
            FireSpot->EvaluateStageForElapsedTime(S.TakeSub->GetElapsedTakeSeconds());
        }

        Test->TestEqual(TEXT("0/1 Route: F1 reached RoomEngulfed"), S.FireF1->GetCurrentFireStage(), EDCFireStage::RoomEngulfed);
        Test->TestEqual(TEXT("0/1 Route: Civilian is Dead"), S.Civilian->GetCivilianState(), EDCCivilianState::Dead);
        Test->TestEqual(TEXT("0/1 Route: EarnedStars == 0"), S.TakeSub->GetEarnedStars(), 0);
        Test->TestEqual(TEXT("0/1 Route: MaxStarContainers == 1"), S.TakeSub->GetMaxStarContainers(), 1);
        Test->TestTrue(TEXT("0/1 Route: Take finished"), S.TakeSub->IsTakeResult());
        Test->TestEqual(TEXT("0/1 Route: EndReason == RoomEngulfedByFire"),
            S.TakeSub->GetLastEndReason(), EDCTakeEndReason::RoomEngulfedByFire);
        if (UDCTakeResultWidget* ResultW = Overlay ? Overlay->GetResultWidget() : nullptr)
        {
            Test->TestTrue(TEXT("0/1 TakeResult displays Chief Witch's red heels won note"),
                WidgetContainsText(ResultW, TEXT("Chief Witch's red heels won")));
        }
        AssertNoClapperboardStrings(Test, TEXT("Result_0Star"), Overlay);
    }

    return true;
}

bool FDCAcceptance::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/Maps/Firefighter")));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
    ADD_LATENT_AUTOMATION_COMMAND(FDCStartFloatingPIE());
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling());
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.5f));

    for (int32 Step = 0; Step <= 16; ++Step)
    {
        ADD_LATENT_AUTOMATION_COMMAND(FDCAcceptanceStep(this, Step));

        if (Step == 0)
        {
            // Allow 0.35s strikethrough & 0.28s sheet shake on SafetyDogma cut to complete before capture.
            ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.55f));
            ADD_LATENT_AUTOMATION_COMMAND(FDCCaptureEvidenceCommand(TEXT("V31_PreTake.png")));
            ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.20f));
        }
        else if (Step == 2)
        {
            ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.20f));
            ADD_LATENT_AUTOMATION_COMMAND(FDCCaptureEvidenceCommand(TEXT("V31_PorchWitchBlocked.png")));
            ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.15f));
        }
        else if (Step == 6)
        {
            // Allow thought bubble pop-in & smooth-pan settle beside MatchGirl before capture.
            ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.45f));
            ADD_LATENT_AUTOMATION_COMMAND(FDCCaptureEvidenceCommand(TEXT("V31_MatchGirlAndRoom.png")));
            ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.15f));
        }
        else if (Step == 13)
        {
            // Live mid-spray shot showing Sky water hold ring on [E] inside S-curve room, then keep holding until F2 finishes!
            ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.15f));
            ADD_LATENT_AUTOMATION_COMMAND(FDCCaptureEvidenceCommand(TEXT("V31_IndoorSpray3Star.png")));
            ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.20f));
        }
        else if (Step == 14)
        {
            // Wait for TakeResult 0.35s pop-in animation to settle at full 1.0 scale.
            ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.45f));
            ADD_LATENT_AUTOMATION_COMMAND(FDCCaptureEvidenceCommand(TEXT("V31_Result3Star.png")));
            ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.15f));
        }
        else if (Step == 1)
        {
            ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.15f));
            ADD_LATENT_AUTOMATION_COMMAND(FDCAcceptanceStep(this, 101));
            ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.15f));
            ADD_LATENT_AUTOMATION_COMMAND(FDCAcceptanceStep(this, 102));
            ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.40f)); // ThoughtBubble 0.30s pop-in settle
        }
        else
        {
            float WaitSec = 0.15f;
            if (Step == 4) { WaitSec = 0.30f; }       // Breakthrough thought settle
            else if (Step == 5) { WaitSec = 0.75f; }  // Physical walk-through doorway
            else if (Step == 7) { WaitSec = 0.70f; }  // Partial hold-E on F1 (~44% of 1.6s)
            else if (Step == 8) { WaitSec = 0.25f; }  // KeepStatic retention pause check
            else if (Step == 9) { WaitSec = 1.80f; }  // Resume hold-E to complete F1 extinguish regardless of frame clamping
            else if (Step == 10) { WaitSec = 0.55f; } // Physical walk east through unlocked F1 shortcut
            else if (Step == 11) { WaitSec = 0.20f; } // SafeZoneVolume::Tick auto-rescue
            else if (Step == 12) { WaitSec = 0.75f; } // Live hold-E buildup on F2 (~47% of 1.6s)
            else if (Step == 15) { WaitSec = 0.20f; } // SafeZoneVolume::Tick auto-rescue on 2/2 route
            ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(WaitSec));
        }
    }

    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}

#endif
