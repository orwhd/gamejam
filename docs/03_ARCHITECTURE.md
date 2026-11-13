# 03 — ARCHITECTURE：架构边界、ECS 与状态机

> 本文定义**职责与依赖方向**；不替用户选择 Blueprint、C++、Mass Entity 或第三方 ECS 的具体实现。

## 1. 未决定的技术选型

| 决策 | 状态 |
|---|---|
| UE 具体版本 | **TBD** |
| 以 Blueprint 还是 C++ 为主 | **TBD** |
| ECS：UE Actor/Component、Mass Entity 或自定义方式 | **TBD** |
| NPC 剧本：Sequencer / 状态机 / 定时事件等 | **TBD** |
| Persona 语义解释：映射表、规则引擎、LLM 等 | **TBD** |
| 持久化 / 存档 / 关卡重放方案 | **TBD** |

**规则**：先界定 Entity / Component / System 的逻辑边界，再选 UE 具体承载技术。不得把「需要 ECS 文档」理解为用户已经选定 Mass Entity。

## 2. 逻辑层级

```text
User Input
    |
    v
Command / Intent
    |
    v
Gameplay Rules  <---->  Authoritative Game State
    |
    +-- Persona rules
    +-- Interaction permission
    +-- NPC script progression
    +-- Scene outcome evaluation
    |
    v
Gameplay Events / State Changes
    |
    v
Presentation Adapters
    +-- World-space paper visuals
    +-- Thought bubble / Persona UI
    +-- Camera / VFX / Audio (if present)
```

**[技术必需]** UI、Sprite、镜头和特效不能成为 Persona 规则、交互是否允许的权威数据源。

## 3. ECS 职责（逻辑模型，不是已选引擎 API）

### Entity / Actor 边界
- Player-controlled character：角色身份与世界对象。
- NPC：按照固定剧本推进的世界对象。
- Interactable：场景中的可交互目标。
- Scene：场景 / 事件状态容器。

对象的正式类名与父类：**TBD**。

### Components / Data
- Persona 数据：原始文字块、当前存在的块、编辑权限。
- Character memory / behavioral restrictions：与 Persona 有关的可查询行为状态。
- Interaction 数据：可执行动作与规则依赖。
- NPC script 数据：预设行动的状态与进度。
- World transform：世界坐标与碰撞标识。
- Presentation binding：逻辑对象与纸片 / 思想泡泡的关联。
- Outcome 数据：火灾、救援等目标的实际完成情况。

> 这些是**职责分类**，不表示必须创建同名 UE Component，也不自动增加数值系统。

### Systems / Logic
- Persona Edit Validator：检查编辑动作是否作用于可编辑的**完整文字块**。
- Persona State Updater：应用被删除的经历对当前角色记忆的变化。
- Interaction Resolver：依据当前角色状态判断是否允许交互。
- NPC Script Runner：按配置推进 NPC 原定行动。
- Outcome Evaluator：根据场景结果产生评价信息（细则未定）。
- Presentation Update：读取状态并显示纸片、泡泡、UI。

这些英文名称是**占位职责名**，不是必须使用的 UE 类名。

## 4. 最小状态模型

### 4.1 Persona 块状态

```
PRESENT --[合法删除]--> DELETED
```

- 仅当文字块允许删除时才可触发。
- 文字块是否允许恢复，以及 `DELETED -> PRESENT` 何时发生：**TBD**。
- 重排块的完整状态机：**TBD**。

### 4.2 交互判定（非自动改变角色意愿）

```
InteractionRequest
    -> Evaluate(current Persona / scene state)
       -> ALLOWED -> ExecuteRequestedAction
       -> DENIED  -> ReturnReason -> ThoughtBubble
```

- 玩家直接操控动作，Persona 决定交互能否执行。
- 具体怎样从删减文字推出 `ALLOWED/DENIED`：**TBD**。
- ThoughtBubble 是否提供完整因果解释：**TBD**。

### 4.3 NPC 与事件结果

```
Preauthored NPC Script -> NPC intended actions
Player interventions  -> Scene actual state
Scene actual state    -> Outcome evaluation
```

- NPC 的计划与已发生的场景结果是两个概念。
- NPC 遭遇被玩家改变的场景结果时如何响应：**TBD**。
- NPC 是否必须精确按时间轴执行：**TBD**。

### 4.4 关卡评价

- 已提出 0–3 星目标（详见 `01_GDD_CORE.md`）。
- 评分触发时机、优先级、三星阈值与所有实际逻辑分支：**TBD**。
- 不要预先编出一套能够让 3 星成立的解谜条件。

## 5. 事件 / 接口合同（仅定义职责，不锁定名称）

| 意图 / 事件 | 需要传递的最小信息 | 具体 API |
|---|---|---|
| 请求删除块 | 主角标识、Block ID | TBD |
| 块状态变化 | 主角标识、Block ID、状态 | TBD |
| 请求执行交互 | 主角、目标、动作标识 | TBD |
| 交互被拒绝 | 请求标识、原因 / 提示文本标识 | TBD |
| NPC 推进预设动作 | NPC 标识、脚本步骤标识 | TBD |
| 实际场景结果变化 | 场景 / 目标状态 | TBD |

如项目已有事件系统，优先检查后适配；不得据此表凭空建立复杂 Event Bus。

## 6. 表现与逻辑解耦红线

1. 删掉 Sprite **不能**删除 NPC、角色或交互对象的逻辑实体。
2. 修改 UI 文字 **不能**作为 Persona 变化的唯一数据操作；必须先通过编辑规则验证。
3. 视觉纸片的 Pivot、材质和朝向 **不能**决定交互权限。
4. 相机如何移动 **不能**改变世界对象的真实坐标或其 Persona 状态。
5. 交互失败时，思想泡泡是**规则结果的反馈**，不是由泡泡控件决定是否允许交互。
6. 角色被删除的记忆影响的是当前角色；NPC 过去和记忆不得自动跟着删除。

## 7. Antigravity Agent 执行和验证协议

- Antigravity 的项目规则入口是仓库根目录 `AGENTS.md`。修改架构前，必须先阅读本文件及相关 `docs/` 文档；不要假定设计文档随启动自动注入。


- 先扫描已有项目，列出现有 Gameplay Framework / Blueprints / Assets。
- 以最小可替换模块验证：Persona 编辑 -> 角色状态变化 -> 交互判定 -> 可视反馈。
- 遇到本文件标记 `TBD` 的游戏设计问题，不应补写「最终逻辑」。
- 报告：修改文件、数据流、UE 编辑器测试内容、仍未解决项。
