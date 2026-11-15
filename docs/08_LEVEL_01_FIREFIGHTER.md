# 08 — LEVEL 01：《消防员（Fire Call）》全明星手绘舞台与解谜详案（v3.1）

> 本文是第一关《消防员（Fire Call）》的**唯一权威关卡圣经**。本版将仓库 `assets/characters/` 上传的 **5 张极具特色的童趣蜡笔手绘资产（方脸消防员、红高跟魔女、爱心裤衩尖叫哥、卖火柴想烤鸡的小女孩、天使光环修狗）** 100% 编织进同一场**「童话荒诞公寓火灾」**，与**「6 词条双刃剑失忆谜题」**实现丝丝入扣的化学反应！
> 文中英文文案与配平数值为 **`[参考默认]`**（开箱即玩），编程实现须全部通过 `DataAsset` 开放编辑器配置。

---

## 1. 全明星登场阵容：5 张特色手绘资产的戏剧定位 `[已确认]`

原本单调的“普通房间救火”现全面升级为令人过目不忘的**无厘头童话合租屋火灾现场**——美工绘制的每一个怪诞细节都在玩法和台词中得到正面呼应：

| 舞台角色 | 绑定美术资产 (`assets/characters/`) | 舞台摆位 | 角色戏份与玩法职能 |
|---|---|---|---|
| **主角：菜鸟消防员 Pipo** | **[`firefighter.png`](../assets/characters/firefighter.png)**<br>*(拆出天蓝蜡笔水花做喷射特效)* | `Z0` 门外出生，全场自由移动 | 大小眼方脸憨直消防员；行为与体能完全受玩家在 `Pre-Take` 剪裁的记忆支配（保留 `VisualMode: 2D_Paper / 3D_Mesh` 切换开关） |
| **拦门上司：红高跟魔女队长（Chief Witch）** | **[`witch.png`](../assets/characters/witch.png)** | `Z0 / D` 公寓门口外侧 | 戴紫尖帽、扎彩带金麻花辫、踩着**大红高跟鞋**举星星魔杖死拦门口（全程高冷无台词）；Pipo 若未剪除「踩脏她红高跟差点被变青蛙」的阴影则不敢进门！ |
| **门外迎宾：天使光环修狗 Coco** | **[`dog.png`](../assets/characters/dog.png)** | `Z_Safe` 门外左侧安全垫上方 | 头顶**天蓝光环与小翅膀**、吐舌流口水的金色卷毛修狗，作为**门外救援安全区的活体信标**扑腾守候！同时对应词条 `#3`（每天抱胖修狗爬 42 层楼练出的神级腿力） |
| **纵火彩蛋：卖火柴想烤鸡的小女孩** | **[`little girl.png`](../assets/characters/little%20girl.png)** | `Z1` 室内右前角（紧挨灭火器 `T`） | 穿补丁背带裤蹲在满地烧黑火柴梗中淡定划火柴，头顶气泡正幻想**香喷喷的金黄烤鸡**——**一秒无字揭晓整间屋子起火的罪魁祸首！** 同时与灭火器组成“烤鸡配大号红番茄酱瓶”的爆笑失忆梗！ |
| **救援目标：爱心裤衩尖叫哥** | **[`rescue.png`](../assets/characters/rescue.png)**<br>*(拆出左右两团红黄蜡笔火焰做 `F1~F3` 火点)* | `Z3` 室内左后深处 | 戴小墨镜、穿**天蓝底粉红爱心四角裤**、**一只脚光脚一只脚穿黄人字拖**，在蜡笔烈火间摆出《呐喊》张大嘴狂叫！随火势「未伤 → 熏伤 → 倒下遇难」三态切换 |

---

## 2. 6 条互锁 Persona 词条矩阵（条条绑定特色角色！） `[已确认结构 · 参考默认英文]`

全页严格控制在 **6 条短句（2 条固定 + 4 条荧光黄可剪，共 16 种剪法组合，仅 1 种 `3/3` 满星黄金解）**。4 条可剪词条分别精准钩住舞台上的 **天使修狗、红高跟魔女、卖火柴小女孩旁边的灭火器、以及爱心裤衩尖叫哥**！

