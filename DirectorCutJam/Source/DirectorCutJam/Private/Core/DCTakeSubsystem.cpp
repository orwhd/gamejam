#include "Core/DCTakeSubsystem.h"
#include "Data/DCLevelConfigData.h"

DEFINE_LOG_CATEGORY(LogDirectorCut);

void UDCTakeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    EnsureFallbackConfig();
    TakeNumber = 1;
    ResetTakeStateInternal();
}

void UDCTakeSubsystem::Deinitialize()
{
    ActiveConfig = nullptr;
    FallbackConfig = nullptr;
    Super::Deinitialize();
}

UDCLevelConfigData* UDCTakeSubsystem::EnsureFallbackConfig() const
{
    if (!FallbackConfig)
    {
        FallbackConfig = NewObject<UDCLevelConfigData>(const_cast<UDCTakeSubsystem*>(this));
    }
    return FallbackConfig;
}

UDCLevelConfigData* UDCTakeSubsystem::GetActiveConfig() const
{
    return ActiveConfig ? ActiveConfig.Get() : EnsureFallbackConfig();
}

void UDCTakeSubsystem::ResetTakeStateInternal()
{
    const UDCLevelConfigData* Config = GetActiveConfig();
    Phase = EDCTakePhase::PreTake;
    EarnedStars = 0;
    MaxStarContainers = Config ? FMath::Max(1, Config->InitialStarContainers) : 3;
    ElapsedTakeSeconds = 0.0f;
    LastEndReason = EDCTakeEndReason::None;
    FireStage = EDCFireStage::Active;
    CivilianState = EDCCivilianState::Uninjured;
    InitialFireSpotCount = 0;
    RemainingFireSpotCount = 0;
    AccumulatedEngulfDelaySeconds = 0.0f;
}

void UDCTakeSubsystem::InitializeStage(UDCLevelConfigData* InConfig)
{
    ActiveConfig = InConfig ? InConfig : EnsureFallbackConfig();
    TakeNumber = 1;
    ResetTakeStateInternal();

    UE_LOG(LogDirectorCut, Display, TEXT("TakeSubsystem initialized stage (Containers=%d)."), MaxStarContainers);

    OnTakeReset.Broadcast();
    OnTakePhaseChanged.Broadcast(Phase);
    OnStarStateChanged.Broadcast(EarnedStars, MaxStarContainers, EDCStarChangeReason::Initialized);
    OnSceneStatusChanged.Broadcast(FireStage, CivilianState);
    if (InitialFireSpotCount == 0)
    {
        OnFireSpotCountChanged.Broadcast(RemainingFireSpotCount, InitialFireSpotCount);
    }
}

void UDCTakeSubsystem::StartTake()
{
    if (Phase != EDCTakePhase::PreTake)
    {
        return;
    }

    Phase = EDCTakePhase::ActiveTake;
    ElapsedTakeSeconds = 0.0f;
    LastEndReason = EDCTakeEndReason::None;

    UE_LOG(LogDirectorCut, Display, TEXT("Take #%d started (ActiveTake)."), TakeNumber);
    OnTakePhaseChanged.Broadcast(Phase);
}

void UDCTakeSubsystem::RestartTake()
{
    ++TakeNumber;
    ResetTakeStateInternal();

    UE_LOG(LogDirectorCut, Display, TEXT("Take restarted -> Take #%d (PreTake)."), TakeNumber);

    OnTakeReset.Broadcast();
    OnTakePhaseChanged.Broadcast(Phase);
    OnStarStateChanged.Broadcast(EarnedStars, MaxStarContainers, EDCStarChangeReason::Initialized);
    OnSceneStatusChanged.Broadcast(FireStage, CivilianState);
    if (InitialFireSpotCount == 0)
    {
        OnFireSpotCountChanged.Broadcast(RemainingFireSpotCount, InitialFireSpotCount);
    }
}

void UDCTakeSubsystem::FinishTake(EDCTakeEndReason Reason)
{
    if (Phase == EDCTakePhase::TakeResult)
    {
        return;
    }

    LastEndReason = Reason;
    Phase = EDCTakePhase::TakeResult;

    UE_LOG(LogDirectorCut, Display, TEXT("Take #%d finished (Reason=%d, Stars=%d/%d)."),
        TakeNumber, static_cast<int32>(Reason), EarnedStars, MaxStarContainers);

    OnTakePhaseChanged.Broadcast(Phase);
    OnTakeFinished.Broadcast(LastEndReason, EarnedStars, MaxStarContainers);
}

