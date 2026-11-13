#include "DCUI.h"
#include "DCPlayer.h"
#include "DCPersona.h"
#include "DCScene.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "InputCoreTypes.h"

namespace DCUI
{
    UTextBlock* Text(UWidgetTree* Tree, const FString& Value, int32 Size=19, FLinearColor Color=FLinearColor(.9,.92,.95))
    {
        auto* T = Tree->ConstructWidget<UTextBlock>(); T->SetText(FText::FromString(Value));
        auto Font = T->GetFont(); Font.Size = Size; T->SetFont(Font);
        T->SetColorAndOpacity(FSlateColor(Color)); T->SetAutoWrapText(false);
        // Explicit wrapping gives native UMG a stable desired height on its first layout pass.
        T->SetWrapTextAt(430.f);
        return T;
    }
    void Line(UVerticalBox* Box, UWidget* W, float Padding=8)
    {
        auto* S = Box->AddChildToVerticalBox(W); S->SetPadding(FMargin(0,Padding));
    }
    UButton* Button(UWidgetTree* Tree, const FString& Caption)
    {
        auto* B = Tree->ConstructWidget<UButton>();
        B->SetBackgroundColor(FLinearColor(.13,.3,.35)); B->AddChild(Text(Tree, Caption, 18));
        return B;
    }
    UBorder* Panel(UWidgetTree* Tree, UWidget* Content)
    {
        auto* B = Tree->ConstructWidget<UBorder>(); B->SetPadding(FMargin(22));
        B->SetBrushColor(FLinearColor(.025,.035,.05,.97)); B->AddChild(Content); return B;
    }
}
void UDCDeleteButton::Build(const FString& Caption)
{
    auto* B = DCUI::Button(WidgetTree, Caption); WidgetTree->RootWidget = B;
    B->OnClicked.AddDynamic(this, &UDCDeleteButton::Delete);
}
void UDCDeleteButton::Delete()
{
    if (Player && Player->bPersonaOpen && Player->Director && !Player->Director->bFinished)
        Player->Persona->DeleteBlock(BlockID);
}
TSharedRef<SWidget> UDCOverlay::RebuildWidget()
{
    auto* Root = WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget = Root;
    auto* Info = WidgetTree->ConstructWidget<UVerticalBox>();
    DCUI::Line(Info, DCUI::Text(WidgetTree, TEXT("DIRECTOR'S CUT"), 32), 0);
    DCUI::Line(Info, DCUI::Text(WidgetTree, TEXT("FIRE SCENE  /  GAME JAM PROTOTYPE"), 13, FLinearColor(.4,.8,.8)), 3);
    Status = DCUI::Text(WidgetTree, TEXT(""), 17); DCUI::Line(Info, Status);
    auto* Edit = DCUI::Button(WidgetTree, TEXT("[Tab] Read / edit Persona"));
    Edit->OnClicked.AddDynamic(this, &UDCOverlay::TogglePersona); DCUI::Line(Info, Edit);
    auto* InfoSlot = Root->AddChildToCanvas(DCUI::Panel(WidgetTree, Info));
    InfoSlot->SetPosition(FVector2D(22,22)); InfoSlot->SetSize(FVector2D(480,315));

    auto* Bottom = WidgetTree->ConstructWidget<UVerticalBox>();
    Prompt = DCUI::Text(WidgetTree,TEXT(""),23); Prompt->SetWrapTextAt(1500); DCUI::Line(Bottom, Prompt, 1);
    auto* Controls = DCUI::Text(WidgetTree,TEXT("WASD move  |  E interact  |  Tab Persona  |  R restart"),16);
    Controls->SetWrapTextAt(1500); DCUI::Line(Bottom, Controls,4);
    Debug = DCUI::Text(WidgetTree,TEXT(""),12, FLinearColor(.55,.65,.7)); Debug->SetWrapTextAt(1600); DCUI::Line(Bottom, Debug,1);
    auto* BottomSlot = Root->AddChildToCanvas(DCUI::Panel(WidgetTree, Bottom));
    BottomSlot->SetAnchors(FAnchors(0,1,1,1)); BottomSlot->SetOffsets(FMargin(22,-145,22,125));

    auto* PersonaContent = WidgetTree->ConstructWidget<UVerticalBox>();
    DCUI::Line(PersonaContent, DCUI::Text(WidgetTree,TEXT("PERSONA / FIREFIGHTER"),24));
    DCUI::Line(PersonaContent, DCUI::Text(WidgetTree,TEXT("Delete a whole memory block. Your identity stays the same. The world and the captain's past do not change."),16));
    Blocks = WidgetTree->ConstructWidget<UVerticalBox>();
    auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>(); Scroll->AddChild(Blocks);
    auto* ScrollSlot = PersonaContent->AddChildToVerticalBox(Scroll); ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    DCUI::Line(PersonaContent, DCUI::Text(WidgetTree,TEXT("TEST: editing does not pause the take.\nRestore is a debug tool (F6), not a decided player rule."),13, FLinearColor(.9,.7,.35)));
    auto* Close = DCUI::Button(WidgetTree, TEXT("Close Persona / [Tab]"));
    Close->OnClicked.AddDynamic(this, &UDCOverlay::ClosePersona); DCUI::Line(PersonaContent, Close);
    PersonaPanel = DCUI::Panel(WidgetTree, PersonaContent);
    auto* PersonaSlot = Root->AddChildToCanvas(PersonaPanel);
    PersonaSlot->SetAnchors(FAnchors(1,0,1,1)); PersonaSlot->SetOffsets(FMargin(-510,22,488,170));

    // Explicit rows keep this initially hidden panel independent of Slate's cached text sizes.
    auto* ResultContent = WidgetTree->ConstructWidget<UCanvasPanel>();
    auto ResultRow = [ResultContent](UWidget* W, float Y, float Height)
    {
        auto* Slot = ResultContent->AddChildToCanvas(W);
        Slot->SetPosition(FVector2D(0,Y)); Slot->SetSize(FVector2D(565,Height));
    };
    ResultRow(DCUI::Text(WidgetTree,TEXT("THAT'S A TAKE"),30),0,55);
    Result = DCUI::Text(WidgetTree,TEXT("0 / 3 stars\n\nFire extinguished: NO\nCivilian rescued: NO"),22); ResultRow(Result,70,160);
    ResultRow(DCUI::Text(WidgetTree,TEXT("Three-star puzzle is not designed yet.\nTEST: default deadline is 120s. Rescue and extinguish are immediate interactions."),15),245,100);
    auto* Again = DCUI::Button(WidgetTree,TEXT("Restart original scene / [R]"));
    Again->OnClicked.AddDynamic(this, &UDCOverlay::Restart); ResultRow(Again,370,50);
    ResultPanel = DCUI::Panel(WidgetTree, ResultContent);
    auto* ResultSlot = Root->AddChildToCanvas(ResultPanel);
    ResultSlot->SetAnchors(FAnchors(.5,.5)); ResultSlot->SetAlignment(FVector2D(.5,.5));
    ResultSlot->SetSize(FVector2D(610,480));
    if (Player) Player->Persona->OnPersonaChanged.AddUniqueDynamic(this, &UDCOverlay::RefreshPersona);
    RefreshPersona(); Refresh();
    return Super::RebuildWidget();
}
void UDCOverlay::RefreshPersona()
{
    if (!Player || !Blocks) return;
    Blocks->ClearChildren();
    for (const auto& B : Player->Persona->GetBlocks())
    {
        const FString Tag = !B.bActive ? TEXT("DELETED") : B.bCanDelete ? TEXT("DELETABLE MEMORY") : TEXT("FIXED");
        DCUI::Line(Blocks, DCUI::Text(WidgetTree,Tag,12,B.bCanDelete ? FLinearColor(1,.65,.3) : FLinearColor(.45,.7,.75)),4);
        if (!B.bActive)
        {
            DCUI::Line(Blocks,DCUI::Text(WidgetTree,TEXT("[Memory removed from current Persona]"),15,FLinearColor(.45,.5,.55)),5);
            continue;
        }
        DCUI::Line(Blocks, DCUI::Text(WidgetTree, B.Text.ToString(),19),5);
        if (B.bCanDelete)
        {
            auto* Delete = CreateWidget<UDCDeleteButton>(GetOwningPlayer());
            Delete->Player = Player; Delete->BlockID = B.BlockID; Delete->Build(TEXT("Delete this memory block"));
            DCUI::Line(Blocks, Delete,8);
        }
    }
}
void UDCOverlay::NativeTick(const FGeometry& G, float Dt) { Super::NativeTick(G, Dt); Refresh(); }
FReply UDCOverlay::NativeOnPreviewKeyDown(const FGeometry& G, const FKeyEvent& E)
{
    if (E.GetKey() == EKeys::Tab) { TogglePersona(); return FReply::Handled(); }
    if (E.GetKey() == EKeys::R) { Restart(); return FReply::Handled(); }
    return Super::NativeOnPreviewKeyDown(G,E);
}
void UDCOverlay::Refresh()
{
    if (!Player || !Player->Director || !Status) return;
    auto* D = Player->Director.Get();
    const TCHAR* Fire = D->Fire == EDCFire::Extinguished ? TEXT("OUT") : D->Fire == EDCFire::OutOfControl ? TEXT("UNCONTROLLED") : TEXT("ACTIVE");
    Status->SetText(FText::FromString(FString::Printf(TEXT("Captain: \"%s\"\n\nFire: %s  |  Civilian: %s\nTest take: %.0f / %.0fs"), *D->CaptainLine, Fire, D->bRescued ? TEXT("SAFE") : TEXT("TRAPPED"),D->Elapsed,D->TakeDurationSeconds)));
    auto* Target = D->NearestInteraction(Player);
    Prompt->SetText(FText::FromString(D->bFinished ? TEXT("Take complete") : Player->bPersonaOpen ? TEXT("Read your Persona, then close the panel to act.") : Target ? FString::Printf(TEXT("[E] %s"), *Target->Prompt.ToString()) : TEXT("Approach a scene marker to interact.")));
    Debug->SetVisibility(D->bEnableDebugControls ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    Debug->SetText(FText::FromString(FString::Printf(TEXT("DEBUG | Block: %s | Fear: %s | Script: %d | Reason: %s\nF6 restore Persona  /  F7 hint reason  /  F8 end take (TEST)"),*D->LastBlockReason,Player->Persona->HasEffect(TEXT("FearDisobeyCaptain")) ? TEXT("ON") : TEXT("OFF"),D->ScriptStep,D->bShowMemoryReason ? TEXT("FULL") : TEXT("SHORT"))));
    PersonaPanel->SetVisibility(Player->bPersonaOpen && !D->bFinished ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    ResultPanel->SetVisibility(D->bFinished ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    if (D->bFinished)
        Result->SetText(FText::FromString(FString::Printf(TEXT("%d / 3 stars\n\nFire extinguished: %s\nCivilian rescued: %s"),D->EvaluateStars(),D->Fire == EDCFire::Extinguished ? TEXT("YES") : TEXT("NO"),D->bRescued ? TEXT("YES") : TEXT("NO"))));
}
void UDCOverlay::TogglePersona() { if (Player) Player->TogglePersona(); }
void UDCOverlay::ClosePersona() { if (Player && Player->bPersonaOpen) Player->TogglePersona(); }
void UDCOverlay::Restart() { if (Player) Player->RestartTake(); }

TSharedRef<SWidget> UDCThought::RebuildWidget()
{
    Text = DCUI::Text(WidgetTree,CurrentThought,17,FLinearColor(.08,.1,.13));
    Text->SetWrapTextAt(370.f);
    Text->SetJustification(ETextJustify::Center);
    auto* B = DCUI::Panel(WidgetTree,Text); B->SetBrushColor(FLinearColor(1,.95,.8)); B->SetPadding(FMargin(15,10));
    WidgetTree->RootWidget = B;
    return Super::RebuildWidget();
}
void UDCThought::SetThought(const FString& V)
{
    CurrentThought = V;
    if (Text) Text->SetText(FText::FromString(V));
}
