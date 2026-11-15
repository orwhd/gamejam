# 03 — ARCHITECTURE：权威规则分层、VisualMode 开关与三态交互裁决

> 本文定义权威游戏逻辑与表现层的职责边界及状态流转。底层规则由 C++ 权威维护，配置数据与视觉表现开放给 UE 编辑器（`DataAsset / Details / UMG`）。

---

## 1. 逻辑与表现严格分层 `[技术必需]`

```text
User Input (Mouse Cut / WASD / E / Tab / R)
    |
    v
Authoritative Gameplay Rules (C++)
    +-- Persona Tag Manager    (管理固定/可删词条，重建 Limit / KnowHow / Trait 标签集)
    +-- Interaction Resolver   (按标签集 + 场景状态裁决：正常执行 / 失忆修正执行 / 拒绝执行 / 彩蛋吐槽)
    +-- Stage & Crisis Runner  (推进火势阶段、剩余火点计数、护送冻结与危机延缓)
    +-- Star Container Judge   (维护 EarnedStars 与 MaxStarContainers，裁决单轮结算)
    |
    v
Presentation Adapters (Read-Only Subscribers)
    +-- 2.5D Sticker Paper Visuals (模切白边蜡笔贴纸纸片 + 程序化蹦跶摇摆 + 双卡错位背人)
    +-- 3D Mesh Fallback Visuals   (VisualMode 切换开关：开发期 3D 白模救急与视觉效果对比)
    +-- Screen-Space Bubbles       (锚定头顶输出受阻线索、失忆乌龙或角色彩蛋吐槽泡泡)
    +-- Journal & Stage UI         (Pre-Take 手账页、Tab 只读速览、顶部闪现星容器、手账结算卡)
    +-- Dollhouse Pan Camera       (定高定俯角锁死朝向，沿 X/Y 平滑跟随 + Clamp)
```

**解耦红线**：UI、2D 贴纸纸片、3D 占位模型、摄像机和特效**绝对不能**反向充当交互权限或星级计算的权威数据源。

### 1.1 主角表现层切换开关（`VisualMode`：`2D_Paper | 3D_Mesh`） `[已确认]`
- **底层权威单一性**：主角始终共用唯一一套 `ACharacter` 胶囊体碰撞、地面移动、交互检测、Persona 门控与跟随相机逻辑。
- **表现层枚举开关**：在主角表现组件与 `LevelConfigData` 中开放 `VisualMode`：
  - **`2D_Paper`（正式交付默认）**：隐藏 3D Mesh，显示接地竖直 2D 蜡笔贴纸纸片（[`firefighter.png`](../assets/characters/firefighter.png)），左右移动执行水平翻转（Flip）并叠加轻量程序化小碎步摇摆跳跃；
  - **`3D_Mesh`（开发救急与对比）**：隐藏 2D 纸片，显示 3D 几何白模占位并沿移动方向朝向旋转，便于在切图未导入前验证关卡及实机一键对比 2D/3D 观感。

---

## 2. 核心状态机 `[已确认]`

### 2.1 演出三阶段与 `Tab` 只读速览
```text
[Pre-Take 全屏手账编辑] --(点击 ACTION! / Space)--> [Active Take 舞台实机 (支持 Tab 只读速览)]
          ^                                                    |
          |                                         (满剩余容器 或 终局失控)
          |                                                    v
          +---------(按 R 键 或 点击 [R] Retake 整轮重置)--- [Take Result 手账结算卡]
```
- **词条状态**：`PRESENT --(仅限 Pre-Take 点击剪除)--> DELETED --(仅限 Restart Take 整轮重置)--> PRESENT`。
- **锁定与速览**：进入 `Active Take` 后词条不可增删；按 `Tab` 键呼出半透明只读手账浮层核对当前保留/划除的记忆。

### 2.2 三态交互裁决模型（含场景彩蛋 NPC 交互）
每次玩家按 `E` 触发交互时，权威层按以下优先级评估：

```text
InteractionRequest(Actor, Target)
  0. 若目标为纯叙事彩蛋 NPC（如室内划火柴幻想吃烤鸡的小女孩 little girl.png）：
       -> FLAVOR_BUBBLE -> 不消耗状态，根据当前记忆弹出对应爆笑吐槽泡泡
  1. 检查目标自身是否仍有效（如受困者已遇难 -> 直接静默关闭交互并隐藏 [E]）
  2. 检查 BlockingPersonaTags（含条件型教条，如“场上仍有未灭火点时禁止触碰爱心裤衩伤员”）
       -> 命中限制 -> DENIED -> 弹出「心理/教条受阻线索泡泡」
  3. 检查 RequiredPersonaTags（专业知识前提，如 KnowHow.UseExtinguisher）
       -> 缺失前提 -> DENIED -> 弹出「失忆乌龙搞笑泡泡」（把灭火器当成小女孩烤鸡用的大号番茄酱瓶）
  4. 检查 RequiredSceneCapabilities（场景道具前提，如是否已拾取灭火器）
       -> 缺失道具 -> DENIED -> 弹出「缺工具提示泡泡」
  5. 允许执行（ALLOWED）并检查 TraitPersonaTags（如天使修狗负重锻炼记忆 Trait.StrongLegs）：
       -> 具备标签 -> NORMAL_EXECUTION（高速抱人奔跑；突破红高跟魔女拦门时附带俏皮气泡）
       -> 缺失标签 -> DEGRADED_EXECUTION（失忆变面条腿：龟速抱人 + 弹出吃力喘气吐槽泡泡）
```

### 2.3 危机推进与动态星容器规则
- **火点与延缓**：单处火点按住 `E` 读满即消除自身视觉与 3D 碰撞（打通 `F1` 捷径），并延缓全屋火满时间 `+Δ_delay`；松手中断时**按火点原地保留已喷进度（不衰减）**。
- **接手状态冻结**：受困者一旦在存活态进入护送，健康状态**立即冻结**；护送期间自动收起灭火工具禁喷射，踏入天使修狗驻守的 `Z_Safe` 结算后自动恢复单人（及已拾取工具）状态。
- **动态星容器**：达成目标加 `EarnedStars`，未接手伤员受伤/遇难扣减 `MaxStarContainers`；当 `EarnedStars == MaxStarContainers` 或到达 `RoomEngulfed` 时立即弹出手账结算卡。
