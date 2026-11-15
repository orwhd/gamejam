# 09. 编程交付与集成说明（`DirectorCutJam` v2 工程现状快照）

> [!IMPORTANT]
> **v3.1 设计改革与特色美术融合交接提示（交接给 `编程 Agent`）**：本文档记录的是早期 v2 代码实现快照。设计文档体系（[`AGENTS.md`](../AGENTS.md) 与 [`01_GDD_CORE.md`](01_GDD_CORE.md) ～ [`08_LEVEL_01_FIREFIGHTER.md`](08_LEVEL_01_FIREFIGHTER.md)）现已全面升级至 **v3.1 特色手绘资产融合冲奖版**：
> 1. **5 大仓库手绘资产全员入戏**：方脸消防员（`firefighter.png` + 拆层天蓝水花）、门外拦路红高跟魔女队长（`witch.png`）、深处爱心裤衩尖叫哥（`rescue.png` + 拆层红黄蜡笔火焰）、右前角纵火彩蛋卖火柴想烤鸡的小女孩（`little girl.png`）、门外安全区迎宾天使光环修狗（`dog.png`），统一采用「模切白边剪纸贴纸 + 单图程序化蹦跶」管线，并保留主角 `VisualMode (2D_Paper | 3D_Mesh)` 切换开关；
> 2. **6 词条双刃剑失忆谜题**：新增抱修狗腿力（`StrongLegs`）、学院灭火器知识（忘掉会把灭火器当小女孩的烤鸡番茄酱瓶）、死板救援铁律（`MustClearFireFirst`）及娃娃屋 `X/Y` 平移跟随相机。
> 后续执行代码实现时，请直接对照 **[`08_LEVEL_01_FIREFIGHTER.md` §7（v2 → v3.1 完整落地清单）](08_LEVEL_01_FIREFIGHTER.md)** 与 **[`05_ASSET_PIPELINE.md`](05_ASSET_PIPELINE.md)** 实施。

> 本文档为 **Unreal Engine 5.8.4** C++ 工程 [`DirectorCutJam`](../DirectorCutJam/DirectorCutJam.uproject) 的 v2 工程架构、模块索引与美术接入底板说明。

---

## 1. 当前交付状态总览（v2）

- **引擎与渲染**：Unreal Engine `5.8.4`（Win64 默认 `DefaultGraphicsRHI_DX12`）。
- **已验证状态**：
  - `DirectorCutJamEditor Win64 Development` 编译通过（UBT 退出码 `0`）。
  - [`create_map.py`](../DirectorCutJam/Scripts/create_map.py) 无头重建全部 `.uasset` 材质/数据资产与 `/Game/Maps/Firefighter` S 形三火点关卡地图成功（`DC_MAP_CREATED /Game/Maps/Firefighter`，零报错）。
  - Live DX12 PIE 自动化验收 `DirectorCut.PIE.Acceptance` 全量通过（`1/1 Success`，`0 errors`，`0 warnings`，耗时 `24.256485s`），并已人工目视检查全部 4 张实机截图（`Win64_PreTake.png`、`Win64_Blocked.png`、`Win64_Inside3Star.png`、`Win64_Result.png`）。
- **核心设计对齐**：
  - **Pre-Take 剧本页**：全屏奶油色手账风格（Retro Cute Journal），彻底去除场记板、`DIRECTOR'S CUT`、`SCENE/TAKE`、`CUT!` 等摄影棚元素；点击可删词条即划粗线保留原文，点击底部 `ACTION!` 进入实机。
  - **Active Take 镜头与空间**：室内单固定透视机位（`(-350, -70, 220)`，`Rotation=(-10.5, 1.5, 0)`，`FOV=64.0`），封闭天花板 + 左侧敞开门口同屏可见门外队长与安全区，室内由两道低矮隔断 `B1/B2` 与三处阻塞火点 `F1/F2/F3` 构成 S 形通道。
  - **护送与灭火机制**：靠近受困者按 `E` 立即转为**单张合体护送纸片**并冻结受困者伤势计时；持灭火器对火点按住 `E` 呈现水蓝环形读条、中断保留进度；扑灭 `F1` 解锁 `B1/B2` 间捷径碰撞并延缓全屋火满时间。

