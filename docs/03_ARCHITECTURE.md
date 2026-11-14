# 03 — ARCHITECTURE：架构边界、ECS 与状态机

> 本文定义**职责与依赖方向**；不替用户选择 Blueprint、C++、Mass Entity 或第三方 ECS 的具体实现。

## 1. 产品与技术级选型状态

| 决策 | 状态 |
|---|---|
| UE 具体版本 | **[已确认]** UE 5.8（开发机环境 5.8.4） |
| 目标交付平台与跨平台约束 | **[已确认]** Windows 打包提交，macOS 仅作开发机；**[技术必需]** 禁止依赖 macOS 专有路径或仅 Metal 可用的特性 |
| C++ 与编辑器分工 | **[已确认]** C++ 负责底层权威规则与状态机；关卡配置、文字块数据与 UI 表现开放给编辑器（Details / DataAsset / UMG）调整 |
| ECS：UE Actor/Component、Mass Entity 或自定义方式 | **TBD**（由编程 Agent 在 C++ 权威框架下按原型规模选定，不预设 Mass Entity） |
| NPC 剧本：Sequencer / 状态机 / 定时事件等 | **TBD** |
| Persona 语义解释机制 | **[已确认]** 确定性规则 / 多标签映射（**不接入 LLM**；单块可赋予 `0..N` 个标签，删块同步移除） |
| 持久化 / 存档 / 关卡重放方案 | **[已确认]** 单局 / 按关重开，无跨退出存档；仅 `Pre-Take` 可删块，单轮内不可单块撤销，仅靠 `Restart Take` 整轮重置 |

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
- Outcome 数据：局内子目标完成情况、场景危机阶段与动态星容器状态。

> 这些是**职责分类**，不表示必须创建同名 UE Component，也不自动增加数值系统。

### Systems / Logic
- Persona Edit Validator：检查编辑动作是否作用于可编辑的**完整文字块**。
- Persona State Updater：应用被删除的经历对当前角色记忆的变化。
- Interaction Resolver：依据当前角色状态判断是否允许交互。
- NPC Script Runner：按配置推进 NPC 原定行动。
- Outcome Evaluator：根据场景危机阶段演化与目标完成/失效事实维护动态星容器及最终星级。
- Presentation Update：读取状态并显示纸片、泡泡、UI。

这些英文名称是**占位职责名**，不是必须使用的 UE 类名。

## 4. 最小状态模型

### 4.1 Take 阶段与 Persona 块状态

```text
[Pre-Take 全屏 Persona 纸页] --(Start Take 开始切入 3D 房间)--> [Active Take 正式演出] --(结算条件满足)--> [Take Result 结算]
             ^                                                        |                                      |
             +--------------(Restart Take / 按 R 键重拍重置)----------+--------------------------------------+
```

```text
PRESENT --[仅在 Pre-Take 合法删除]--> DELETED
DELETED --[仅通过 Restart Take 整轮重置]--> PRESENT
```

- **[已确认]** `Pre-Take` 阶段显示全屏纯文字 Persona 纸页（隐藏 3D 房间画面，世界时间暂停），仅在此阶段且文字块 `can_delete == true` 时允许触发 `PRESENT -> DELETED`；点击 `ACTION!` 进入 `Active Take` 后切入 3D 房间画面，默认隐藏该纸页并锁定编辑权限（演出期间是否支持按 `Tab` 只读查看见 [`07_UI_UX_SPEC.md`](07_UI_UX_SPEC.md) §5.4 `[待决定]`）。
- **[已确认]** 单轮演出内不支持单块撤销恢复；`DELETED -> PRESENT` 仅在触发 `Restart Take`（按 `R` 键或点击手账结算卡 `[R] Retake` 按钮，退回全屏 `Pre-Take` 并整轮重置 Persona 与场景状态）时发生。
- **[已确认]** 灰盒原型阶段不启用重排状态机（仅保留 `can_reorder` 数据字段扩展位，当前关卡全为 `false`）；当前仅交付第一个「消防员」关卡闭环，不设跨关卡流转状态机。

### 4.2 交互判定（确定性标签与场景前置条件检查）

```text
InteractionRequest
    -> Evaluate(active Persona tags + scene prerequisite state)
       -> ALLOWED -> ExecuteRequestedAction
       -> DENIED  -> ReturnReason -> ThoughtBubble (心理受阻线索 或 缺少前置条件吐槽)
```

