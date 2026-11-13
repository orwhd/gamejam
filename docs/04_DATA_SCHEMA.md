# 04 — DATA SCHEMA：数据契约与未填值

> **重要**：下列字段是为程序协作整理的**候选技术接口**，并非用户已选定的数据库、结构体名称或玩法数值。
> 对未决定的数据保持 `null`；不能将模板空值偷偷替换成游戏规则。

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
- **[待决定]** 完整 Persona 的正式文本、各种效果 ID、效果组合、权重、评分阈值、镜头数值。

## 3. ProjectConfig（占位）

```json
{
  "game_title": null,
  "game_jam_theme": "Director's Cut",
  "engine_name": "Unreal Engine",
  "engine_version": null,
  "target_platform": null,
  "input_scheme": null,
  "gameplay_implementation": null,
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
      "can_reorder": null,
      "is_present": null,
      "effect_ref": null
    }
  ],
  "edit_timing_rule": null,
  "undo_rule": null,
  "reorder_semantics": null
}
```

字段语义：
- `block_id`：同一 Persona 内标识一个完整文字块。
- `text`：该块显示的文本。
- `can_delete` / `can_reorder`：可操作性开关，**实际值由设计者填写**。
- `is_present`：当前块是否仍存在；是否持久化及如何重置尚未确定。
- `effect_ref`：连接到角色行为 / 记忆效果的标识；**解释机制尚未决定**。

> 不允许默认把删除 `not fireproof` 解释成 `fireproof = true`。

## 5. InteractionRuleSchema（候选字段）

```json
{
  "interaction_id": null,
  "actor_id": null,
  "target_id": null,
  "action_id": null,
  "required_context": null,
  "persona_gate_ref": null,
  "denial_thought_id": null,
  "result_event_id": null
}
```

- `persona_gate_ref` 只代表「这项交互可能受 Persona 约束」，**不规定布尔、数值、推理或 AI 实现方法**。
- `denial_thought_id` 指向思想泡泡文案；是否提供隐藏的背景原因：**TBD**。
- 碰撞检测、交互距离、提示按钮：**TBD**。

## 6. NPCScriptSchema（候选字段）

```json
{
  "npc_id": null,
  "script_id": null,
  "steps": [
    {
      "step_id": null,
      "trigger": null,
      "action": null,
      "parameters": null
    }
  ],
  "unexpected_world_state_policy": null
}
```

- `steps` 表示 **NPC 预设的行动计划**。
- 不要将 `steps` 直接当作世界结局记录。
- 时间触发 / 剧情触发的选择，以及遇阻处理：**TBD**。

## 7. Scene / OutcomeSchema（候选字段）

```json
{
  "scene_id": null,
  "controlled_character_id": null,
  "fire_extinguished": null,
  "person_rescued": null,
  "fire_escalation_definition": null,
  "result_evaluation_trigger": null,
  "result_rating": null
}
```

- `fire_extinguished` 和 `person_rescued` 是目前关卡目标中已有的概念。
- `fire_escalation_definition` 与 3 星精确阈值尚未确定。
- 不能把 `null` 当 `false`，也不能把 `null` 自动评价为 0 星。

## 8. SpatialPresentationSchema（候选字段）

```json
{
  "entity_id": null,
  "world_transform": null,
  "visual_form": null,
  "visual_asset_ref": null,
  "visual_anchor_offset": null,
  "plane_orientation_rule": null,
  "interaction_shape": null,
  "render_sort_policy": null
}
```

已确认的约束：
- `world_transform` 是世界中的位置，不是 UI 屏幕坐标。
- 角色 / 可交互物品的 `visual_form` 必须符合**纸片表现**。
- 房间结构本身仍属于 3D 场景。

未确认的值：
- Pivot 的具体局部坐标。
- 纸片朝向（Billboard / 固定方向 / 跟随对象）。
- Sprite 单位尺寸、比例尺、碰撞体尺寸。
- 透明排序 / 层级规则。

## 9. CameraConfig（字段留空）

```json
{
  "reference_goal": "用户提供的第二张室内参考图",
  "projection_mode": null,
  "location_cm": null,
  "rotation_deg": null,
  "fov_deg": null,
  "orthographic_width": null,
  "camera_follow_mode": null
}
```

**视觉目标已确定**：室内纵深构图，而不是高空俯视盒子。

**精确投影、机位、镜头运动均待决定**。不得自动填入看似合理的常用值。

## 10. 评分（记录既有构想；判定实现留空）

```json
{
  "rating_0_description": "火没有灭",
  "rating_1_description": "火灭了",
  "rating_2_description": "火灭了，人也救出来了",
  "rating_3_description": "在火势变大之前就把火灭掉了，人直接先被救出来",
  "rating_resolution_rules": null,
  "rating_3_mechanics": null
}
```

**[已确认]** 三星设计先暂置。描述不是可执行的逻辑规则。

## 11. Schema 校验红线

1. 未知值统一 `null`，而不是猜一个数值、时间、枚举或台词。
2. 字段命名可随项目现状调整，但不得改变已确认的语义约束。
3. 数据示例不能自动被当作正式关卡数据。
4. 逻辑效果与渲染资产引用分开储存。
5. 不允许因删除玩家记忆而自动修改 NPC 历史数据。
