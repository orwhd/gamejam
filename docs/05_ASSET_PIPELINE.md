# 05 — ASSET PIPELINE：蜡笔贴纸抠图、单图蹦跶动画与 2.5D 管线规范

> 本文定义如何将仓库 `assets/characters/` 中极具灵魂的**白底蜡笔手绘原画**高质感、零损耗地接入 UE 5.8 的 3D 娃娃屋舞台。画风与拆层详案见 [`06_ART_DIRECTION.md`](06_ART_DIRECTION.md)。

---

## 1. 核心管线巧思：「模切白边剪纸贴纸（Die-Cut Sticker）」抠图法 `[已确认]`

仓库正式角色原画（[`firefighter.png`](../assets/characters/firefighter.png)、[`witch.png`](../assets/characters/witch.png)、[`rescue.png`](../assets/characters/rescue.png)、[`little girl.png`](../assets/characters/little%20girl.png)、[`dog.png`](../assets/characters/dog.png)）采用**白纸底上的松散蜡笔/炭笔/水彩涂鸦笔触**。若直接对松散蜡笔线条做透明硬抠，会导致边缘发虚或黑边锯齿；因此统一采用**「模切白边贴纸（Die-Cut Paper Sticker）」**处理管线：

1. **保留 6～10px 手工剪纸白边**：沿角色与笔触外围留出一圈圆润干净的**纯白纸张贴纸包边（`#FFFFFF` 或暖白 `#FFFDF7`）**，白边以内 100% 保留原始蜡笔飞白与纸纹，白边以外为 **100% 全透明 Alpha（`A = 0`）**，交界处做实边硬切。
2. **一举两得的技术与美学收益**：
   - **技术零瑕疵**：完美匹配 UE `M_PaperMasked`（`Alpha Clip = 0.5` 双面材质），真实写入 3D Scene Depth 并投射干净的剪纸阴影，彻底杜绝半透明排序穿帮！
   - **视觉大加分**：角色站在 3D 房间里就像从手账本上用剪刀剪下来的**立体纸偶贴纸**，不仅在深色/火光背景前轮廓极其醒目，更与 `Director's Cut ✂️`（剪刀剪裁）主题浑然天成！

---

## 2. 4 天 Jam 超高性价比动画方案（单张原画也能活灵活现） `[已确认]`

为确保在仅有单张关键帧 PNG 时画面依然充满生机，同时无缝兼容后续序列帧，管线采用**「程序化纸偶蹦跶 + 双卡背人回退」**双保险机制：

| 机制 | 实现规范 | 产能价值 |
|---|---|---|
| **单图程序化纸偶小碎步（Procedural Waddle）** | 当角色处于移动状态且未配置多帧 `Flipbook` 时，表现组件自动对纸片施加轻量正弦摇摆（`Roll ±6°`）与颠簸小跳（`Local Z +0~7cm`），停止移动立即归零接地 | 哪怕每个角色只有 **1 张手绘 PNG**，跑起来也像手工纸偶剧一样呆萌欢脱，零动画帧阻塞！ |
| **双纸片错位贴合背人（Dual-Card Piggyback）** | 若未单独绘制合体护送图，直接启用 `UDCPaperVisualComponent` 已有的 `BuddyCardMeshComp` 副卡槽，将尖叫裤衩哥（`rescue.png` 去火焰主体）斜挂贴合在消防员背后同速蹦跶移动；若配置了独立合体图则自动优先切换 | **零新增画图负担**即可直接用现有 `firefighter.png` + `rescue.png` 跑通最爆笑的“背着尖叫裤衩男狂奔”画面！ |
| **主角 `VisualMode` 切换开关** | 保留 `2D_Paper`（默认正式贴纸纸片）与 `3D_Mesh`（几何白模救急/对比）一键切换枚举 | 保障编程与美术完全并行互不卡脖子 |

---

## 3. 原画拆层复用表（1 张图拆出角色 + 场景特效！） `[已确认]`

现有原画中自带极具表现力的手绘水花与火焰，切图导入 `/Game/Sprites/` 时按以下规则拆层，直接省去特效制作成本：

| 源文件 (`assets/characters/`) | 拆出的角色主体贴图 (`Bottom-Center` 接地) | 额外白赚的手绘场景/特效贴图 |
|---|---|---|
| **`firefighter.png`** | `T_Firefighter_Base`（方脸消防员单人） / `T_Firefighter_WithTool`（怀抱红灭火器） | **`T_VFX_WaterSplash_Crayon`**：拆出右侧大片天蓝色蜡笔飞溅水花，仅在按住 `E` 喷射灭火（`Spraying`）时于枪口挂点显示并轻微抖动缩放！ |
| **`rescue.png`** | `T_Civilian_Scream`（蓝底粉爱心裤衩 + 单只黄拖鞋尖叫男主体；受伤态可叠一层浅炭灰脏污色或切图） | **`T_Env_Fire_Crayon_A` & `T_Env_Fire_Crayon_B`**：拆出人物左右两侧的红底黄线蜡笔火焰团，直接作为室内 `F1 / F2 / F3` 三处火点的正式 2D 纸片火舌！ |
| **`witch.png`** | `T_CaptainWitch_Block`（紫尖帽、彩带金麻花辫、星星魔杖、红高跟魔女队长） | — |
| **`little girl.png`** | `T_NPC_MatchGirl`（满地火柴梗 + 背带补丁裤小女孩 + 头顶自带烤鸡幻想气泡，或将烤鸡气泡拆为常驻小挂件） | 可选拆出脚边火柴梗做地面小装饰 |
| **`dog.png`** | `T_NPC_AngelDog`（蓝光环 + 小翅膀卷毛天使修狗，用于 `Z_Safe` 门外安全区悬浮呼吸迎宾 & 手账贴纸） | — |

---

## 4. 锚点与命名规范

- **统一命名前缀**：地图 `L_`、数据资产 `DA_`、贴图 `T_`、Sprite `S_`、Flipbook `FB_`、材质 `M_ / MI_`、UI `WBP_`。
- **Pivot 铁律**：所有地面人物与火焰贴纸统一设在**底边正中心（`Bottom-Center`，双脚/火底接地处）**；竖直立于地面（`Pitch=0, Roll=0`），左右变向通过 `FlipU` 水平镜像翻转（原画中均无任何字母数字，100% 翻转安全！）。
