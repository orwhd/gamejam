# DirectorCutJam — Win64 DX12 Validation Record (v2)

## Environment

- **OS**: Windows 11 (`25H2`, Build `10.0.26200.9974`)
- **Engine**: Unreal Engine `5.8.4` (`DirectorCutJam.uproject`)
- **RHI / GPU**: `D3D12 (SM5)` / `GameViewer Virtual Display Adapter`
- **CPU / RAM**: `AMD Ryzen 7 8845H w/ Radeon 780M Graphics` / `32 GB`
- **DDC Path**: `UE-LocalDataCachePath = D:\UE_DDC`

---

## Latest Verified Runs (2026-11-14)

### 1. C++ Compilation (`DirectorCutJamEditor Win64 Development`)

```powershell
D:\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe `
  "D:\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" `
  DirectorCutJamEditor Win64 Development `
  -Project="d:\antigravity_projects\gamejam\DirectorCutJam\DirectorCutJam.uproject" -WaitMutex
```

- **Result**: `Succeeded` (`7/7` actions, `27.50 seconds`, exit code `0`, building final polish changes across `DCPlayerCharacter.cpp`, `DCFirefighterActors.cpp`, `DCHudOverlayWidget.cpp`, `DCStarPopupWidget.cpp`, `DCValidation.cpp`, and linking `UnrealEditor-DirectorCutJam.dll`).

### 2. Asset & Map Rebuild (`Scripts/create_map.py`)

```powershell
${env:UE-LocalDataCachePath} = 'D:\UE_DDC'
& "D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "d:\antigravity_projects\gamejam\DirectorCutJam\DirectorCutJam.uproject" `
  -ExecutePythonScript="d:\antigravity_projects\gamejam\DirectorCutJam\Scripts\create_map.py" `
  -DCRebuildMap -unattended -nop4 -nosplash -NullRHI
```

- **Result**: Exit code `0`; logged `LogPython: DC_MAP_CREATED /Game/Maps/Firefighter` (`Saved/Logs/DirectorCutJam-backup-2026.11.14-13.20.06.log` line `2219`, `2026.11.14-13.20.00`, rotated after the subsequent `13.22.55` PIE run) with `0` Python errors; regenerated `/Game/Materials/*` (including matte `M_Partition` `#6E7887`, `M_DoorFrame`, `M_Porch`, `M_Fire`, and `M_PaperMasked` with `FlipU` + 6 `MI_Paper_*` instances), `/Game/Data/DA_FirefighterPersona`, `/Game/Data/DA_FirefighterLevelConfig`, and all v2 S-curve actors in `/Game/Maps/Firefighter`.

### 3. Live DX12 PIE Acceptance (`DirectorCut.PIE.Acceptance`)

```powershell
${env:UE-LocalDataCachePath} = 'D:\UE_DDC'
& "D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "d:\antigravity_projects\gamejam\DirectorCutJam\DirectorCutJam.uproject" `
  /Game/Maps/Firefighter `
  -ExecCmds="Automation RunTests DirectorCut.PIE.Acceptance; Quit" `
  -ReportExportPath="d:\antigravity_projects\gamejam\DirectorCutJam\Saved\Validation\Win64Report" `
  -DCCaptureEvidence -unattended -nop4 -nosplash
```

- **JSON Report**: [`Saved/Validation/Win64Report/index.json`](Saved/Validation/Win64Report/index.json)
  - `reportCreatedOn`: `2026.11.14-13.22.55` (`dateTime`: `2026.11.14-13.22.31`)
  - `totalDuration`: `24.256484985351562` seconds
  - `state`: `Success` (`succeeded: 1`, `succeededWithWarnings: 0`, `failed: 0`, `notRun: 0`, `warnings: 0`, `errors: 0`)
  - **Engine Log**: `Saved/Logs/DirectorCutJam.log` line `2311` confirms `Display: Test Completed. Result={Success} Name={Acceptance} Path={DirectorCut.PIE.Acceptance}`.

> Note: UE 5.8 prints baseline engine `PreInit` `UnifiedErrorTest` diagnostics at editor startup prior to running `Automation RunTests`; those belong to engine bootstrap, not `DirectorCut.*` tests, and `Win64Report/index.json` records `0` warnings and `0` errors for `DirectorCut.PIE.Acceptance`.

---

## Automated Coverage (`DCValidation.cpp`)

`DirectorCut.PIE.Acceptance` exercises an end-to-end 18-step (`Step 0–17`) PIE flow directly against [`/Game/Maps/Firefighter`](Scripts/create_map.py):

1. **Pre-Take Retro Cute Journal & No-Clapperboard Audit**:
   - Verifies `5` Persona blocks loaded (`Identity`, `Motivation`, `Daily_A`, `PunishmentMemory`, `Daily_B`), opaque journal backdrop covering viewport, clicking harmless `Daily_A` strikes it out (`bIsPresent == false`) while `PunishmentMemory` remains active, and recursive UMG text audit confirms zero clapperboard/studio strings (`"DIRECTOR'S CUT"`, `"SCENE 01"`, `"TAKE #"`, `"CUT!"`). Captures `Win64_PreTake.png`.
2. **Blocked Doorway, Thought Bubble & Full `[R]` Retake**:
   - Walks player to doorway `(350, -340)`, triggers `E` -> verifies `Denied_BlockedByPersona`, `UDCThoughtBubbleWidget` visible, `[E] ENTER` prompt visible, and captures `Win64_Blocked.png`.
   - Triggers `[R]` (`RestartTakeAction`) -> verifies complete reset back to `PreTake` with `5/5` blocks present and no clapperboard UI.