| 顺序 & Block ID | 性质 | 可删 | 赋予权威标签 (`granted_tags`) | `[参考默认]` 手账英文句子（≤16 词） | 保留 vs 剪除的真实玩法后果 |
|---|---|---|---|---|---|
| **1. `Identity`** | 固定 · 身份 | ❌ `false` | `Persona.Identity.Firefighter` | `I am square-headed rookie firefighter Pipo.` | 锁定方脸消防员身份 |
| **2. `Motivation`** | 固定 · 动机 | ❌ `false` | `Persona.Motivation.SaveLives` | `Someone is screaming inside—I have to save them!` | 明确冲进屋救裤衩哥的核心动机 |
| **3. `AngelPupWorkout`** | **伪装成宠物日常的【体能正面块】** *(联动 `dog.png`)* | ✂️ `true` | `Persona.Trait.StrongLegs` | `Every morning I carry my chubby angel pup Coco up forty-two stairs.` | **保留（必留）**：抱起裤衩哥健步如飞（`295 cm/s`）冲向门外修狗；<br>**误剪**：忘记抱狗练腿经历，背人移速暴跌至 `130 cm/s` 变面条腿，导致折返灭火超时！ |
| **4. `WitchCaptainTrauma`** | **核心阻挡 1：【魔女阴影负面块】** *(联动 `witch.png`)* | ✂️ `true` | `Persona.Limit.FearDisobeyCaptain` | `Last week I scuffed Chief Witch's red heels—she'll zap me into a frog if I disobey!` | **保留**：门口按 `E` 怕被星星魔杖变青蛙而拒绝进门；<br>**剪除（必剪）**：彻底忘掉魔女威慑，夸着她的尖尖帽和红高跟直接钻进火场！ |
| **5. `ExtinguisherTraining`** | **防全删陷阱：【学院知识正面块】** *(联动 `little girl.png`)* | ✂️ `true` | `Persona.KnowHow.UseExtinguisher` | `At the academy, I memorized how to spray a red foam extinguisher.` | **保留（必留）**：正常拿起角落灭火器狂飙天蓝水花（`firefighter.png`）；<br>**误剪**：看着旁边小女孩头顶的烤鸡幻想，**把红色灭火器当成配烤鸡的大号番茄酱瓶**拒不拾取！ |
| **6. `SafetyDogma`** | **核心阻挡 2：【死板教条负面块】** *(联动 `rescue.png`)* | ✂️ `true` | `Persona.Limit.MustClearFireFirst` | `Academy Rule #1: NEVER hug a half-dressed victim while any flame still burns!` | **保留**：只要屋内还有火没灭（`RemainingFires > 0`），靠近爱心裤衩哥按 `E` 就会因死守教条+害羞而拒绝接手，逼你先灭火致其熏伤（锁死 `2/2` 星）；<br>**剪除（冲 3 星必剪）**：忘掉死板规矩，直接冲进火海一把抱走爱心裤衩哥！ |

---

## 3. 全场景交互与 6 大爆笑思想泡泡全表 `[已确认]`

每一句气泡都直接点响美工画作里的视觉包袱（红高跟、星星魔杖、烤鸡幻想、番茄酱瓶、粉红爱心裤衩、天使修狗）：

