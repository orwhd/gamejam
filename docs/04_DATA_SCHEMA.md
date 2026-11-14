# 04 — DATA SCHEMA：数据契约与未填值

> **重要**：下列字段是为程序协作整理的**通用候选技术接口**，并非用户已选定的数据库、结构体名称或玩法数值。
> 对未决定的数据保持 `null`；不能将模板空值偷偷替换成游戏规则。
> **第一关《消防员（Fire Call）》的专属字段草案与编辑器待填清单见 [`08_LEVEL_01_FIREFIGHTER.md`](08_LEVEL_01_FIREFIGHTER.md) §9。**

## 1. UE 数据载体说明

本项目明确使用 **Unreal Engine**，而 `ScriptableObject` 是 Unity 的术语。

UE 中可以考虑 JSON、`UDataAsset`、`UPrimaryDataAsset`、`UDataTable`、`UStruct` 等数据载体，但**选型未决定**。

此文以 JSON 展示可讨论的**字段形状**，Antigravity Agent 不得据此断言一定要使用 JSON 作为运行时存储。

## 2. 确认与未确认的字段语义

- **[已确认]** 文字块具有整体操作边界，并可分别控制是否可删除 / 重排。
- **[已确认]** 删除经历的对象是当前角色的记忆。
- **[已确认]** Persona 能影响角色的交互。
- **[已确认]** NPC 的既定剧本与实际世界结果需要分开表示。
- **[已确认]** 世界对象有实际 3D 位置，纸片只是可见表现。
- **[编辑器待填值]** 各文字块与泡泡的具体英文词句、子目标数量、持续交互读条秒数、危机延缓秒数、阶段推进阈值秒数、移动/交互距离数值、摄像机精确厘米坐标与偏航角均保持 `null`，交由编辑器配置与灰盒实测校准（第一关详细配置清单见 [`08_LEVEL_01_FIREFIGHTER.md`](08_LEVEL_01_FIREFIGHTER.md) §9.3）。

## 3. ProjectConfig（已确认项与待定项）

```json
{
  "game_title": "Persona",
  "game_jam_theme": "Director's Cut",
  "engine_name": "Unreal Engine",
  "engine_version": "5.8",
  "target_platform": "Windows (developed on macOS)",
  "input_scheme": "Keyboard (WASD move, E interact/hold, R restart take) + Mouse (Full-screen Pre-Take script & UI)",
  "current_level_scope": "SingleLevel_FirefighterSceneFirst",
  "gameplay_implementation": "C++ authoritative rules & state machine + Editor-exposed config / DataAsset / UMG",
  "persona_evaluation_mode": "Deterministic rule/tag mapping (no LLM)",
  "persistence_scope": "Per-take / per-level restart (no cross-session save)",
  "data_backend": null
}
```

## 4. PersonaSchema（候选字段）

```json
{
  "persona_id": null,
  "owner_character_id": null,
  "blocks": [
    {
      "block_id": null,
      "text": null,
      "can_delete": null,
      "can_reorder": false,
      "is_present": null,
      "granted_tags": []
    }
  ],
  "edit_timing_rule": "PreTakeOnly",
  "pre_take_presentation_mode": "FullScreenTextScript_RoomHidden",
  "active_take_presentation_mode": "HiddenAndLocked_Full3DRoomView",
  "active_take_tab_readonly_peek": null,
  "undo_rule": "RestartTakeFullResetOnly",
  "reorder_enabled_in_prototype": false
}
```

字段语义：
- `block_id`：同一 Persona 内标识一个完整文字块。
- `text`：该块显示的文本（由编辑器配置英文词句）。
- `can_delete`：可删除开关，**实际值由设计者按块填写**。
- `can_reorder`：重排扩展字段；**[已确认]** 灰盒原型阶段不启用重排玩法，默认填 `false`。
- `is_present`：当前块在本轮 Take 中是否仍存在；仅在 `Pre-Take` 阶段可置为 `false`，触发 `Restart Take` 时重置回初始值。
- `granted_tags`：该块激活（`is_present == true`）时赋予角色的 `0..N` 个确定性状态标签列表（可含心理限制标签或正面认知/能力标签；删块即失去这些标签，绝不自动产生反向能力标签）。
- `active_take_tab_readonly_peek`：`Active Take` 期间默认隐藏并锁定剧本页；是否允许按 `Tab` 键只读查看当前残存文字块（不可剪辑）见 [`07_UI_UX_SPEC.md`](07_UI_UX_SPEC.md) §5.4 `[待决定]`。

> 不允许默认把删除 `not fireproof` 解释成 `fireproof = true`。

## 5. InteractionRuleSchema（候选字段）

```json
{
  "interaction_id": null,
  "actor_id": null,
  "target_id": null,
  "action_id": null,
  "input_mode": "InstantPressE | HoldEToComplete",
  "hold_duration_sec": null,
  "progress_on_interrupt": "KeepPerTarget | Reset",
  "prompt_key": "E",
  "required_context": null,
  "required_persona_tags": [],
  "blocking_persona_tags": [],
  "denial_thought_id": null,
  "result_event_id": null
}
```

