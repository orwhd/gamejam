# DirectorCutJam (Unreal Engine 5.8.4 — v2)

Minimal 2.5D Persona-editing prototype built in C++ on **Unreal Engine 5.8.4** (`DX12 SM5`), implementing Level 01 **"Fire Call"** with the **Retro Cute Journal** UI system and **S-curve 3-fire graybox stage**.

## Quick Play

- **Direct launch**: Run [`Play.bat`](Play.bat) (launches `/Game/Maps/Firefighter -game -windowed -ResX=1440 -ResY=900 -NoSplash`, expecting `D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe` unless `UE_EDITOR` is set), or open [`DirectorCutJam.uproject`](DirectorCutJam.uproject) and press Play (PIE).
- **Controls**:
  - **Pre-Take Journal Sheet**: Click any butter-yellow cuttable Persona sentence (e.g. `PunishmentMemory`, `Daily_A`, `Daily_B`) to strike it out, then click **`ACTION!`** (or press `Enter`/`Space`) to start the take.
  - **`W A S D`**: Camera-plane movement (single fixed perspective room camera).
  - **`E`**: Contextual interact — instant press for Doorway breakthrough, Extinguisher pickup, or Civilian combined escort; **hold `E`** next to a fire spot (`1.8s`, progress retained per spot on release) to spray and extinguish.
  - **`R`** (or top-right **`[R] Retake`** / Take Result button): Full reset back to `PreTake` with `5/5` Persona blocks restored.

## Level 01 Overview ("Fire Call")

- **Single fixed indoor perspective camera**: Positioned at `(-350, -70, 220)`, `Rotation=(-10.5, 1.5, 0)`, `FOV=64.0`, framing the enclosed room ceiling/walls/floor, open left doorway (`Y=-300`), exterior Captain + SafeZone porch, knee-height S-curve partitions (`B1`/`B2`), and all 3 fires (`F1`/`F2`/`F3`).
- **Persona Gate**: Cutting `PunishmentMemory` removes `Persona.Limit.FearDisobeyCaptain`, allowing the firefighter through the doorway (`Action.Door.Enter`). Cutting `Daily_A` or `Daily_B` has zero penalty.
- **3 Fires & Shortcut**:
  - `F1 (760, -220, 0)` seals the front shortcut gap between `B1` and `B2` until extinguished.
  - `F2 (660, 35, 0)` and `F3 (880, 160, 0)` form the indoor S-curve hazard alongside `F1`.
  - Each extinguished spot delays `RoomEngulfedByFire` (`26.0s` base + `4.5s` per spot extinguished).
- **Escort Freeze & Star Outcomes (`0–3` Stars)**:
  - Interacting (`E`) with `Civilian` switches to a single **combined escort paper card** and **freezes** Civilian injury/death timers (`T_injure = 7.2s`, `T_dead = 18.5s`) until delivered to `SafeZone`.
  - **Outcomes**: Rescue Uninjured + extinguish all 3 fires -> **`3/3`**; Rescue Injured + extinguish all 3 fires -> **`2/2`** (1 burnt star container); Dead Civilian (non-interactable) + extinguish all 3 fires -> **`1/1`**; Room Engulfed -> **`0/1`**.

## Project Layout & Docs

- `Config/` — Default map/mode (`/Game/Maps/Firefighter`, `/Script/DirectorCutJam.DCGameMode`), DX12 renderer settings, `WASD` axis bindings.
- `Scripts/` — [`create_map.py`](Scripts/create_map.py) (builds `/Game/Materials/*`, `/Game/Data/DA_Firefighter*`, and `/Game/Maps/Firefighter`).
- `Source/DirectorCutJam/` — C++ runtime (`Core/`, `Data/`, `Persona/`, `Interaction/`, `Characters/`, `Spatial/`, `Level1/`, `UI/`) and PIE acceptance suite [`DCValidation.cpp`](Source/DirectorCutJam/DCValidation.cpp).
- `Validation/` — DX12 PIE verification screenshots (`Win64_PreTake.png`, `Win64_Blocked.png`, `Win64_Inside3Star.png`, `Win64_Result.png`).
- **Design & Handoff Specs**:
  - [`../docs/07_UI_UX_SPEC.md`](../docs/07_UI_UX_SPEC.md) — Retro Cute Journal visual & UX specification.
  - [`../docs/08_LEVEL_01_FIREFIGHTER.md`](../docs/08_LEVEL_01_FIREFIGHTER.md) — Level 01 spatial design, timings, and §12 `[待决定]` switches.
  - [`../docs/09_PROGRAMMING_HANDOFF.md`](../docs/09_PROGRAMMING_HANDOFF.md) — Authoritative C++ module map, `[TBD]` DataAsset switches, Paper2D/UMG art swap slots.
  - [`VALIDATION.md`](VALIDATION.md) — Latest UE 5.8.4 Win64 build, map rebuild, and DX12 PIE test results.

## Build, Map Rebuild & Automated PIE Validation

Run from PowerShell (set `UE-LocalDataCachePath` to `D:\UE_DDC` before headless `UnrealEditor-Cmd.exe` runs):

```powershell
# 1. Compile Editor target
D:\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe `
  "D:\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" `
  DirectorCutJamEditor Win64 Development `
  -Project="d:\antigravity_projects\gamejam\DirectorCutJam\DirectorCutJam.uproject" -WaitMutex

# 2. Rebuild materials, DataAssets, and /Game/Maps/Firefighter
${env:UE-LocalDataCachePath} = 'D:\UE_DDC'
& "D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "d:\antigravity_projects\gamejam\DirectorCutJam\DirectorCutJam.uproject" `
  -ExecutePythonScript="d:\antigravity_projects\gamejam\DirectorCutJam\Scripts\create_map.py" `
  -DCRebuildMap -unattended -nop4 -nosplash -NullRHI

# 3. Run DX12 PIE acceptance suite and capture evidence screenshots
${env:UE-LocalDataCachePath} = 'D:\UE_DDC'
& "D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "d:\antigravity_projects\gamejam\DirectorCutJam\DirectorCutJam.uproject" `
  /Game/Maps/Firefighter `
  -ExecCmds="Automation RunTests DirectorCut.PIE.Acceptance; Quit" `
  -ReportExportPath="d:\antigravity_projects\gamejam\DirectorCutJam\Saved\Validation\Win64Report" `
  -DCCaptureEvidence -unattended -nop4 -nosplash
```
