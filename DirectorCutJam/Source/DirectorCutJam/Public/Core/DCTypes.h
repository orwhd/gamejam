#pragma once

#include "CoreMinimal.h"
#include "DCTypes.generated.h"

DIRECTORCUTJAM_API DECLARE_LOG_CATEGORY_EXTERN(LogDirectorCut, Log, All);

UENUM(BlueprintType)
enum class EDCTakePhase : uint8
{
    PreTake UMETA(DisplayName = "Pre-Take"),
    ActiveTake UMETA(DisplayName = "Active Take"),
    TakeResult UMETA(DisplayName = "Take Result")
};

UENUM(BlueprintType)
enum class EDCFireStage : uint8
{
    Active UMETA(DisplayName = "Active"),
    InjuredCivilian UMETA(DisplayName = "Injured Civilian"),
    KilledCivilian UMETA(DisplayName = "Killed Civilian"),
    RoomEngulfed UMETA(DisplayName = "Room Engulfed"),
    Extinguished UMETA(DisplayName = "Extinguished")
};

UENUM(BlueprintType)
enum class EDCCivilianState : uint8
{
    Uninjured UMETA(DisplayName = "Uninjured"),
    Injured UMETA(DisplayName = "Injured"),
    Dead UMETA(DisplayName = "Dead"),
    EscortingUninjured UMETA(DisplayName = "Escorting Uninjured"),
    EscortingInjured UMETA(DisplayName = "Escorting Injured"),
    RescuedUninjured UMETA(DisplayName = "Rescued Uninjured"),
    RescuedInjured UMETA(DisplayName = "Rescued Injured")
};

UENUM(BlueprintType)
enum class EDCStarChangeReason : uint8
{
    Initialized UMETA(DisplayName = "Initialized"),
    FireExtinguished UMETA(DisplayName = "Fire Extinguished"),
    CivilianRescuedUninjured UMETA(DisplayName = "Civilian Rescued Uninjured"),
    CivilianRescuedInjured UMETA(DisplayName = "Civilian Rescued Injured"),
    ContainerLost_CivilianInjured UMETA(DisplayName = "Container Lost (Civilian Injured)"),
    ContainerLost_CivilianKilled UMETA(DisplayName = "Container Lost (Civilian Killed)")
};

UENUM(BlueprintType)
enum class EDCTakeEndReason : uint8
{
    None UMETA(DisplayName = "None"),
    AllRemainingContainersFilled UMETA(DisplayName = "All Remaining Containers Filled"),
    RoomEngulfedByFire UMETA(DisplayName = "Room Engulfed By Fire"),
    ManualDebugEnd UMETA(DisplayName = "Manual Debug End")
};

UENUM(BlueprintType)
enum class EDCInteractionResult : uint8
{
    Allowed UMETA(DisplayName = "Allowed"),
    Denied_NotActiveTake UMETA(DisplayName = "Denied (Not Active Take)"),
    Denied_OutOfRange UMETA(DisplayName = "Denied (Out Of Range)"),
    Denied_UnavailableInScene UMETA(DisplayName = "Denied (Unavailable In Scene)"),
    Denied_MissingCapability UMETA(DisplayName = "Denied (Missing Capability)"),
    Denied_BlockedByPersona UMETA(DisplayName = "Denied (Blocked By Persona)"),
    Denied_MissingPersonaTag UMETA(DisplayName = "Denied (Missing Persona Tag)")
};

UENUM(BlueprintType)
enum class EDCInteractionInputMode : uint8
{
    InstantPress UMETA(DisplayName = "Instant Press"),
    HoldToComplete UMETA(DisplayName = "Hold To Complete")
};

// [TBD] Policy enums pending final design confirmation (08_LEVEL_01_FIREFIGHTER.md §12.2); preserved as configurable fields.
UENUM(BlueprintType)
enum class EDCExtinguishInterruptPolicy : uint8
{
    KeepStatic UMETA(DisplayName = "Keep Retained Progress Static"),
    DecayOverTime UMETA(DisplayName = "Decay Retained Progress Over Time"),
    IncreaseRequiredOnStageUp UMETA(DisplayName = "Increase Required Hold Duration On Fire Stage Up")
};

UENUM(BlueprintType)
enum class EDCEscortToolPolicy : uint8
{
    HideToolAndDisableSpray UMETA(DisplayName = "Hide Tool & Disable Spray While Escorting"),
    ShowToolAndAllowSpray UMETA(DisplayName = "Show Tool & Allow Spray While Escorting")
};

UENUM(BlueprintType)
enum class EDCActiveTakePersonaViewPolicy : uint8
{
    HiddenAndLocked UMETA(DisplayName = "Hidden & Locked During Active Take"),
    AllowTabReadOnlyPeek UMETA(DisplayName = "Allow [Tab] Read-Only Peek During Active Take")
};

UENUM(BlueprintType)
enum class EDCFireVisualMode : uint8
{
    Paper2DPlaceholder UMETA(DisplayName = "Paper2D Placeholder"),
    Stylized3DPlaceholder UMETA(DisplayName = "Stylized 3D Placeholder")
};

UENUM(BlueprintType)
enum class EDCCharacterVisualMode : uint8
{
    Paper2D UMETA(DisplayName = "2D Paper Sticker"),
    Mesh3D UMETA(DisplayName = "3D Mesh Fallback")
};

UENUM(BlueprintType)
enum class EDCDoorTraversalPolicy : uint8
{
    FreeWalkAfterBreakthrough UMETA(DisplayName = "Free Walk After Breakthrough"),
    RequireInteractEachCrossing UMETA(DisplayName = "Require Interact Each Crossing")
};

// Legacy TBD enums kept for backwards reference safety.
UENUM(BlueprintType)
enum class EDCEscortMode : uint8
{
    FollowAutoRescueAtSafeZone UMETA(DisplayName = "Follow & Auto-Rescue at Safe Zone"),
    FollowManualDropAtSafeZone UMETA(DisplayName = "Follow & Manual Drop at Safe Zone"),
    CarryDisableTools UMETA(DisplayName = "Carry (Disable Tools)")
};

UENUM(BlueprintType)
enum class EDCEscortFirePolicy : uint8
{
    VulnerableUntilSafeZone UMETA(DisplayName = "Vulnerable Until Safe Zone"),
    ImmuneOnceEscortStarts UMETA(DisplayName = "Immune Once Escort Starts")
};

UENUM(BlueprintType)
enum class EDCMissingCapabilityFeedbackPolicy : uint8
{
    HudObjectiveHint UMETA(DisplayName = "HUD Objective Hint"),
    CharacterThoughtToast UMETA(DisplayName = "Character Thought Toast"),
    Silent UMETA(DisplayName = "Silent")
};