---

## 2. 工程模块与源码地图

工程位于 [`DirectorCutJam/`](../DirectorCutJam/)，仅依赖单运行时模块 `DirectorCutJam`（[`DirectorCutJam.Build.cs`](../DirectorCutJam/Source/DirectorCutJam/DirectorCutJam.Build.cs)）。

- **启用插件**（[`DirectorCutJam.uproject`](../DirectorCutJam/DirectorCutJam.uproject)）：`Paper2D`（Runtime）、`PythonScriptPlugin`（Editor）、`EditorScriptingUtilities`（Editor）。
- **模块依赖**：`Core`、`CoreUObject`、`Engine`、`InputCore`、`GameplayTags`、`Paper2D`、`UMG`、`Slate`、`SlateCore`（Editor 附加 `UnrealEd`）。

| 子目录 / 文件 | 核心类 / 职责 | 源码路径 |
|---|---|---|
| **Core** | 阶段/结果/火势/受困者与 `[TBD]` 策略枚举、12 个 Native GameplayTags、权威单局状态机 `UDCTakeSubsystem` | [`DCTypes.h`](../DirectorCutJam/Source/DirectorCutJam/Public/Core/DCTypes.h)、[`DCGameplayTags.h`](../DirectorCutJam/Source/DirectorCutJam/Public/Core/DCGameplayTags.h)、[`DCTakeSubsystem.h`](../DirectorCutJam/Source/DirectorCutJam/Public/Core/DCTakeSubsystem.h) |
| **Data** | 人格词条资产 `UDCPersonaDefinition`（`MakeDefaultFirefighterGrayboxBlocks`）与关卡配置资产 `UDCLevelConfigData` | [`DCPersonaData.h`](../DirectorCutJam/Source/DirectorCutJam/Public/Data/DCPersonaData.h)、[`DCLevelConfigData.h`](../DirectorCutJam/Source/DirectorCutJam/Public/Data/DCLevelConfigData.h) |
| **Persona** | 主角挂载组件 `UDCPersonaComponent`（管理 5 条词条、Tag 重建、Pre-Take 可编辑开关、`OnPersonaChanged`） | [`DCPersonaComponent.h`](../DirectorCutJam/Source/DirectorCutJam/Public/Persona/DCPersonaComponent.h) |
| **Interaction** | 交互规则基类 `ADCInteractableBase`（`FDCInteractionRule`：Instant/Hold、Persona/Capability Gate）与玩家状态组件 `UDCActionStateComponent` | [`DCInteractableBase.h`](../DirectorCutJam/Source/DirectorCutJam/Public/Interaction/DCInteractableBase.h)、[`DCActionStateComponent.h`](../DirectorCutJam/Source/DirectorCutJam/Public/Interaction/DCActionStateComponent.h) |
| **Characters** | 2.5D 纸片主角 `ADCPlayerCharacter`（相机平面 WASD、`E` 按住/松开、思想气泡与交互提示挂点、合体纸片/工具状态同步） | [`DCPlayerCharacter.h`](../DirectorCutJam/Source/DirectorCutJam/Public/Characters/DCPlayerCharacter.h) |
| **Spatial** | 固定舞台机位 `ADCFixedStageCamera` 与 Bottom-Center 接地纸片视觉组件 `UDCPaperVisualComponent`（含主卡/同伴/工具/水柱多槽） | [`DCFixedStageCamera.h`](../DirectorCutJam/Source/DirectorCutJam/Public/Spatial/DCFixedStageCamera.h)、[`DCPaperVisualComponent.h`](../DirectorCutJam/Source/DirectorCutJam/Public/Spatial/DCPaperVisualComponent.h) |
| **Level1** | 关卡建筑与状态编排 `ADCGameMode`/`ADCStageDirector`；场景实体 `ADCCaptainActor`、`ADCDoorwayInteractable`、`ADCExtinguisherPickup`、`ADCFireHazardActor`、`ADCCivilianActor`、`ADCSafeZoneVolume` | [`DCStageDirector.h`](../DirectorCutJam/Source/DirectorCutJam/Public/Level1/DCStageDirector.h)、[`DCFirefighterActors.h`](../DirectorCutJam/Source/DirectorCutJam/Public/Level1/DCFirefighterActors.h) |
| **UI** | Retro Cute Journal 纯 Slate 向量绘制与配色/圆角主题库、全屏 Pre-Take 剧本面板、Active HUD（右上 `[R]`、顶部闪现星容器）、世界空间思想气泡与 `[E]` 水蓝环形提示、结算卡 | [`DCWidgetTheme.h`](../DirectorCutJam/Source/DirectorCutJam/Public/UI/DCWidgetTheme.h)、[`DCPersonaPanelWidget.h`](../DirectorCutJam/Source/DirectorCutJam/Public/UI/DCPersonaPanelWidget.h)、[`DCHudOverlayWidget.h`](../DirectorCutJam/Source/DirectorCutJam/Public/UI/DCHudOverlayWidget.h)、[`DCInteractPromptWidget.h`](../DirectorCutJam/Source/DirectorCutJam/Public/UI/DCInteractPromptWidget.h)、[`DCThoughtBubbleWidget.h`](../DirectorCutJam/Source/DirectorCutJam/Public/UI/DCThoughtBubbleWidget.h)、[`DCStarPopupWidget.h`](../DirectorCutJam/Source/DirectorCutJam/Public/UI/DCStarPopupWidget.h)、[`DCTakeResultWidget.h`](../DirectorCutJam/Source/DirectorCutJam/Public/UI/DCTakeResultWidget.h) |
| **Validation & Scripts** | UE Automation 测试套件 `DirectorCut.PIE.Acceptance` 与 Python 灰盒资产/地图生成脚本 | [`DCValidation.cpp`](../DirectorCutJam/Source/DirectorCutJam/DCValidation.cpp)、[`create_map.py`](../DirectorCutJam/Scripts/create_map.py) |