| # | 交互目标与情境 | 触发判定条件 | 执行结果 | `[参考默认]` 头顶英文思想泡泡文案 | 设计意图与线索指向 |
|---|---|---|---|---|---|
| **1a** | **门前畏惧魔女**（`D` 门口 / `witch.png`） | 按 `E` 且携带 `Limit.FearDisobeyCaptain` | **拒绝进门** | `"EEK! Red heels and a star wand! Disobey Chief Witch and ribbit—I'm a frog!"` | 指向剪除 `#4 WitchCaptainTrauma` |
| **1b** | **失忆钻过魔女**（`D` 门口 / `witch.png`） | 按 `E` 且**已剪除** `#4` | **开门入屋**（此后自由进出）；魔女保持原姿势呆立门外 | *(进门瞬间自动飘出 1.8s 不阻塞气泡)*：<br>`"Fab red heels and party hat, ma'am~ Coming through!"` | 引爆“单向失忆”反差笑点！ |
| **2a** | **失忆把灭火器当烤鸡番茄酱**（`T` 角落灭火器，紧挨 `little girl.png`） | 对灭火器按 `E`，但**缺失** `KnowHow.UseExtinguisher` | **拒绝拾取** | `"A giant ketchup bottle for that kid's imaginary roast chicken?! No clue how it works!"` | **神级联动梗**：惩罚全删党，提示恢复 `#5 ExtinguisherTraining` |
| **2b** | **正常拾取灭火器**（`T`） | 对灭火器按 `E` 且具备 `#5` | 地面灭火器消失，Pipo 切为怀抱灭火器贴纸态 | — | — |
| **2c** | **彩蛋互动：吐槽卖火柴小女孩**（`little girl.png`） | 靠近角落划火柴的小女孩按 `E`（可选轻交互，不卡流程） | 不阻断移动，弹出吐槽气泡 | `"Kid, quit lighting matches for imaginary roast chicken—pants-guy is cooking for real!!"` | 盘活 `little girl.png` 纵火真相彩蛋！ |
| **3a** | **空手试图灭火**（`F1~F3` 蜡笔火焰） | 未拿灭火器对火点按 `E` | **拒绝灭火** | `"I can't spit enough water! Need that red bottle by the matchstick kid!"` | 引导玩家看右前角小女孩身旁的灭火器 |
| **3b** | **狂飙天蓝水花灭火**（`F1~F3`） | 持灭火器按住 `E`（松手原地保留进度） | 显示 `firefighter.png` 天蓝水花层 + 外圈水蓝环形读条（`1.6s`）；读满清除该火点与碰撞（**灭 `F1` 打通捷径**）并续时 `+3.5s`；全灭 **`+1` 星** | — | 还原 `firefighter.png` 灵魂飙水画面 |
| **4a** | **死板教条拒抱裤衩哥**（`C` 处 `rescue.png`） | 对存活裤衩哥按 `E`，但携带 `Limit.MustClearFireFirst` **且屋内仍有未灭火点** | **拒绝救援** | `"Wait—Rule #1! Clear ALL flames before hugging a guy in pink-heart boxers!"` | **2 星升 3 星核心锁**：提示剪掉 `#6 SafetyDogma`！ |
| **4b** | **神腿抱人冲刺**（`C` 处 `rescue.png`） | 合法按 `E` 接手存活裤衩哥，且具备 `Trait.StrongLegs`（保留了 `#3`） | 切为合体护送/双卡背人态，**立即冻结伤势**，按 `295 cm/s` 高速奔向门外天使修狗 | — | 爽快救援手感 |
| **4c** | **失忆面条腿龟速背人**（`C` 处 `rescue.png`） | 合法按 `E` 接手存活裤衩哥，但**误剪了 `#3` 天使修狗锻炼记忆** | 切为护送态并冻结伤势，但**护送移速暴跌至 `130 cm/s`** | *(接手瞬间飘出吃力吐槽气泡)*：<br>`"Oof... noodle legs!! Did I forget all my puppy-carrying stair workouts?!"` | 搞笑惩罚全删党，导致回屋灭火差几秒超时！ |
| **5** | **奔向天使修狗完成救援**（`Z_Safe` 处 `dog.png`） | 护送裤衩哥踏入天使修狗 Coco 驻守的 `Z_Safe` 安全垫 | 自动放下裤衩哥结算（未伤 **`+2` 星** / 已伤 **`+1` 星`），修狗欢快弹跳，Pipo 恢复单人（及已持工具）态 | *(顶部星容器弹跳闪现)* | 让 `dog.png` 成为温暖治愈的救援终点！ |

---

## 4. 娃娃屋舞台空间布局（5 角色同台全景图） `[已确认]`

```text
                        BACK WALL（深处，封闭天花板）
