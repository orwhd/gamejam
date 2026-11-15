#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "DCPersonaData.generated.h"

USTRUCT(BlueprintType)
struct DIRECTORCUTJAM_API FDCPersonaBlock
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Persona")
    FName BlockID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Persona", meta=(MultiLine=true))
    FText DisplayText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Persona")
    bool bCanDelete = false;

    // Reserved expansion field; reordering gameplay is disabled in the graybox prototype.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Persona")
    bool bCanReorder = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Persona")
    bool bIsPresent = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Persona")
    bool bEnabledInLevel = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Persona")
    FGameplayTagContainer GrantedTags;
};

UCLASS(BlueprintType)
class DIRECTORCUTJAM_API UDCPersonaDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UDCPersonaDefinition();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Persona")
    FText CharacterRoleTitle;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Persona")
    FText PageHeaderTitle;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Persona")
    FText LevelBadgeTitle;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Persona")
    TArray<FDCPersonaBlock> InitialBlocks;

    UFUNCTION(BlueprintCallable, Category="Persona")
    static TArray<FDCPersonaBlock> MakeDefaultFirefighterGrayboxBlocks();
};