- **[已确认]** 每个处于 `PRESENT` 状态的文字块向角色提供 `0..N` 个显式状态标签（可含限制类标签与能力/认知前提类标签）；删除文字块即移除其提供的全部标签，不自动反向赋权。
- **[已确认]** 交互判定同时检查场景客观条件（如是否持有前置工具、目标是否仍可交互）、所需前置标签是否具备、以及是否存在阻止该交互的心理限制标签。
- **[已确认]** 因心理限制被拒绝时，`ThoughtBubble` 输出「表层抗拒态度 + 隐晦指向过去经历的线索」；因缺少场景前置道具/条件被拒绝时，同样可返回原因并由 `ThoughtBubble` 输出角色吐槽提示。

### 4.3 NPC、场景危机演化与事件结果

```text
Preauthored NPC Script     -> NPC intended actions (NPC 按预设站位、姿态或行动推进，不随主角删记忆改变)
Scene Crisis Progression   -> InitialActive -> GoalDegraded (容器扣减/状态变化) -> GoalLost (容器再扣减/关闭交互) -> TerminalCrisis (强制收工)
Player Interventions       -> 获取前置道具、执行瞬时/持续读条交互（按目标保留进度）、触发合体护送/状态冻结等介入行动
Scene Actual State         -> 动态星容器即时闪现反馈 & 终局结算
```

- **[已确认]** NPC 的预设行为与实际场景结果分离；主角删减自身记忆后，NPC 原有记忆、剧本与站位不随之改变，仅主角自身的心理门控与交互能力发生变化。
- **[已确认]** 权威状态维护关卡子目标与危机阶段：可消除阻挡的交互目标在读条完成后移除阻挡并按规则延缓危机终态；持续交互支持中断且保留当前目标进度；进入合体/接手保护态的受助对象状态冻结，不受后续危机阶段恶化影响；目标永久失效后立即关闭其可交互状态并通知表现层更新。
- **[已确认]** 主角不设血条死亡；已达成并结算的星数在到达终局失控收工时予以保留。
- 第一关《消防员》的队长拦门、多火点计数 `RemainingFireCount`、四阶段火势与合体护送具体状态机见 [`08_LEVEL_01_FIREFIGHTER.md`](08_LEVEL_01_FIREFIGHTER.md) §5–§7。

### 4.4 动态星容器与关卡结算模型

- **[已确认]** 权威状态维护 `EarnedStars`（初始 `0`）与 `MaxStarContainers`（初始由关卡目标数决定，如 `3`）：
  - 当局内目标达成时，按关卡规则增加 `EarnedStars`，触发即时星容器点亮闪现事件；
  - 当尚未接手的局内目标因危机恶化而永久部分或全部失效时，同步扣减 `MaxStarContainers`，触发即时星容器扣减闪现事件；
  - 第一关具体加星与扣减条件见 [`08_LEVEL_01_FIREFIGHTER.md`](08_LEVEL_01_FIREFIGHTER.md) §7。
- **[已确认]** 单轮结束判定：`EarnedStars == MaxStarContainers`（达成当前剩余容器满星）**或** 到达关卡终局危机阶段（如火势铺满全屋）。
- **[技术必需]** 星容器 UI 仅订阅上述状态变化事件做短暂闪现展示，不得由 UI 自身维护或裁决星数。

## 5. 事件 / 接口合同（仅定义职责，不锁定名称）

| 意图 / 事件 | 需要传递的最小信息 | 具体 API |
|---|---|---|
| 请求删除块（仅 `Pre-Take`） | 主角标识、Block ID | TBD |
| 块状态变化 | 主角标识、Block ID、状态 | TBD |
| 开始演出（`ACTION!`）/ 重开重置（`Restart Take`，支持 `R` 键与结算按钮） | 关卡/Take 标识 | TBD |
| 请求执行 / 持续按住 / 中断交互 | 主角、目标、动作标识与输入动作类型（瞬时触发 / 持续按住 / 中断） | TBD |
| 交互被拒绝（心理限制或缺少前置条件） | 请求标识、拒绝原因类型、思想泡泡文本标识 | TBD |
| 子目标状态与场景危机阶段推进 | 剩余子目标计数、当前危机阶段、NPC/目标状态变化、危机延缓秒数 | TBD |
| 星容器即时闪现通知 | 当前已得星数 `EarnedStars`、当前容器上限 `MaxStarContainers`、变化类型 | TBD |
| 单轮结束结算 | 结束原因（满容器 / 危机终态）、最终星数、容器上限与客观结果摘要 | TBD |

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