---

## 3. 工程配置与默认按键

- **默认地图与模式**（[`DefaultEngine.ini`](../DirectorCutJam/Config/DefaultEngine.ini)）：
  - `EditorStartupMap=/Game/Maps/Firefighter`
  - `GameDefaultMap=/Game/Maps/Firefighter`
  - `GlobalDefaultGameMode=/Script/DirectorCutJam.DCGameMode`
  - `DefaultGraphicsRHI=DefaultGraphicsRHI_DX12`，`NearClipPlane=10.0`，`r.DynamicGlobalIlluminationMethod=0`、`r.ReflectionMethod=0`、`r.DefaultFeature.AutoExposure=False`、`r.DefaultFeature.MotionBlur=False`、`r.Shadow.Virtual.Enable=0`。
- **按键绑定**（[`DefaultInput.ini`](../DirectorCutJam/Config/DefaultInput.ini) + [`DCPlayerCharacter.cpp`](../DirectorCutJam/Source/DirectorCutJam/Private/Characters/DCPlayerCharacter.cpp)）：
  - `W / S` -> `MoveForward (+1 / -1)`，`D / A` -> `MoveRight (+1 / -1)`（均按固定舞台机位水平平面投影移动）
  - `E` -> `StartInteractInput`（Pressed）/ `StopInteractInput`（Released）（支持门、灭火器、受困者瞬时交互与火点按住持续读条）
  - `R` -> `RestartTakeAction`（在 `PreTake` 已有删改后、以及 `ActiveTake` / `TakeResult` 中均可整轮重置回初始 `PreTake` 状态）
  - `Tab` -> `TogglePersona`（受 `ActiveTakePersonaViewPolicy` 控制，默认在 `ActiveTake` 锁定提示）
  - `Enter / Space` -> `StartTakeAction`（在 `PreTake` 等效点击 `ACTION!`）

---

## 4. GameplayTags 与 DataAsset 权威配置

### 4.1 Native GameplayTags（`DCGameplayTags`）

- **Persona / Limit**：`Persona.Identity.Firefighter`、`Persona.Motivation.SaveLives`、`Persona.Memory.DisobediencePunished`、`Persona.Limit.FearDisobeyCaptain`
- **Capability / State**：`Capability.ExtinguishFire`、`State.EscortingCivilian`、`State.SprayingFire`、`State.EscortingInjured`
- **Action**：`Action.Door.Enter`、`Action.Tool.PickupExtinguisher`、`Action.Fire.Extinguish`、`Action.Civilian.Rescue`

