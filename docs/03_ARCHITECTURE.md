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

**[技术必需]** UI、3D 模型、镜头和特效不能成为 Persona 规则、交互是否允许的权威数据源。

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
- Presentation binding：逻辑对象与 3D 模型 / 思想泡泡的关联。
- Outcome 数据：火灾、救援等目标的实际完成情况。

> 这些是**职责分类**，不表示必须创建同名 UE Component，也不自动增加数值系统。

### Systems / Logic
- Persona Edit Validator：检查编辑动作是否作用于可编辑的**完整文字块**。
- Persona State Updater：应用被删除的经历对当前角色记忆的变化。
- Interaction Resolver：依据当前角色状态判断是否允许交互。
- NPC Script Runner：按配置推进 NPC 原定行动。
- Outcome Evaluator：根据火势四阶段演化与救援/灭火事实维护动态星容器及最终星级。
- Presentation Update：读取状态并显示 3D 模型、泡泡、UI。

这些英文名称是**占位职责名**，不是必须使用的 UE 类名。

## 4. 最小状态模型

### 4.1 Take 阶段与 Persona 块状态

```text
[Pre-Take 开拍前准备] --(Start Take 开拍)--> [Active Take 正式演出] --(结算条件满足)--> [Take Result 结算]
         ^                                            |                                      |
         +------------(Restart Take 重拍重置)----------+--------------------------------------+
```

```text
PRESENT --[仅在 Pre-Take 合法删除]--> DELETED
DELETED --[仅通过 Restart Take 整轮重置]--> PRESENT
```

- **[已确认]** 仅在 `Pre-Take` 阶段且文字块 `can_delete == true` 时允许触发 `PRESENT -> DELETED`；进入 `Active Take` 后锁定编辑权限。
- **[已确认]** 单轮演出内不支持单块撤销恢复；`DELETED -> PRESENT` 仅在触发 `Restart Take`（整轮重置 Persona 与场景状态）时发生。
- **[已确认]** 灰盒原型阶段不启用重排状态机（仅保留 `can_reorder` 数据字段扩展位，当前关卡全为 `false`）。

### 4.2 交互判定（确定性标签检查）

```text
InteractionRequest
    -> Evaluate(active Persona tags + scene state)
       -> ALLOWED -> ExecuteRequestedAction
       -> DENIED  -> ReturnReason -> ThoughtBubble (表层抗拒想法 + 隐晦过去经历线索)
```

- **[已确认]** 每个处于 `PRESENT` 状态的文字块向角色提供 `0..N` 个显式状态标签（可含限制类标签与能力/认知前提类标签）；删除文字块即移除其提供的全部标签，不自动反向赋权。
- **[已确认]** 交互判定同时检查场景客观条件、所需前置标签是否具备、以及是否存在阻止该交互的心理限制标签。
- **[已确认]** 因心理限制被拒绝时，`ThoughtBubble` 输出「表层抗拒态度 + 隐晦指向过去经历的线索」。

### 4.3 NPC 与事件结果

```text
Preauthored NPC Script -> NPC intended actions (如队长无台词撑手拦在入口前)
Fire Stage Progression -> Active -> CivilianInjured (容器 3->2) -> CivilianDead (容器 2->1) -> RoomEngulfed (强制收工)
Player interventions   -> 拾取/开启灭火工具、扑灭火灾、护送受困者至安全区
Scene actual state     -> 动态星容器即时闪现反馈 & 终局结算
```

- **[已确认]** NPC 的预设行为与实际场景结果分离；消防员关卡中队长全程无台词、双手撑开拦截在门前，不随主角删除记忆而改变站位或态度。
- **[已确认]** 主角不设血条死亡；已护送至安全区的救援星数在火势铺满全屋收工时予以保留。

### 4.4 动态星容器与关卡结算模型

- **[已确认]** 权威状态维护 `EarnedStars`（初始 `0`）与 `MaxStarContainers`（初始 `3`）：
  - 扑灭火灾：`EarnedStars += 1`，触发即时星容器闪现事件。
  - 受困者护送至安全区：若未受伤 `EarnedStars += 2`，若已受伤 `EarnedStars += 1`，触发即时星容器闪现事件。
  - 火势波及未获救受困者使其受伤：`MaxStarContainers` 由 `3` 减至 `2`，触发即时星容器扣减闪现事件。
  - 火势继续波及未获救受困者致其身亡：`MaxStarContainers` 由 `2` 减至 `1`，触发即时星容器扣减闪现事件。
- **[已确认]** 单轮结束判定：`EarnedStars == MaxStarContainers`（达成当前剩余容器满星）**或** `火势阶段 == RoomEngulfed（铺满整个房间）`。
- **[技术必需]** 星容器 UI 仅订阅上述状态变化事件做短暂闪现展示，不得由 UI 自身维护或裁决星数。

## 5. 事件 / 接口合同（仅定义职责，不锁定名称）

| 意图 / 事件 | 需要传递的最小信息 | 具体 API |
|---|---|---|
| 请求删除块（仅 `Pre-Take`） | 主角标识、Block ID | TBD |
| 块状态变化 | 主角标识、Block ID、状态 | TBD |
| 开始演出 / 重拍重置 | 关卡/Take 标识 | TBD |
| 请求执行交互 | 主角、目标、动作标识（进门/取工具/灭火/护送） | TBD |
| 交互被拒绝 | 请求标识、原因 / 思想泡泡文本标识 | TBD |
| 火势阶段推进 | 当前火势阶段、受困者健康状态变化 | TBD |
| 星容器即时闪现通知 | 当前已得星数 `EarnedStars`、当前容器上限 `MaxStarContainers`、变化类型 | TBD |
| 单轮结束结算 | 结束原因（满容器 / 火满全屋）、最终星数与容器上限 | TBD |

如项目已有事件系统，优先检查后适配；不得据此表凭空建立复杂 Event Bus。

## 6. 表现与逻辑解耦红线

1. 删掉或替换 3D 模型 **不能**删除 NPC、角色或交互对象的逻辑实体。
2. 修改 UI 文字 **不能**作为 Persona 变化的唯一数据操作；必须先通过编辑规则验证。
3. 3D 模型的原点、材质和朝向 **不能**决定交互权限。
4. 相机如何移动 **不能**改变世界对象的真实坐标或其 Persona 状态。
5. 交互失败时，思想泡泡是**规则结果的反馈**，不是由泡泡控件决定是否允许交互。
6. 角色被删除的记忆影响的是当前角色；NPC 过去和记忆不得自动跟着删除。

## 7. Antigravity Agent 执行和验证协议

- Antigravity 的项目规则入口是仓库根目录 `AGENTS.md`。修改架构前，必须先阅读本文件及相关 `docs/` 文档；不要假定设计文档随启动自动注入。


- 先扫描已有项目，列出现有 Gameplay Framework / Blueprints / Assets。
- 以最小可替换模块验证：Persona 编辑 -> 角色状态变化 -> 交互判定 -> 可视反馈。
- 遇到本文件标记 `TBD` 的游戏设计问题，不应补写「最终逻辑」。
- 报告：修改文件、数据流、UE 编辑器测试内容、仍未解决项。