void UDCTakeSubsystem::AwardStars(int32 Amount, EDCStarChangeReason Reason)
{
    if (Amount <= 0 || Phase == EDCTakePhase::TakeResult)
    {
        return;
    }

    EarnedStars = FMath::Clamp(EarnedStars + Amount, 0, MaxStarContainers);
    UE_LOG(LogDirectorCut, Display, TEXT("Awarded %d star(s) (Reason=%d) -> %d/%d."),
        Amount, static_cast<int32>(Reason), EarnedStars, MaxStarContainers);

    OnStarStateChanged.Broadcast(EarnedStars, MaxStarContainers, Reason);

    if (Phase == EDCTakePhase::ActiveTake && EarnedStars >= MaxStarContainers && MaxStarContainers > 0)
    {
        FinishTake(EDCTakeEndReason::AllRemainingContainersFilled);
    }
}

void UDCTakeSubsystem::ReduceStarContainers(int32 Penalty, EDCStarChangeReason Reason)
{
    if (Penalty <= 0 || Phase == EDCTakePhase::TakeResult)
    {
        return;
    }

    const int32 OldMax = MaxStarContainers;
    MaxStarContainers = FMath::Max(0, MaxStarContainers - Penalty);
    EarnedStars = FMath::Min(EarnedStars, MaxStarContainers);

    if (MaxStarContainers != OldMax)
    {
        UE_LOG(LogDirectorCut, Display, TEXT("Reduced star containers %d -> %d (Reason=%d)."),
            OldMax, MaxStarContainers, static_cast<int32>(Reason));
        OnStarStateChanged.Broadcast(EarnedStars, MaxStarContainers, Reason);
    }

    if (Phase == EDCTakePhase::ActiveTake && EarnedStars >= MaxStarContainers && MaxStarContainers > 0)
    {
        FinishTake(EDCTakeEndReason::AllRemainingContainersFilled);
    }
}

void UDCTakeSubsystem::NotifyFireStageChanged(EDCFireStage NewStage)
{
    if (FireStage == NewStage)
    {
        return;
    }

    FireStage = NewStage;
    OnSceneStatusChanged.Broadcast(FireStage, CivilianState);

    if (FireStage == EDCFireStage::RoomEngulfed && Phase != EDCTakePhase::TakeResult)
    {
        FinishTake(EDCTakeEndReason::RoomEngulfedByFire);
    }
}

void UDCTakeSubsystem::NotifyCivilianStateChanged(EDCCivilianState NewState)
{
    if (CivilianState == NewState)
    {
        return;
    }

    CivilianState = NewState;
    OnSceneStatusChanged.Broadcast(FireStage, CivilianState);
}

void UDCTakeSubsystem::RegisterFireSpots(int32 TotalActiveSpots)
{
    const int32 ClampedCount = FMath::Max(0, TotalActiveSpots);
    InitialFireSpotCount = ClampedCount;
    RemainingFireSpotCount = ClampedCount;
    AccumulatedEngulfDelaySeconds = 0.0f;

    UE_LOG(LogDirectorCut, Display, TEXT("Registered %d active fire spot(s)."), InitialFireSpotCount);
    OnFireSpotCountChanged.Broadcast(RemainingFireSpotCount, InitialFireSpotCount);
}

void UDCTakeSubsystem::NotifySingleFireSpotExtinguished()
{
    if (Phase == EDCTakePhase::TakeResult || RemainingFireSpotCount <= 0)
    {
        return;
    }

    const UDCLevelConfigData* Config = GetActiveConfig();
    RemainingFireSpotCount = FMath::Max(0, RemainingFireSpotCount - 1);
    if (Config && Config->EngulfDelayPerSpotSeconds > 0.0f)
    {
        AccumulatedEngulfDelaySeconds += Config->EngulfDelayPerSpotSeconds;
    }

    UE_LOG(LogDirectorCut, Display, TEXT("Fire spot extinguished (%d / %d remaining, EngulfDelay=+%.2fs)."),
        RemainingFireSpotCount, InitialFireSpotCount, AccumulatedEngulfDelaySeconds);

    OnFireSpotCountChanged.Broadcast(RemainingFireSpotCount, InitialFireSpotCount);

    if (RemainingFireSpotCount == 0 && InitialFireSpotCount > 0)
    {
        FireStage = EDCFireStage::Extinguished;
        OnSceneStatusChanged.Broadcast(FireStage, CivilianState);

        const int32 FireStars = Config ? Config->StarsForExtinguish : 1;
        AwardStars(FireStars, EDCStarChangeReason::FireExtinguished);
    }
}