### 4.2 `/Game/Data/DA_FirefighterPersona`（`UDCPersonaDefinition`）

由 `create_map.py` 填充自 `UDCPersonaDefinition::MakeDefaultFirefighterGrayboxBlocks()`：

| 属性 / BlockID | 默认值 / 可编辑性 | 语义与 Tag |
|---|---|---|
| `PageHeaderTitle` / `LevelBadgeTitle` / `CharacterRoleTitle` | `"PERSONA"` / `"Fire Call"` / `"Firefighter"` | 手账页眉、关卡胶囊标签、副标题 |
| `Identity` | Fixed（`bCanDelete=false`） | `Persona.Identity.Firefighter` |
| `Motivation` | Fixed（`bCanDelete=false`） | `Persona.Motivation.SaveLives` |
| `Daily_A` | Cuttable（`bCanDelete=true`） | 无阻挡日常干扰项（删掉无惩罚） |
| `PunishmentMemory` | Cuttable（`bCanDelete=true`） | 携带 `Persona.Memory.DisobediencePunished` + `Persona.Limit.FearDisobeyCaptain`（删掉解除门口阻挡） |
| `Daily_B` | Cuttable（`bCanDelete=true`） | 可选第 5 块日常干扰项（受 `bEnableOptionalDailyBlockB` 控制） |

### 4.3 `/Game/Data/DA_FirefighterLevelConfig`（`UDCLevelConfigData`）

| 分类 | 属性名 | v2 默认值 | 说明 |
|---|---|---|---|
| **Stage** | `SceneTitle` / `LevelDisplayName` | `"Fire Call"` / `"Fire Call"` | UI 关卡标题 |
| **Stage** | `DefaultPersonaAsset` | `/Game/Data/DA_FirefighterPersona` | Pre-Take 默认加载的人格资产 |
| **Fire Timings** | `SecondsToInjureCivilian` | `7.2` s | 火势到达第 2 阶段并致未护送受困者受伤 |
| **Fire Timings** | `SecondsToKillCivilian` | `18.5` s | 火势到达第 3 阶段并致未护送受困者遇难 |
| **Fire Timings** | `SecondsToEngulfRoom` | `26.0` s | 基础全屋火满时间（每灭 1 处加 `EngulfDelayPerSpotSeconds`） |
| **Fire Timings** | `ExtinguishActionSeconds` | `1.8` s | 单个火点按住 `E` 灭火所需总读条秒数 |
| **Fire Timings** | `EngulfDelayPerSpotSeconds` | `4.5` s | 每扑灭 1 处火点延缓全屋火满的秒数 |
| **Star Rules** | `InitialStarContainers` / `StarsForExtinguish` / `StarsForRescueAlive` / `BonusStarsForRescueUninjured` | `3` / `1` / `1` / `1` | 总容器 3；全部灭火得 1 星；活体护送出屋得 1 星；未受伤额外得 1 星 |
| **Star Rules** | `ContainerPenaltyOnInjured` / `ContainerPenaltyOnKilled` / `StarPopupDisplaySeconds` | `1` / `1` / `2.0` s | 受伤扣 1 容器（3→2），遇难再扣 1 容器（2→1）；顶部闪现持续秒数 |
| **Movement** | `PlayerWalkSpeed` / `DefaultInteractionRadius` / `EscortFollowDistance` / `EscortMoveSpeed` | `300.0` / `175.0` / `90.0` / `280.0` | 移动与默认交互半径（`ThoughtBubbleDurationSeconds = 3.8` s） |
| **Camera** | `CameraFOV` / `CameraWorldLocation` / `CameraWorldRotation` | `64.0` / `(-350, -70, 220)` / `(-10.5, 1.5, 0)` | 符合 `08` §4.1 固定室内透视约束（`FOV [55,65]`、`Pitch [-15,-5]`、`Roll=0`） |

---

## 5. Level 01 空间坐标与已放置 Actor（`/Game/Maps/Firefighter`）

