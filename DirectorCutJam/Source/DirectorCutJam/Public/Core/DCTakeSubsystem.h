#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/DCTypes.h"
#include "DCTakeSubsystem.generated.h"

class UDCLevelConfigData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCTakePhaseChangedDelegate, EDCTakePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDCStarStateChangedDelegate, int32, EarnedStars, int32, MaxContainers, EDCStarChangeReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDCTakeFinishedDelegate, EDCTakeEndReason, EndReason, int32, FinalEarnedStars, int32, FinalMaxContainers);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCTakeResetDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDCSceneStatusChangedDelegate, EDCFireStage, FireStage, EDCCivilianState, CivilianState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDCFireSpotCountChangedDelegate, int32, RemainingFireSpots, int32, InitialFireSpots);

UCLASS()
class DIRECTORCUTJAM_API UDCTakeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintPure, Category="DirectorCut|Take")
    EDCTakePhase GetPhase() const { return Phase; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Take")
    bool IsPreTake() const { return Phase == EDCTakePhase::PreTake; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Take")
    bool IsActiveTake() const { return Phase == EDCTakePhase::ActiveTake; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Take")
    bool IsTakeResult() const { return Phase == EDCTakePhase::TakeResult; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Take")
    int32 GetEarnedStars() const { return EarnedStars; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Take")
    int32 GetMaxStarContainers() const { return MaxStarContainers; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Take")
    int32 GetTakeNumber() const { return TakeNumber; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Take")
    float GetElapsedTakeSeconds() const { return ElapsedTakeSeconds; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Take")
    EDCTakeEndReason GetLastEndReason() const { return LastEndReason; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Take")
    EDCFireStage GetFireStage() const { return FireStage; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Take")
    EDCCivilianState GetCivilianState() const { return CivilianState; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Take")
    UDCLevelConfigData* GetActiveConfig() const;

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Take")
    void InitializeStage(UDCLevelConfigData* InConfig);

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Take")
    void StartTake();

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Take")
    void RestartTake();

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Take")
    void FinishTake(EDCTakeEndReason Reason);

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Take")
    void AwardStars(int32 Amount, EDCStarChangeReason Reason);

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Take")
    void ReduceStarContainers(int32 Penalty, EDCStarChangeReason Reason);

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Take")
    void NotifyFireStageChanged(EDCFireStage NewStage);

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Take")
    void NotifyCivilianStateChanged(EDCCivilianState NewState);

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Take")
    void AdvanceElapsedTime(float DeltaSeconds);

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Take")
    void RegisterFireSpots(int32 TotalActiveSpots);

    UFUNCTION(BlueprintCallable, Category="DirectorCut|Take")
    void NotifySingleFireSpotExtinguished();

    UFUNCTION(BlueprintPure, Category="DirectorCut|Take")
    int32 GetInitialFireSpotCount() const { return InitialFireSpotCount; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Take")
    int32 GetRemainingFireSpotCount() const { return RemainingFireSpotCount; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Take")
    float GetAccumulatedEngulfDelaySeconds() const { return AccumulatedEngulfDelaySeconds; }

    UFUNCTION(BlueprintPure, Category="DirectorCut|Take")
    float GetEffectiveSecondsToEngulf() const;

    UFUNCTION(BlueprintPure, Category="DirectorCut|Take")
    bool IsCivilianStateFrozenByEscort() const;

    UPROPERTY(BlueprintAssignable, Category="DirectorCut|Take")
    FDCTakePhaseChangedDelegate OnTakePhaseChanged;

    UPROPERTY(BlueprintAssignable, Category="DirectorCut|Take")
    FDCStarStateChangedDelegate OnStarStateChanged;

    UPROPERTY(BlueprintAssignable, Category="DirectorCut|Take")
    FDCTakeFinishedDelegate OnTakeFinished;

    UPROPERTY(BlueprintAssignable, Category="DirectorCut|Take")
    FDCTakeResetDelegate OnTakeReset;

    UPROPERTY(BlueprintAssignable, Category="DirectorCut|Take")
    FDCSceneStatusChangedDelegate OnSceneStatusChanged;

    UPROPERTY(BlueprintAssignable, Category="DirectorCut|Take")
    FDCFireSpotCountChangedDelegate OnFireSpotCountChanged;

private:
    UDCLevelConfigData* EnsureFallbackConfig() const;
    void ResetTakeStateInternal();
    void ApplyFireStageWithCivilianFatalCheck(EDCFireStage NewFireStage, int32 InjurePenalty, int32 KillPenalty);

    UPROPERTY(Transient)
    TObjectPtr<UDCLevelConfigData> ActiveConfig;

    UPROPERTY(Transient)
    mutable TObjectPtr<UDCLevelConfigData> FallbackConfig;

    UPROPERTY(Transient)
    EDCTakePhase Phase = EDCTakePhase::PreTake;

    UPROPERTY(Transient)
    int32 EarnedStars = 0;

    UPROPERTY(Transient)
    int32 MaxStarContainers = 3;

    UPROPERTY(Transient)
    int32 TakeNumber = 1;

    UPROPERTY(Transient)
    float ElapsedTakeSeconds = 0.0f;

    UPROPERTY(Transient)
    EDCTakeEndReason LastEndReason = EDCTakeEndReason::None;

    UPROPERTY(Transient)
    EDCFireStage FireStage = EDCFireStage::Active;

    UPROPERTY(Transient)
    EDCCivilianState CivilianState = EDCCivilianState::Uninjured;

    UPROPERTY(Transient)
    int32 InitialFireSpotCount = 0;

    UPROPERTY(Transient)
    int32 RemainingFireSpotCount = 0;

    UPROPERTY(Transient)
    float AccumulatedEngulfDelaySeconds = 0.0f;
};