- `required_persona_tags` / `blocking_persona_tags`：确定性标签门控——要求角色具备某些认知/能力标签，且不携带阻止该行为的心理限制标签。
- `denial_thought_id`：统一指向头顶思想泡泡文案——**[已确认]** 当因心理限制标签被拒绝时输出「表层抗拒想法 + 隐晦指向过去经历的线索」；当因缺少 `required_context`（如前置工具/道具）被拒绝时，同样通过 `denial_thought_id` 触发主角英文思想泡泡吐槽提示（第一关具体表现见 [`08_LEVEL_01_FIREFIGHTER.md`](08_LEVEL_01_FIREFIGHTER.md) §5）。
- **[已确认]** 交互提示与输入模式：靠近处于可交互状态的目标时显示 `[E]` 提示（目标变为不可交互态时立即隐藏）；支持按一下 `E` 瞬时触发与按住 `E` 持续读条两类模式，`hold_duration_sec` 由编辑器配置，`progress_on_interrupt` 在第一关采用按目标保留进度（`KeepPerTarget`）。
- 具体碰撞形状尺寸与触发半径数值：保持 `null`，在编辑器校准。

## 6. NPCScriptSchema（候选字段）

```json
{
  "npc_id": null,
  "script_id": null,
  "has_dialogue": false,
  "posture_or_behavior": null,
  "steps": [
    {
      "step_id": null,
      "trigger": null,
      "action": null,
      "parameters": null
    }
  ]
}
```

- NPC 的预设剧本与站位/姿态不随玩家删减自身记忆而自动改变；第一关队长的具体实例配置见 [`08_LEVEL_01_FIREFIGHTER.md`](08_LEVEL_01_FIREFIGHTER.md) §9.1。
- 不要将 `steps` 直接当作世界结局记录。

## 7. Scene / OutcomeSchema（通用候选字段）

```json
{
  "scene_id": null,
  "controlled_character_id": null,
  "crisis_stage": null,
  "stage_durations_sec": {},
  "max_star_containers": null,
  "earned_stars": 0,
  "take_end_condition": "EarnedStarsEqualsMaxContainers_Or_TerminalCrisis"
}
```

- `stage_durations_sec` 具体秒数留空（`null`），由设计者在编辑器配置微调，不在 Schema 中写死。
- 第一关《消防员》的完整字段实例（含 `has_extinguish_tool`、多火点计数与延缓秒数、四阶段火势、受困者状态与合体护送状态等）已收归 [`08_LEVEL_01_FIREFIGHTER.md`](08_LEVEL_01_FIREFIGHTER.md) §9.2。

## 8. SpatialPresentationSchema（候选字段）

```json
{
  "entity_id": null,
  "world_transform": null,
  "visual_form": "WorldSpace2DPaper",
  "carrier_type": "PaperSprite_Or_PlaneMesh",
  "material_blend_mode": "Masked_TwoSided",
  "pivot_rule": "BottomCenter",
  "plane_orientation_rule": "UprightFacingCameraPlane_NoPitchTilt",
  "turn_behavior": "HorizontalFlip",
  "visual_asset_ref": null,
  "visual_anchor_offset": null,
  "interaction_shape": null
}
```

已确认的约束：
- `world_transform` 是世界中的 3D 位置，角色可在水平地面 `X/Y` 四/八方向移动（无跳跃）。
- 角色、NPC、合体护送状态与可交互物品采用竖直立于 3D 地面的 2D 纸片（底边中心接地、`Masked` 双面材质写深度、左右转身水平翻转）。
- 头顶思想泡泡与近距 `[E]` 提示锚定世界 3D 坐标并投影至屏幕空间（Screen-space）显示。

## 9. CameraConfig（已确认区间与待校准坐标）

```json
{
  "reference_goal": "assets/example/envirments/example_0.png（同时可见墙壁、天花板、地面及左侧敞开门口门外区的室内透视纵深构图）",
  "projection_mode": "Perspective",
  "camera_mode": "SingleFixedIndoorObliqueFrontLeftLookingDeep_NoCut",
  "exterior_to_interior_staging": "VisibleThroughOpenDoorwayInSameFrame",
  "fov_deg_range": [55, 65],
  "pitch_down_deg_range": [5, 15],
  "location_cm": null,
  "yaw_deg": null
}
```

- **[已确认]** 全程采用单固定室内 3D 透视机位（不切镜头），位于房间前方偏左朝室内深处拍摄，`FOV 55°–65°`，俯视角 `5°–15°`，同时可见墙壁、天花板和地面，并透过敞开的门口/玄关在同一画面看到门外入口区与安全区。
- `location_cm` 与 `yaw_deg` 在灰盒关卡搭建时按上述视觉约束实测填入。

## 10. 星容器与评价规则契约（通用结构）

```json
{
  "initial_star_containers": null,
  "star_sources": {},
  "container_penalties": {},
  "hud_feedback_mode": "TransientPopupOnStarOrContainerChange"
}
```

- **[已确认]** 星容器仅在获得星或容器上限扣减时屏幕顶部居中即时闪现，非常驻显示；单轮在「填满当前剩余星容器」或「达到终局危机阶段」时结束。
- 第一关《消防员》的 3 星来源、容器扣减值及 `3/3` 满星实现路径详案见 [`08_LEVEL_01_FIREFIGHTER.md`](08_LEVEL_01_FIREFIGHTER.md) §7 & §9.2。

## 11. Schema 校验红线

1. 未知值统一 `null`，而不是猜一个数值、时间、枚举或台词。
2. 字段命名可随项目现状调整，但不得改变已确认的语义约束。
3. 数据示例不能自动被当作正式关卡数据。
4. 逻辑效果与渲染资产引用分开储存。
5. 不允许因删除玩家记忆而自动修改 NPC 历史数据。
