#include "Persona/DCPersonaComponent.h"
#include "Core/DCTakeSubsystem.h"
#include "Core/DCTypes.h"
#include "Data/DCLevelConfigData.h"
#include "Engine/World.h"

UDCPersonaComponent::UDCPersonaComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UDCPersonaComponent::BeginPlay()
{
    Super::BeginPlay();

    if (UWorld* World = GetWorld())
    {
        if (UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            BoundTakeSubsystem = TakeSubsystem;
            TakeSubsystem->OnTakeReset.AddDynamic(this, &UDCPersonaComponent::HandleTakeReset);
        }
    }

    InitializePersona(PersonaAsset);
}

void UDCPersonaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UDCTakeSubsystem* TakeSubsystem = BoundTakeSubsystem.Get())
    {
        TakeSubsystem->OnTakeReset.RemoveDynamic(this, &UDCPersonaComponent::HandleTakeReset);
    }
    BoundTakeSubsystem.Reset();

    Super::EndPlay(EndPlayReason);
}

void UDCPersonaComponent::HandleTakeReset()
{
    ResetPersona();
}

const UDCPersonaDefinition* UDCPersonaComponent::ResolveEffectiveDefinition() const
{
    if (PersonaAsset)
    {
        return PersonaAsset.Get();
    }

    if (const UWorld* World = GetWorld())
    {
        if (const UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            if (const UDCLevelConfigData* Config = TakeSubsystem->GetActiveConfig())
            {
                if (Config->DefaultPersonaAsset)
                {
                    return Config->DefaultPersonaAsset.Get();
                }
            }
        }
    }

    return nullptr;
}

void UDCPersonaComponent::InitializePersona(UDCPersonaDefinition* InAsset)
{
    if (InAsset != nullptr)
    {
        PersonaAsset = InAsset;
    }

    ResetPersona();
}

void UDCPersonaComponent::ResetPersona()
{
    const UDCPersonaDefinition* Def = ResolveEffectiveDefinition();
    const TArray<FDCPersonaBlock> SourceBlocks = (OverrideBlocks.Num() > 0)
        ? OverrideBlocks
        : ((Def && Def->InitialBlocks.Num() > 0)
            ? Def->InitialBlocks
            : UDCPersonaDefinition::MakeDefaultFirefighterGrayboxBlocks());

    const UDCLevelConfigData* Config = nullptr;
    if (const UWorld* World = GetWorld())
    {
        if (const UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            Config = TakeSubsystem->GetActiveConfig();
        }
    }

    ActiveBlocks.Reset();
    ActiveBlocks.Reserve(SourceBlocks.Num());
    for (const FDCPersonaBlock& Block : SourceBlocks)
    {
        if (!Block.bEnabledInLevel)
        {
            continue;
        }
        if (Block.BlockID == TEXT("Daily_B") && Config && !Config->bEnableOptionalDailyBlockB)
        {
            continue;
        }
        ActiveBlocks.Add(Block);
    }

    RebuildActiveTags();
    OnPersonaChanged.Broadcast();
}

bool UDCPersonaComponent::CanEditNow() const
{
    const UWorld* World = GetWorld();
    const UDCTakeSubsystem* TakeSubsystem = World ? World->GetSubsystem<UDCTakeSubsystem>() : nullptr;
    return TakeSubsystem ? TakeSubsystem->IsPreTake() : false;
}

bool UDCPersonaComponent::TryDeleteBlock(FName BlockID)
{
    if (!CanEditNow())
    {
        return false;
    }

    for (FDCPersonaBlock& Block : ActiveBlocks)
    {
        if (Block.BlockID == BlockID && Block.bCanDelete && Block.bIsPresent)
        {
            Block.bIsPresent = false;
            RebuildActiveTags();

            UE_LOG(LogDirectorCut, Display, TEXT("Persona block deleted: %s"), *BlockID.ToString());
            OnPersonaChanged.Broadcast();
            return true;
        }
    }

    return false;
}

bool UDCPersonaComponent::IsBlockPresent(FName BlockID) const
{
    for (const FDCPersonaBlock& Block : ActiveBlocks)
    {
        if (Block.BlockID == BlockID)
        {
            return Block.bIsPresent;
        }
    }
    return false;
}

bool UDCPersonaComponent::HasPersonaTag(FGameplayTag Tag) const
{
    return Tag.IsValid() && ActivePersonaTags.HasTag(Tag);
}

bool UDCPersonaComponent::HasAnyPersonaTags(const FGameplayTagContainer& Tags) const
{
    return !Tags.IsEmpty() && ActivePersonaTags.HasAny(Tags);
}

bool UDCPersonaComponent::HasAllPersonaTags(const FGameplayTagContainer& Tags) const
{
    return ActivePersonaTags.HasAll(Tags);
}

FText UDCPersonaComponent::GetCharacterRoleTitle() const
{
    if (const UDCPersonaDefinition* Def = ResolveEffectiveDefinition())
    {
        if (!Def->CharacterRoleTitle.IsEmpty())
        {
            return Def->CharacterRoleTitle;
        }
    }

    return NSLOCTEXT("DirectorCut", "DefaultRoleTitle", "FIREFIGHTER");
}

FText UDCPersonaComponent::GetPageHeaderTitle() const
{
    if (const UDCPersonaDefinition* Def = ResolveEffectiveDefinition())
    {
        if (!Def->PageHeaderTitle.IsEmpty())
        {
            return Def->PageHeaderTitle;
        }
    }

    return NSLOCTEXT("DirectorCut", "DefaultPageHeaderTitle", "PERSONA");
}

FText UDCPersonaComponent::GetLevelBadgeTitle() const
{
    if (const UDCPersonaDefinition* Def = ResolveEffectiveDefinition())
    {
        if (!Def->LevelBadgeTitle.IsEmpty())
        {
            return Def->LevelBadgeTitle;
        }
    }

    if (const UWorld* World = GetWorld())
    {
        if (const UDCTakeSubsystem* TakeSubsystem = World->GetSubsystem<UDCTakeSubsystem>())
        {
            if (const UDCLevelConfigData* Config = TakeSubsystem->GetActiveConfig())
            {
                if (!Config->LevelDisplayName.IsEmpty())
                {
                    return Config->LevelDisplayName;
                }
            }
        }
    }

    return NSLOCTEXT("DirectorCut", "DefaultLevelBadgeTitle", "Fire Call");
}

void UDCPersonaComponent::RebuildActiveTags()
{
    ActivePersonaTags.Reset();

    for (const FDCPersonaBlock& Block : ActiveBlocks)
    {
        if (Block.bIsPresent)
        {
            ActivePersonaTags.AppendTags(Block.GrantedTags);
        }
    }
}