+----------------------+--------------------------------------------------+
|  [dog.png 天使修狗]   |  [rescue.png 爱心裤衩尖叫哥]         (F3 蜡笔火)   |  Z3 后区
|   Z_Safe 门外安全垫   |        ^                                  <---+  |
|   (放下即得救援星!)   |        | (灭掉 F1 即打通直达捷径!)              |  |
|                      |  [F1 蜡笔火]======= B2 矮隔断 ========   [G2]  |  |
|   Z0 门外走廊        |        ^                                      ^  |
| [Pipo] -> [witch.png]| 宽视窗/低剖面 (门外受阻可直视裤衩哥惨叫)        |  |  Z2 中区
|          红高跟魔女   |        |           (F2 蜡笔火)     ---------->|  |
|          举魔杖拦门   D 门口  [G1]======= B1 矮隔断 ===================  |
|                      |--->            [little girl.png 卖火柴女孩] [T]   |  Z1 前区右角
+----------------------+--------------------------------------------------+
  FRONT 朝镜头统一敞开 | 镜头：定高 Z + 恒定俯角 10°–20° + 锁死 Yaw，沿 X/Y 平移跟随 + Clamp
```

- **一眼封神的环境叙事动线**：
  1. 开场镜头聚焦左侧门外：方脸 **Pipo** 面对踩着大红高跟鞋举魔杖的 **魔女队长**，左上角 **天使修狗 Coco** 顶着蓝光环扑腾翅膀；隔墙宽视窗后方，**爱心裤衩哥** 正张大嘴惨叫！
  2. 突破进门向右平移展开舞台：右前角地板上，**卖火柴小女孩** 正守在红色灭火器 `T` 旁边划火柴幻想吃烤鸡，地上散落着火柴梗，三团同款红黄蜡笔火焰 `F1 / F2 / F3` 沿着低矮隔断烧出一条 S 形救援走廊！

---

## 5. 评委试玩「三幕四回合」心流与数学配平基准 `[已确认]`

### 5.1 三幕反转体验流（5～7 分钟笑声不断）
1. **第 1 回合（不剪试探 → `0 星`）**：门口按 `E` 怕被红高跟魔女变青蛙，隔窗看爱心裤衩哥被熏黑碎星（`3→2`），按 `R` 秒懂规则。
2. **第 2 回合（两类典型受挫 → `2 星`）**：
   - **老实派**（只剪 `#4 怕魔女`）：钻过魔女冲到裤衩哥面前，却触发 `#6` 铁律气泡（*“火没灭完不准抱穿粉红爱心裤衩的伤员！”*），被迫去卖火柴小女孩旁边拿灭火器灭完 3 团火，裤衩哥已熏伤，拿 **`2/2` 星（留 1 颗焦黑碎星）**；
   - **全删耍聪明派**（4 条黄色全划掉）：虽能抱起无伤裤衩哥，却触发**忘抱天使修狗变面条腿龟速** + **把小女孩身旁的灭火器当成烤鸡番茄酱瓶拒捡**，爆笑中火满全屋仅拿 **`2/3` 星**！
3. **第 3 回合（`Director's Cut` 封神 → `3/3` 满星）**：精准**留 `#3 天使修狗` + 剪 `#4 红高跟魔女` + 留 `#5 学院灭火器` + 剪 `#6 死板教条`** → 夸着魔女高跟鞋冲进火海 → S 路抢出无伤裤衩哥送给天使修狗（`+2` 星）→ 折返从小女孩身旁抄起灭火器轰开 `F1` 捷径喷灭全场（`+1` 星）= **`3/3` 大满贯！**

