#include "DCPersona.h"

UDCPersona::UDCPersona()
{
    PrimaryComponentTick.bCanEverTick = false;
    auto Add = [this](const TCHAR* ID, const TCHAR* Text, bool Deletable, FName Effect)
    {
        FDCBlock B;
        B.BlockID = ID; B.Text = FText::FromString(Text); B.bCanDelete = Deletable; B.EffectID = Effect;
        InitialBlocks.Add(B);
    };
    Add(TEXT("Identity"), TEXT("I am a firefighter."), false, NAME_None);
    Add(TEXT("Motivation"), TEXT("I always want to save people in danger."), false, NAME_None);
    Add(TEXT("PunishmentMemory"), TEXT("I once disobeyed my captain's orders and was severely punished. Since then, I have been afraid to disobey him again."), true, TEXT("FearDisobeyCaptain"));
}
void UDCPersona::BeginPlay() { Super::BeginPlay(); ResetPersona(); }
bool UDCPersona::DeleteBlock(FName ID)
{
    for (FDCBlock& B : Blocks)
        if (B.BlockID == ID && B.bActive && B.bCanDelete)
        {
            B.bActive = false;
            UE_LOG(LogTemp, Display, TEXT("DC Persona deleted: %s"), *ID.ToString());
            OnPersonaChanged.Broadcast();
            return true;
        }
    return false;
}
bool UDCPersona::QueryBlockActive(FName ID) const
{
    for (const FDCBlock& B : Blocks) if (B.BlockID == ID) return B.bActive;
    return false;
}
bool UDCPersona::HasEffect(FName ID) const
{
    if (ID.IsNone()) return false;
    for (const FDCBlock& B : Blocks) if (B.bActive && B.EffectID == ID) return true;
    return false;
}
void UDCPersona::ResetPersona() { Blocks = InitialBlocks; OnPersonaChanged.Broadcast(); }
