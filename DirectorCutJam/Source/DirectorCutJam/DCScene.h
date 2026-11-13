#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "DCScene.generated.h"

class ADCPlayer;
class UStaticMeshComponent;
class UTextRenderComponent;
UENUM(BlueprintType)
enum class EDCAction : uint8 { Enter, Exit, Rescue, Extinguish };
UENUM(BlueprintType)
enum class EDCFire : uint8 { Active, Extinguished, OutOfControl };

UCLASS(Blueprintable)
class DIRECTORCUTJAM_API ADCInteraction : public AActor
{
    GENERATED_BODY()
public:
    ADCInteraction();
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Shape;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EDCAction Action = EDCAction::Enter;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Range = 230.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName BlockingEffect = TEXT("FearDisobeyCaptain");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Prompt;
    UFUNCTION(BlueprintCallable) bool TryInteract(ADCPlayer* Player);
};

// A single native actor owns the deterministic test sequence. No behavior tree or history system.
UCLASS(Blueprintable)
class DIRECTORCUTJAM_API ADCDirector : public AActor
{
    GENERATED_BODY()
public:
    ADCDirector();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TEST PLACEHOLDERS", meta=(ClampMin="1")) float CaptainFollowupSeconds = 20.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TEST PLACEHOLDERS", meta=(ClampMin="1")) float FireExpansionSeconds = 45.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TEST PLACEHOLDERS", meta=(ClampMin="1")) float TakeDurationSeconds = 120.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TEST PLACEHOLDERS") bool bAllowEditingDuringTake = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TEST PLACEHOLDERS") bool bShowMemoryReason = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TEST PLACEHOLDERS") bool bEnableDebugControls = true;
    UPROPERTY(BlueprintReadOnly) bool bCaptainOrderActive = false;
    UPROPERTY(BlueprintReadOnly) bool bRescued = false;
    UPROPERTY(BlueprintReadOnly) EDCFire Fire = EDCFire::Active;
    UPROPERTY(BlueprintReadOnly) bool bFinished = false;
    UPROPERTY(BlueprintReadOnly) bool bAwaitingTakeStart = false;
    UPROPERTY(BlueprintReadOnly) bool bFireExpanded = false;
    UPROPERTY(BlueprintReadOnly) float Elapsed = 0.f;
    UPROPERTY(BlueprintReadOnly) int32 ScriptStep = 0;
    UPROPERTY(BlueprintReadOnly) FString CaptainLine;
    UPROPERTY(BlueprintReadOnly) FString LastBlockReason;
    UPROPERTY(BlueprintReadOnly) TArray<FString> ScriptTrace;
    UPROPERTY(BlueprintReadOnly) TArray<TObjectPtr<ADCInteraction>> Interactions;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FireVisual;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> VictimVisual;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CaptainVisual;
    UFUNCTION(BlueprintCallable) void ResetTake();
    UFUNCTION(BlueprintCallable) void FinishTake();
    UFUNCTION(BlueprintPure) int32 EvaluateStars() const;
    // Three stars intentionally have no awarded path until the puzzle is designed.
    UFUNCTION(BlueprintImplementableEvent, Category="Reserved") bool QualifiesForDesignedThirdStar() const;
    UFUNCTION(BlueprintPure) ADCInteraction* NearestInteraction(const ADCPlayer* Player) const;
    UFUNCTION(BlueprintPure) bool IsActionAvailable(EDCAction Action) const;
    void AdvanceScript();
private:
    void AddScriptLine(int32 Step, const FString& Line);
};

UCLASS()
class DIRECTORCUTJAM_API ADCGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ADCGameMode();
};