3. **Breakthrough, Missing Tool Thought Bubble, Hold-`E` Retention & Shortcut Unlock**:
   - Cuts `PunishmentMemory`, clicks `ACTION!`, enters through doorway, verifies missing-tool `E` on `F1` triggers `Denied_MissingCapability` thought bubble, picks up Extinguisher `T` at `(320, 270)` (`Capability.ExtinguishFire`).
   - Tests partial hold-`E` (`~0.9s` via `StartInteractInput`/`StopInteractInput`) on `F1` -> verifies `0.15s < RetainedSpraySeconds < 1.8s`, waits `0.2s` -> verifies `KeepStatic` retention, resumes hold-`E` to finish `F1` -> verifies `FireBlockerBox` shortcut collision disabled, remaining fires `= 2`, `AccumulatedEngulfDelaySeconds == 4.5s`.
4. **S-Curve Rescue, Escort Freeze & `3/3` Star Finish**:
   - After fresh `[R]` retake and doorway breakthrough, traverses S-curve (`G1 -> Z2 -> G2 -> Z3`) to Civilian `(885, -95)`, presses `E` -> verifies standalone Civilian card hidden and player switched to `CombinedEscortUninjured`, advances past `T_injure = 7.2s` -> verifies `CivilianState` remains **frozen** (`EscortingUninjured`) and `MaxStarContainers == 3`.
   - Enters `SafeZone` (`+2` stars, Civilian parked beside Captain), picks up Extinguisher, captures `Win64_Inside3Star.png` mid-hold on `F2` inside the room (live Sky water hold ring), finishes `F2` and `F3` -> verifies `AllRemainingContainersFilled` with `3/3` stars on Take Result and captures `Win64_Result.png`.
5. **Degraded Outcomes & Dead Non-Interactable**:
   - **Injured (`2/2`)**: Crosses `T_injure` (`7.2s`) unescorted (`MaxStarContainers == 2`), initiates `EscortingInjured`, crosses `T_dead` (`18.5s`) proving frozen injured escort does not die, rescues and extinguishes all 3 fires -> `2/2` (preserving 1 burnt star remnant).
   - **Dead (`1/1`)**: Crosses `T_dead` (`18.5s`) unescorted -> `Dead`, `MaxStarContainers == 1`; verifies `IsActionAvailable == false`, `[E]` hidden, and pressing `E` next to dead Civilian does nothing; extinguishes all fires -> `1/1`.
   - **Room Engulfed (`0/1`)**: Crosses `T_engulf` (`26.0s`) -> `RoomEngulfedByFire` (`0/1`).
6. **2.5D Upright & Grounded Card Invariant**:
   - Across active steps, checks every visible `UDCPaperVisualComponent` (including player `CardMeshComp` and `BuddyCardMeshComp`) maintains `Pitch == 0`, `Roll == 0`, and Bottom-Center grounded at `Z == 0` within tolerance.

---

## Visual Inspection Summary (Win64 DX12 Evidence)

All 4 captured screenshots under [`Validation/`](Validation/) were visually inspected:

| Evidence File | Visual Verification Result |
|---|---|
| [`Validation/Win64_PreTake.png`](Validation/Win64_PreTake.png) | **Pass** — Opaque cocoa desk backdrop hiding room; cream journal sheet with 4 angled pastel corner tapes; `PERSONA` + `Fire Call` pill; dashed rule; 5 blocks (`Identity`, `Motivation`, `Daily_A` with ink strikethrough, `PunishmentMemory` & `Daily_B` in butter-gold with scissors); bottom-left legend (`FIXED / CUTTABLE / CUT`); bottom-right `ACTION!` button; zero clapperboard elements. |
| [`Validation/Win64_Blocked.png`](Validation/Win64_Blocked.png) | **Pass** — Single perspective cutaway room displaying ceiling, `B1/B2` slate-taupe partitions, 3 active orange/amber fire spots (`F1/F2/F3`), red/pink Extinguisher at front-right, cyan/mint Civilian in `Z3`, Captain & green SafeZone on porch, `[E] ENTER` prompt at chest/door-handle height (`Z = 130`), dot-tail thought bubble (`"I must obey the Captain's orders... / last time I didn't, it never ended well."`) sitting cleanly above the doorway lintel (`Z = 142`, zero overlap with `[E]`), top-right `[R] Retake` pill, and bottom lock hint pill. |
| [`Validation/Win64_Inside3Star.png`](Validation/Win64_Inside3Star.png) | **Pass** — Top-center star pill displaying `2` filled gold stars + `1` empty star container; rescued Civilian parked cleanly on SafeZone beside Captain; Player standing at `F2` behind `B1` with live Sky-blue clockwise water hold progress arc around `[E] EXTINGUISH`; top-right `[R] Retake` pill visible. |
| [`Validation/Win64_Result.png`](Validation/Win64_Result.png) | **Pass** — Clean Retro Cute Journal Take Result modal with 2 top corner tapes, `3` gold filled stars, dashed dividers, `FIRES -> ALL OUT`, `CIVILIAN -> SAVED UNINJURED`, `STARS -> 3 / 3`, and sole `[R] Retake` button (no duplicate top-center `StarPopup`, no `CUT!`, no Next Level). |