- **房间几何**（由 `ADCStageDirector` 构造）：
  - 室内地面：`X ∈ [0, 960]`、`Y ∈ [-300, 420]`、`Z ∈ [0, 340]`（左墙在 `Y=-300` 上开宽 `360 cm` 门口 `X ∈ [170, 530], Z ∈ [0, 260]`，外接门廊 `X ∈ [140, 620], Y ∈ [-520, -300]`）。
  - 膝盖高 S 形矮隔断（高 `46 cm`，`M_Partition`）：
    - `PartitionB1Mesh`：中心 `(560, 140, 23)`，尺寸 `(30, 560, 46)`，前部留出通道缺口 `G1`（`Y ∈ [-300, -140]`）。
    - `PartitionB2Mesh`：中心 `(760, 35, 23)`，尺寸 `(30, 350, 46)`，深处留出主通道缺口 `G2`（`Y ∈ [210, 420]`），前侧捷径缺口（`Y ∈ [-300, -140]`）初始被 `F1` 封锁。
- **材质资产**（`/Game/Materials/`）：`M_Floor`、`M_Wall`、`M_Ceiling`、`M_Partition`（`#6E7887` 灰蓝灰石板）、`M_DoorFrame`、`M_Entry`、`M_Porch`、`M_Ground`、`M_SafeZone`、`M_Fire`、`M_PaperMasked`（含 `TintColor` 与 `FlipU` 参数）及 6 个实例 `MI_Paper_Player`、`MI_Paper_Captain`、`MI_Paper_Civilian`、`MI_Paper_Extinguisher`、`MI_Paper_Door`、`MI_Paper_Fire`。

| Actor Label | C++ 类 | 摆放坐标 `(X, Y, Z)` / 朝向 | 关键组件与配置 |
|---|---|---|---|
| `DC_StageDirector` | `ADCStageDirector` | `(0, 0, 0)` | `LevelConfig = DA_FirefighterLevelConfig`，`PlayerSpawnLocation = (500, -345, 92)`，材质绑定全房间静态网格 |
| `DC_FixedStageCamera` | `ADCFixedStageCamera` | `(-350, -70, 220)` / `(-10.5, 1.5, 0)` | 自动读取 `LevelConfig` 相机位置、旋转、FOV（`64.0`） |
| `DC_CaptainActor` | `ADCCaptainActor` | `(370, -375, 0)` | 门外阻拦队长，`MI_Paper_Captain`，无台词 |
| `DC_DoorwayInteractable` | `ADCDoorwayInteractable` | `(350, -300, 0)` | 门洞阻挡与 `[E] ENTER` 交互，未删 `PunishmentMemory` 时拒行并弹气泡 |
| `DC_ExtinguisherPickup` | `ADCExtinguisherPickup` | `(320, 270, 0)` | Z1 右前侧角便携灭火器，`MI_Paper_Extinguisher`，赋予 `Capability.ExtinguishFire` |
| `DC_FireHazard_F1` | `ADCFireHazardActor` | `(760, -220, 0)` | `FireSpotIndex = 0`，`M_Fire` + `MI_Paper_Fire`，`bAutoExpandIndexZeroShortcutBlocker = true` 封锁 `B1/B2` 前侧捷径直到扑灭 |
| `DC_FireHazard_F2` | `ADCFireHazardActor` | `(660, 35, 0)` | `FireSpotIndex = 1`，`M_Fire` + `MI_Paper_Fire` |
| `DC_FireHazard_F3` | `ADCFireHazardActor` | `(880, 160, 0)` | `FireSpotIndex = 2`，`M_Fire` + `MI_Paper_Fire` |
| `DC_CivilianActor` | `ADCCivilianActor` | `(885, -95, 0)` | 深处 Z3 受困者，`MI_Paper_Civilian`；按 `E` 进入合体护送，倒下后隐藏交互 |
| `DC_SafeZoneVolume` | `ADCSafeZoneVolume` | `(420, -385, 0)` | `CivilianParkOffset = (45, 25, 0)`（安全区停靠点 `(465, -360, 0)`），`M_SafeZone` |
| `Firefighter_Start` | `APlayerStart` | `(500, -345, 92)` | 玩家起点，与 `StageDirector.PlayerSpawnLocation` 对齐 |

---

## 6. `[待决定]` 策划微调开关清单（对应 `08` §12.2 `#5`–`#10`）

