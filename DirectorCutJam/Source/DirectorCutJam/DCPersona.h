#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DCPersona.generated.h"

USTRUCT(BlueprintType)
struct FDCBlock
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName BlockID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Text;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bCanDelete = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bCanReorder = false; // Reserved; no reorder gameplay.
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bActive = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName EffectID;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCPersonaChanged);

UCLASS(ClassGroup=(DirectorCut), meta=(BlueprintSpawnableComponent))
class DIRECTORCUTJAM_API UDCPersona : public UActorComponent
{
    GENERATED_BODY()
public:
    UDCPersona();
    virtual void BeginPlay() override;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Persona") TArray<FDCBlock> InitialBlocks;
    UPROPERTY(BlueprintAssignable) FDCPersonaChanged OnPersonaChanged;
    UFUNCTION(BlueprintCallable) bool DeleteBlock(FName BlockID);
    UFUNCTION(BlueprintPure) bool QueryBlockActive(FName BlockID) const;
    UFUNCTION(BlueprintPure) bool HasEffect(FName EffectID) const;
    UFUNCTION(BlueprintCallable) void ResetPersona();
    UFUNCTION(BlueprintPure) TArray<FDCBlock> GetBlocks() const { return Blocks; }
private:
    UPROPERTY(Transient) TArray<FDCBlock> Blocks;
};
