#include "Data/DCPersonaData.h"
#include "Core/DCGameplayTags.h"

UDCPersonaDefinition::UDCPersonaDefinition()
{
    CharacterRoleTitle = NSLOCTEXT("DirectorCut", "DefaultRoleTitle", "FIREFIGHTER");
    PageHeaderTitle = NSLOCTEXT("DirectorCut", "DefaultPageHeaderTitle", "PERSONA");
    LevelBadgeTitle = NSLOCTEXT("DirectorCut", "DefaultLevelBadgeTitle", "Fire Call");
    InitialBlocks = MakeDefaultFirefighterGrayboxBlocks();
}

TArray<FDCPersonaBlock> UDCPersonaDefinition::MakeDefaultFirefighterGrayboxBlocks()
{
    TArray<FDCPersonaBlock> Blocks;
    Blocks.Reserve(6);

    {
        FDCPersonaBlock IdentityBlock;
        IdentityBlock.BlockID = TEXT("Identity");
        IdentityBlock.DisplayText = NSLOCTEXT(
            "DirectorCut",
            "GrayboxIdentityBlock",
            "I am square-headed rookie firefighter Pipo.");
        IdentityBlock.bCanDelete = false;
        IdentityBlock.bCanReorder = false;
        IdentityBlock.bIsPresent = true;
        IdentityBlock.bEnabledInLevel = true;
        IdentityBlock.GrantedTags.AddTag(DCGameplayTags::Persona_Identity_Firefighter.GetTag());
        Blocks.Add(MoveTemp(IdentityBlock));
    }

    {
        FDCPersonaBlock MotivationBlock;
        MotivationBlock.BlockID = TEXT("Motivation");
        MotivationBlock.DisplayText = NSLOCTEXT(
            "DirectorCut",
            "GrayboxMotivationBlock",
            "Someone is screaming inside—I have to save them!");
        MotivationBlock.bCanDelete = false;
        MotivationBlock.bCanReorder = false;
        MotivationBlock.bIsPresent = true;
        MotivationBlock.bEnabledInLevel = true;
        MotivationBlock.GrantedTags.AddTag(DCGameplayTags::Persona_Motivation_SaveLives.GetTag());
        Blocks.Add(MoveTemp(MotivationBlock));
    }

    {
        FDCPersonaBlock AngelPupBlock;
        AngelPupBlock.BlockID = TEXT("AngelPupWorkout");
        AngelPupBlock.DisplayText = NSLOCTEXT(
            "DirectorCut",
            "GrayboxAngelPupWorkoutBlock",
            "Every morning I carry my chubby angel pup Coco up forty-two stairs.");
        AngelPupBlock.bCanDelete = true;
        AngelPupBlock.bCanReorder = false;
        AngelPupBlock.bIsPresent = true;
        AngelPupBlock.bEnabledInLevel = true;
        AngelPupBlock.GrantedTags.AddTag(DCGameplayTags::Persona_Trait_StrongLegs.GetTag());
        Blocks.Add(MoveTemp(AngelPupBlock));
    }

    {
        FDCPersonaBlock MemoryBlock;
        MemoryBlock.BlockID = TEXT("WitchCaptainTrauma");
        MemoryBlock.DisplayText = NSLOCTEXT(
            "DirectorCut",
            "GrayboxWitchCaptainTraumaBlock",
            "Last week I scuffed Chief Witch's red heels—she'll zap me into a frog if I disobey!");
        MemoryBlock.bCanDelete = true;
        MemoryBlock.bCanReorder = false;
        MemoryBlock.bIsPresent = true;
        MemoryBlock.bEnabledInLevel = true;
        MemoryBlock.GrantedTags.AddTag(DCGameplayTags::Persona_Memory_DisobediencePunished.GetTag());
        MemoryBlock.GrantedTags.AddTag(DCGameplayTags::Persona_Limit_FearDisobeyCaptain.GetTag());
        Blocks.Add(MoveTemp(MemoryBlock));
    }

    {
        FDCPersonaBlock ExtinguisherBlock;
        ExtinguisherBlock.BlockID = TEXT("ExtinguisherTraining");
        ExtinguisherBlock.DisplayText = NSLOCTEXT(
            "DirectorCut",
            "GrayboxExtinguisherTrainingBlock",
            "At the academy, I memorized how to spray a red foam extinguisher.");
        ExtinguisherBlock.bCanDelete = true;
        ExtinguisherBlock.bCanReorder = false;
        ExtinguisherBlock.bIsPresent = true;
        ExtinguisherBlock.bEnabledInLevel = true;
        ExtinguisherBlock.GrantedTags.AddTag(DCGameplayTags::Persona_KnowHow_UseExtinguisher.GetTag());
        Blocks.Add(MoveTemp(ExtinguisherBlock));
    }

    {
        FDCPersonaBlock SafetyDogmaBlock;
        SafetyDogmaBlock.BlockID = TEXT("SafetyDogma");
        SafetyDogmaBlock.DisplayText = NSLOCTEXT(
            "DirectorCut",
            "GrayboxSafetyDogmaBlock",
            "Academy Rule #1: NEVER hug a half-dressed victim while any flame still burns!");
        SafetyDogmaBlock.bCanDelete = true;
        SafetyDogmaBlock.bCanReorder = false;
        SafetyDogmaBlock.bIsPresent = true;
        SafetyDogmaBlock.bEnabledInLevel = true;
        SafetyDogmaBlock.GrantedTags.AddTag(DCGameplayTags::Persona_Limit_MustClearFireFirst.GetTag());
        Blocks.Add(MoveTemp(SafetyDogmaBlock));
    }

    return Blocks;
}
