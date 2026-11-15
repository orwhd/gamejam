# 04 — DATA SCHEMA：通用数据配置契约

> 本文定义支撑「双刃剑失忆解谜」「5 大手绘角色舞台」与「`VisualMode` 切换」的数据资产结构契约（UE 使用 `UDataAsset` + `UStruct` 承载，向编辑器公开全部文案、贴图与调参字段）。
> 第一关《消防员》的完整词条表、参考英文文案与推荐配平值见 [`08_LEVEL_01_FIREFIGHTER.md`](08_LEVEL_01_FIREFIGHTER.md) §2–§7。

---

## 1. Persona 数据结构（`PersonaDefinition`）

```json
{
  "persona_id": "Firefighter_Pipo",
  "page_header_title": "PERSONA",
  "page_subtitle_hint": "Snip unwanted memories with ✂️ before ACTION! — careful what he forgets.",
  "director_cut_badge_text": "Director's Cut ✂️",
  "blocks": [
    {
      "block_id": "CaptainTrauma",
      "text": "Last week I scuffed Chief Witch's red heels—she'll zap me into a frog if I disobey!",
      "can_delete": true,
      "can_reorder": false,
      "granted_tags": ["Persona.Limit.FearDisobeyCaptain"]
    }
  ]
}
```

- **`granted_tags` 三类确定性标签（删块即同步移除，不自动产生反向超能力）**：
  1. `Persona.Limit.*`：心理阴影或死板教条限制（如怕红高跟魔女队长、火没灭完不敢碰爱心裤衩伤员）；
  2. `Persona.KnowHow.*`：专业技能记忆前提（删掉后把灭火器当成烤鸡番茄酱瓶而无法拾取）；
  3. `Persona.Trait.*`：身体素质记忆（如每天抱胖天使修狗爬楼练出的强壮腿力，删掉后抱人移速暴跌）。

---

## 2. 交互门控与泡泡反馈结构（`InteractionRule`）

```json
{
  "interaction_id": "RescueCivilian",
  "input_mode": "InstantPressE | HoldEToComplete | FlavorOnly",
  "hold_duration_sec": 1.6,
  "interrupt_policy": "KeepStaticPerTarget",
  "blocking_persona_tags": ["Persona.Limit.MustClearFireFirst"],
  "blocking_condition": "OnlyWhenRemainingFiresGreaterThanZero",
  "blocking_denial_bubble": "Wait—Rule #1! Clear ALL flames before hugging a guy in pink-heart boxers!",
  "required_persona_tags": [],
  "missing_knowhow_bubble": "",
  "required_capability_tags": [],
  "missing_capability_bubble": "",
  "on_success_bubble_if_missing_trait": {
    "trait_tag": "Persona.Trait.StrongLegs",
    "bubble_text": "Oof... noodle legs!! Without puppy-lifting workouts, he weighs a ton!!"
  },
  "on_success_passing_bubble": ""
}
```

---

## 3. 空间表现与关卡配置结构（`SpatialPresentation` & `LevelConfigData`）

```json
{
  "protagonist_visual_mode": "2D_Paper | 3D_Mesh",
  "paper_cutout_style": "DieCutWhiteStickerBorder_MaskedTwoSided",
  "procedural_paper_waddle": {
    "enabled": true,
    "roll_sway_deg": 6.0,
    "hop_height_cm": 7.0,
    "hop_frequency_hz": 5.5
  },
  "escort_visual_strategy": "CombinedSpriteIfAssigned_ElseDualCardPiggyback",
  "fire_timings_sec": {
    "to_injure_civilian": 9.5,
    "to_kill_civilian": 19.5,
    "to_engulf_room": 20.5,
    "extinguish_hold_per_spot": 1.6,
    "engulf_delay_per_spot_extinguished": 3.5
  },
  "movement_params": {
    "walk_speed_cm_s": 320.0,
    "escort_speed_strong_cm_s": 295.0,
    "escort_speed_weak_amnesia_cm_s": 130.0,
    "escort_tool_policy": "HideToolAndDisableSpray",
    "active_take_tab_readonly_peek": true
  },
  "camera_params": {
    "projection_mode": "Perspective",
    "camera_mode": "DollhousePanFollow_LockedZAndAngles_XYClamp",
    "fov_deg": 60.0,
    "constant_height_z_cm": 245.0,
    "locked_pitch_deg": -16.0,
    "locked_yaw_deg": 0.0
  }
}
```