以下全部字段均已在 `UDCLevelConfigData` 的 `TBD Policies (Pending Design)` 分类中实装（可在 [`DA_FirefighterLevelConfig`](../DirectorCutJam/Content/Data/DA_FirefighterLevelConfig.uasset) Details 面板免改代码直接切换）：

| 编号 | C++ 字段名 / 类型 | 当前默认值 | 可选替代值与运行期行为 |
|---|---|---|---|
| **#5** | `ExtinguishInterruptPolicy` (`EDCExtinguishInterruptPolicy`) | `KeepStatic` | 灭火按住 `E` 松手/走开中断时：**`KeepStatic`**（原地保留已喷进度）、`DecayOverTime`（按 `ExtinguishProgressDecayPerSecond = 0.25/s` 衰减）、`IncreaseRequiredOnStageUp`（火势升级后额外增加 `ExtraExtinguishSecondsPerFireStage = 0.6s` 读条时长） |
| **#6** | `EscortToolPolicy` (`EDCEscortToolPolicy`) | `HideToolAndDisableSpray` | 护送途中手持灭火工具规则：**`HideToolAndDisableSpray`**（藏起工具、禁止护送时喷水/拾取，防两手忙）、`ShowToolAndAllowSpray`（允许边背/扶人边喷水，切换 `*WithTool` 合体纸片） |
| **#7** | `bScaleFireBlockerWithStage` (`bool`) | `false` | 火势阶段膨胀时是否同步放大物理阻挡体积：**`false`**（固定 `FireBlockerBox` 不挡主路）、`true`（按 `FireBlockerStageScaleMultiplier = 1.15` 倍率放大） |
| **#8** | `ActiveTakePersonaViewPolicy` (`EDCActiveTakePersonaViewPolicy`) | `HiddenAndLocked` | 演出中按 `Tab` 开关：**`HiddenAndLocked`**（完全隐藏并锁死编辑，提示按 `R` 重拍）、`AllowTabReadOnlyPeek`（允许只读回看手账页） |
| **#9** | `ActiveFireSpotCount` (`int32`) + `bEnableOptionalDailyBlockB` (`bool`) | `3` + `true` | 控制激活火点数（`3` 处或 `4` 处；由 `ADCStageDirector::SyncFireSpotsWithConfig()` 同步隐藏多余火点并重算总火点）与是否保留第 5 块 `Daily_B` |
| **#10** | `FireVisualMode` (`EDCFireVisualMode`) | `Paper2DPlaceholder` | 火点显示方案：**`Paper2DPlaceholder`**（启用 `UDCPaperVisualComponent` 纸片火舌）、`Stylized3DPlaceholder`（使用 `FireCoreMesh / FireOuterMesh` 3D 火焰占位体并隐藏火点纸片） |
| **补充** | `DoorTraversalPolicy` (`EDCDoorTraversalPolicy`) | `FreeWalkAfterBreakthrough` | 门口突破后进出规则：**`FreeWalkAfterBreakthrough`**（首次 `E` 突破后本轮自由进出）、`RequireInteractEachCrossing`（每次穿越门框需再次按 `E`） |

---

## 7. 美术 & UI 零代码替换接入点

### 7.1 2D 纸片美术替换（`UDCPaperVisualComponent`）

每个角色与场景纸片实体均挂载 `UDCPaperVisualComponent`（原点严格为 **Bottom-Center** 本地 `Z=0`，程序强制保持 `Pitch=0 / Roll=0` 并面向相机平面，水平向左移动时将材质参数 `FlipU` 设为 `-1.0`）。

- **通用插槽（Details -> `Paper2D|Slots` / `Paper2D|Material`）**：
  - `IdleFlipbook` / `MoveFlipbook`（`UPaperFlipbook*`）
  - `IdleSprite`（`UPaperSprite*`）
  - `StateIdleFlipbooks` / `StateMoveFlipbooks` / `StateSprites`（按 `FName` 状态键映射多态帧/单图）
  - `PaperMaterialOverride`（需继承或保留 `TintColor` 与 `FlipU` 材质参数，或直接引用 `M_PaperMasked` 派生纹理材质）
  - 尺寸与辅助灰度色：`CardSizeCm`、`DeadCardSizeCm`、`StateTints`、`bFaceCameraPlane`、`bCastDynamicShadow`
  - 运行时 `6` 个纸片视觉与占位组件（填充正式 Sprite/Flipbook 后可替代灰盒单色片）：`FlipbookComp`、`SpriteComp`、`CardMeshComp`、`BuddyCardMeshComp`、`ToolCardMeshComp`、`SprayCardMeshComp`。

