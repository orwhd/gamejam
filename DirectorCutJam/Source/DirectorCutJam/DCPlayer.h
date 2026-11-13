#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DCPlayer.generated.h"
class UDCPersona;
class ADCDirector;
class UDCOverlay;
class UDCThought;
class UWidgetComponent;

UCLASS(Blueprintable)
class DIRECTORCUTJAM_API ADCPlayer : public ACharacter
{
    GENERATED_BODY()
public:
    ADCPlayer();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UDCPersona> Persona;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> ThoughtComponent;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<ADCDirector> Director;
    UPROPERTY(BlueprintReadOnly) bool bInside = false;
    UPROPERTY(BlueprintReadOnly) bool bPersonaOpen = false;
    UPROPERTY(BlueprintReadOnly) FString ThoughtText;
    UPROPERTY(BlueprintReadOnly) float ThoughtRemaining = 0.f;
    UPROPERTY(Transient) TObjectPtr<UDCOverlay> Overlay;
    UPROPERTY(Transient) TObjectPtr<UDCThought> ThoughtWidget;
    UFUNCTION(BlueprintCallable) void Interact();
    UFUNCTION(BlueprintCallable) void TogglePersona();
    UFUNCTION(BlueprintCallable) void ShowThought(const FString& Text);
    UFUNCTION(BlueprintCallable) void RestartTake();
    UFUNCTION(BlueprintCallable) void DebugResetPersona();
    UFUNCTION(BlueprintCallable) void DebugToggleReason();
    UFUNCTION(BlueprintCallable) void DebugFinishTake();
    UFUNCTION() void HandlePersonaChanged();
private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    bool CanMove() const;
};
