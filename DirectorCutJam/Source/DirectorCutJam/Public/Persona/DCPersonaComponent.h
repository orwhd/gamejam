#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Data/DCPersonaData.h"
#include "DCPersonaComponent.generated.h"

class UDCTakeSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCPersonaChangedDelegate);

UCLASS(ClassGroup=(DirectorCut), meta=(BlueprintSpawnableComponent))
class DIRECTORCUTJAM_API UDCPersonaComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDCPersonaComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Persona")
    TObjectPtr<UDCPersonaDefinition> PersonaAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Persona")
    TArray<FDCPersonaBlock> OverrideBlocks;

    UPROPERTY(BlueprintAssignable, Category="Persona")
    FDCPersonaChangedDelegate OnPersonaChanged;

    UFUNCTION(BlueprintCallable, Category="Persona")
    void InitializePersona(UDCPersonaDefinition* InAsset = nullptr);

    UFUNCTION(BlueprintCallable, Category="Persona")
    void ResetPersona();

    UFUNCTION(BlueprintPure, Category="Persona")
    bool CanEditNow() const;

    UFUNCTION(BlueprintCallable, Category="Persona")
    bool TryDeleteBlock(FName BlockID);

    UFUNCTION(BlueprintPure, Category="Persona")
    const TArray<FDCPersonaBlock>& GetBlocks() const { return ActiveBlocks; }

    UFUNCTION(BlueprintPure, Category="Persona")
    bool IsBlockPresent(FName BlockID) const;

    UFUNCTION(BlueprintPure, Category="Persona")
    const FGameplayTagContainer& GetActivePersonaTags() const { return ActivePersonaTags; }

    UFUNCTION(BlueprintPure, Category="Persona")
    bool HasPersonaTag(FGameplayTag Tag) const;

    UFUNCTION(BlueprintPure, Category="Persona")
    bool HasAnyPersonaTags(const FGameplayTagContainer& Tags) const;

    UFUNCTION(BlueprintPure, Category="Persona")
    bool HasAllPersonaTags(const FGameplayTagContainer& Tags) const;

    UFUNCTION(BlueprintPure, Category="Persona")
    FText GetCharacterRoleTitle() const;

    UFUNCTION(BlueprintPure, Category="Persona")
    FText GetPageHeaderTitle() const;

    UFUNCTION(BlueprintPure, Category="Persona")
    FText GetLevelBadgeTitle() const;

    UFUNCTION(BlueprintPure, Category="Persona")
    UDCPersonaDefinition* GetPersonaAsset() const { return PersonaAsset.Get(); }

private:
    UFUNCTION()
    void HandleTakeReset();

    void RebuildActiveTags();
    const UDCPersonaDefinition* ResolveEffectiveDefinition() const;

    UPROPERTY(Transient)
    TArray<FDCPersonaBlock> ActiveBlocks;

    UPROPERTY(Transient)
    FGameplayTagContainer ActivePersonaTags;

    TWeakObjectPtr<UDCTakeSubsystem> BoundTakeSubsystem;
};