- **程序自动派发的 `StateKey` 名称清单**（`SetPaperState(Key)`，配置 `StateSprites` / `StateIdleFlipbooks` / `StateMoveFlipbooks` 时请使用下表精确键名）：

| Actor | 触发条件与精确 `StateKey`（`FName`） |
|---|---|
| `ADCPlayerCharacter` | 默认空手 `None`；持灭火器 `HoldingTool`；按住喷射 `Spraying`；合体护送未受伤 `CombinedEscortUninjured`；合体护送受伤 `CombinedEscortInjured`；若 `#6` 启用带工具护送：`CombinedEscortUninjuredWithTool` / `CombinedEscortInjuredWithTool` |
| `ADCCivilianActor` | 未伤 `None`；受伤 `Injured`；遇难 `Dead`；已送至安全区 `Rescued` |
| `ADCDoorwayInteractable` | 关闭 `None`；已突破敞开 `Unlocked` |
| `ADCCaptainActor` | `BlockingPostureState`（默认 `BlockEntranceArmsSpread`） |
| `ADCFireHazardActor` | 正常 `None`；被喷射中 `Spraying`；阶段对应受困者受伤/遇难/全屋火满状态同步 `InjuredCivilian` / `KilledCivilian` / `RoomEngulfed` |

### 7.2 UMG 皮肤与 `WBP_*` 蓝图替换（`UDCLevelConfigData` + `BindWidgetOptional`）

所有 UMG 基类已内置完整的纯 Slate/C++ Retro Cute Journal 默认视觉，**无需编写蓝图控件即可直接运行**；若美术希望使用设计师制作的 `WBP_*` Widget Blueprint 皮肤，只需将对应 C++ 类设为父类，并在 `DA_FirefighterLevelConfig` 的 `UI Overrides` 栏填入资产即可（全部插槽均为 `BindWidgetOptional`，未放的控件自动保持 C++ 行为，且提供 `BP_On*` 蓝图事件）：

| `DCLevelConfigData` 配置槽 | C++ 基类 | `BindWidgetOptional` 控件名 & 蓝图回调事件 |
|---|---|---|
| `HudOverlayWidgetClass` | `UDCHudOverlayWidget` | `PersonaPanelWidget`、`StarPopupWidget`、`ResultWidget`、`RetakeHintContainer`、`RetakeHintButton`、`RetakeHintLabelText`、`HintToastBorder`、`HintToastText`；事件 `BP_OnOverlayRefreshed(NewPhase, EarnedStars, MaxContainers)` |
| `PreTakeScriptWidgetClass` | `UDCPersonaPanelWidget` + `UDCBlockCutButton` | 面板：`BackdropBorder`、`JournalSheetBorder`、`RoleTitleText`、`PhaseBadgeBorder`、`PhaseBadgeText`、`ReadOnlyBadgeBorder`、`ReadOnlyBadgeText`、`BlocksContainer`、`StartTakeButton`、`StartTakeButtonLabel`、`RestartTakeButton`、`FooterRuleText`、`LegendFixedLabelText`、`LegendCuttableLabelText`、`LegendCutLabelText`（事件 `BP_OnPersonaPanelRefreshed`）；行按钮：`CutButton`、`ButtonCaptionText`、`BlockHighlightBorder`、`HoverScissorsIcon`（动态代理 `OnBlockCutTriggered`） |
| `InteractPromptWidgetClass` | `UDCInteractPromptWidget` | `KeycapBorder`、`KeycapText`、`PromptPillBorder`、`PromptLabelText`、`RingAnchorWidget`；事件 `BP_OnPromptStateUpdated(LabelText, HoldProgress01, bIsHoldMode, bIsSpraying)` |
| `ThoughtBubbleWidgetClass` | `UDCThoughtBubbleWidget` | `BubbleBorder`、`ThoughtTextBlock`、`SecondaryThoughtTextBlock`；事件 `BP_OnThoughtUpdated(NewThought)` |
| **顶部闪现星容器（嵌入 HUD）** | `UDCStarPopupWidget` | `PopupAccentBorder`、`StarsGlyphRow`、`StarsGlyphText`、`EventCaptionText`、`ContainerStatusText`；事件 `BP_OnStarPopupTriggered(EarnedStars, MaxContainers, Reason)` |
| `TakeResultWidgetClass` | `UDCTakeResultWidget` | `HeaderTitleText`、`TakeMetaText`、`StarsGlyphRow`、`StarsGlyphText`、`StarsSummaryText`、`EndReasonText`、`OutcomeSummaryText`、`RestartButton`；事件 `BP_OnResultUpdated(EndReason, EarnedStars, MaxContainers)` |
| **星级/剪刀贴图直换（免建 WBP）** | `UDCLevelConfigData` 直接配置 | `StarEmptyTexture`、`StarFilledTexture`、`StarBurntTexture`、`ScissorsCursorTexture`（自动覆盖 `UDCJournalStarWidget` 与 `UDCJournalScissorsWidget` 默认矢量图），以及全部手账按钮/标签 `FText` 字段（`ActionButtonLabel`、`RetakeHintLabel`、`Result*` 等） |