float UDCTakeSubsystem::GetEffectiveSecondsToEngulf() const
{
    const UDCLevelConfigData* Config = GetActiveConfig();
    const float BaseEngulfSeconds = Config ? Config->SecondsToEngulfRoom : 20.5f;
    return BaseEngulfSeconds + AccumulatedEngulfDelaySeconds;
}

bool UDCTakeSubsystem::IsCivilianStateFrozenByEscort() const
{
    return CivilianState == EDCCivilianState::EscortingUninjured
        || CivilianState == EDCCivilianState::EscortingInjured
        || CivilianState == EDCCivilianState::RescuedUninjured
        || CivilianState == EDCCivilianState::RescuedInjured;
}

void UDCTakeSubsystem::ApplyFireStageWithCivilianFatalCheck(EDCFireStage NewFireStage, int32 InjurePenalty, int32 KillPenalty)
{
    FireStage = NewFireStage;

    int32 PenaltyToApply = 0;
    if (!IsCivilianStateFrozenByEscort())
    {
        if (CivilianState == EDCCivilianState::Uninjured)
        {
            CivilianState = EDCCivilianState::Dead;
            PenaltyToApply = InjurePenalty + KillPenalty;
        }
        else if (CivilianState == EDCCivilianState::Injured)
        {
            CivilianState = EDCCivilianState::Dead;
            PenaltyToApply = KillPenalty;
        }
    }

    OnSceneStatusChanged.Broadcast(FireStage, CivilianState);

    if (PenaltyToApply > 0)
    {
        ReduceStarContainers(PenaltyToApply, EDCStarChangeReason::ContainerLost_CivilianKilled);
    }
}

void UDCTakeSubsystem::AdvanceElapsedTime(float DeltaSeconds)
{
    if (Phase != EDCTakePhase::ActiveTake || DeltaSeconds <= 0.0f)
    {
        return;
    }

    ElapsedTakeSeconds += DeltaSeconds;

    if (FireStage == EDCFireStage::Extinguished)
    {
        return;
    }

    if (InitialFireSpotCount > 0 && RemainingFireSpotCount <= 0)
    {
        return;
    }

    const UDCLevelConfigData* Config = GetActiveConfig();
    const float InjureSec = Config ? Config->SecondsToInjureCivilian : 9.5f;
    const float KillSec = Config ? Config->SecondsToKillCivilian : 19.5f;
    const float EffectiveEngulfSec = GetEffectiveSecondsToEngulf();
    const int32 InjurePenalty = Config ? Config->ContainerPenaltyOnInjured : 1;
    const int32 KillPenalty = Config ? Config->ContainerPenaltyOnKilled : 1;

    if (FireStage == EDCFireStage::Active && ElapsedTakeSeconds >= InjureSec)
    {
        FireStage = EDCFireStage::InjuredCivilian;

        bool bAppliedInjurePenalty = false;
        if (CivilianState == EDCCivilianState::Uninjured && !IsCivilianStateFrozenByEscort())
        {
            CivilianState = EDCCivilianState::Injured;
            bAppliedInjurePenalty = true;
        }

        OnSceneStatusChanged.Broadcast(FireStage, CivilianState);

        if (bAppliedInjurePenalty)
        {
            ReduceStarContainers(InjurePenalty, EDCStarChangeReason::ContainerLost_CivilianInjured);
        }

        if (Phase != EDCTakePhase::ActiveTake)
        {
            return;
        }
    }

    if ((FireStage == EDCFireStage::Active || FireStage == EDCFireStage::InjuredCivilian)
        && ElapsedTakeSeconds >= KillSec)
    {
        ApplyFireStageWithCivilianFatalCheck(EDCFireStage::KilledCivilian, InjurePenalty, KillPenalty);
        if (Phase != EDCTakePhase::ActiveTake)
        {
            return;
        }
    }

    if ((FireStage == EDCFireStage::Active || FireStage == EDCFireStage::InjuredCivilian || FireStage == EDCFireStage::KilledCivilian)
        && ElapsedTakeSeconds >= EffectiveEngulfSec)
    {
        ApplyFireStageWithCivilianFatalCheck(EDCFireStage::RoomEngulfed, InjurePenalty, KillPenalty);
        if (Phase != EDCTakePhase::TakeResult)
        {
            FinishTake(EDCTakeEndReason::RoomEngulfedByFire);
        }
    }
}