### 5.2 数学配平基准值（`UDCLevelConfigData` `[参考默认]`）
- **移动速度**：标准行走 `320 cm/s`；保留 `#3` 强壮护送 `295 cm/s`；误删 `#3` 虚弱护送 **`130 cm/s`**。
- **时间与灭火**：`T_injure = 9.5s`（直接救人约 `6.8s` 稳拿无伤；先灭 3 处火必超 `12s` 致熏伤）；`T_dead = 19.5s`；基础火满 `T_engulf = 20.5s`；单处喷水 `t_spray = 1.6s`（中断原地保留进度）；每灭 1 处续时 `Δ_delay = +3.5s`（强壮腿力约 `21.8s < 27.5s` 轻松全清；面条腿送完人已达 `16.4s`，刚跑到 `F1` 即在 `20.5s` 被火满截胡！）。

---

## 6. 结算卡手账星级与角色冷幽默复盘评语 `[已确认]`

| 结局分支 | 星级展示 | `[参考默认]` 结算卡底部冷幽默复盘提示 |
|---|---|---|
| **门外怕变青蛙未进门** | **`0 星`** (`✖ ✖ ☆`) | `"Chief Witch's red heels won. Pipo stayed frog-free on the porch."` |
| **仅灭完火（裤衩哥遇难）** | **`1 星`** (`★ ✖ ✖`) | `"Fires out! Sadly, heart-boxers guy screamed his last scream."` |
| **死守教条局（火灭 + 救出熏伤裤衩哥）** | **`2 星`** (`★ ★ ✖`) | `"Textbook Rule #1! Too bad smoke doesn't wait for protocol..."` |
| **失忆番茄酱局（无伤救人 + 不识灭火器）** | **`2 星`** (`★ ★ ☆`) | `"Heart-boxers guy is safe! Shame Pipo thought the extinguisher was roast-chicken ketchup."` |
| **失忆面条腿局（无伤救人 + 抱狗记忆被删超时）** | **`2 星`** (`★ ★ ☆`) | `"How could you snip out Angel Pup Coco?! Skipping leg day cost the room!"` |
| **`Director's Cut` 完美局** | **`3 星`** (`★ ★ ★` + 天使修狗贴纸欢呼) | `"PERFECT DIRECTOR'S CUT! One flip-flop, zero burns, all flames out!"` |

---

## 7. 交接给 `编程 Agent` 的 v2 → v3.1 完整落地清单

1. **模切贴纸切图与拆层导入（按 [`05_ASSET_PIPELINE.md`](05_ASSET_PIPELINE.md) §3）**：
   - 将 `assets/characters/` 的 5 张原画按「6–10px 白纸贴纸包边 + 外部透明硬切」生成贴图：拆出 `firefighter.png` 主体与天蓝水花 `T_VFX_WaterSplash_Crayon`、拆出 `rescue.png` 裤衩哥主体与两团红黄火焰 `T_Env_Fire_Crayon_A/B`，连同 `witch.png`（拦门 `ADCCaptainActor`）、`little girl.png`（`Z1` 角落彩蛋 Actor）、`dog.png`（`Z_Safe` 安全区信标 Actor）接入场景。
   - 在主角表现层保留 `VisualMode`（`2D_Paper | 3D_Mesh`）切换开关，并为 `2D_Paper` 开启移动时的轻量程序化纸偶摇摆小跳与 `BuddyCardMeshComp` 双卡背人回退。
2. **更新 6 条双刃剑 Persona 数据与交互逻辑**：
   - 注册 `Persona.Trait.StrongLegs`、`Persona.KnowHow.UseExtinguisher`、`Persona.Limit.MustClearFireFirst`，按 §2–§3 配置 6 条词条与 6 类角色联动思想泡泡（含卖火柴小女孩按 `E` 吐槽彩蛋）。
3. **相机与舞台升级**：
   - 将摄像机升级为定高 `Z`、恒定俯角（约 `-16°`）、锁死 `Yaw` 的娃娃屋 `X/Y` 平移跟随相机 + 边界 Clamp；隔墙补齐视窗/低剖面避让，校准 §5.2 配平数值，开启局内 `Tab` 只读速览手账与 `Director's Cut ✂️` 页眉贴纸引导。