---

## 8. 扩展新关卡（Level 02+）标准流程

核心循环（`PreTake -> ActiveTake -> TakeResult -> [R] Retake`、星容器与 `UDCPersonaComponent` Tag 门控）与 Level 01 业务解耦：
1. 新建 `UDCPersonaDefinition` DataAsset，填写新关卡词条、固定/可删标志与授予的 `GameplayTags`。
2. 新建 `UDCLevelConfigData` DataAsset，指定 `DefaultPersonaAsset`、机位参数、火势/时间阈值及 UI 覆写。
3. 新建 `.umap`，放置 `ADCStageDirector`（绑定新 `LevelConfig`）、`ADCFixedStageCamera`、`APlayerStart` 与派生自 `ADCInteractableBase` 的关卡交互物；若需全通用交互，直接配置 `FDCInteractionRule`（`RequiredPersonaTags` / `BlockingPersonaTags` / `RequiredCapabilityTags` / `GrantedCapabilitiesOnSuccess`）即可完成门控验证。

---

## 9. 标准构建、重建与 Win64 DX12 验证命令

> 在 Windows PowerShell 中调用无头命令行工具前，务必显式设置 `${env:UE-LocalDataCachePath} = 'D:\UE_DDC'` 避免默认盘符 DDC 异常；切勿直接无参启动 `UnrealEditor.exe` 执行批处理。

```powershell
# 1. 编译 C++ Editor 模块
D:\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe `
  "D:\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" `
  DirectorCutJamEditor Win64 Development `
  -Project="d:\antigravity_projects\gamejam\DirectorCutJam\DirectorCutJam.uproject" -WaitMutex

# 2. 重建材质、DataAsset 与 /Game/Maps/Firefighter
${env:UE-LocalDataCachePath} = 'D:\UE_DDC'
& "D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "d:\antigravity_projects\gamejam\DirectorCutJam\DirectorCutJam.uproject" `
  -ExecutePythonScript="d:\antigravity_projects\gamejam\DirectorCutJam\Scripts\create_map.py" `
  -DCRebuildMap -unattended -nop4 -nosplash -NullRHI

# 3. 执行 Live DX12 PIE 验收并导出报告与 4 张实机截图
${env:UE-LocalDataCachePath} = 'D:\UE_DDC'
& "D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "d:\antigravity_projects\gamejam\DirectorCutJam\DirectorCutJam.uproject" `
  /Game/Maps/Firefighter `
  -ExecCmds="Automation RunTests DirectorCut.PIE.Acceptance; Quit" `
  -ReportExportPath="d:\antigravity_projects\gamejam\DirectorCutJam\Saved\Validation\Win64Report" `
  -DCCaptureEvidence -unattended -nop4 -nosplash
```

详细验收项、日志结果与截图说明见 [`DirectorCutJam/VALIDATION.md`](../DirectorCutJam/VALIDATION.md)。
