# Director's Cut — Game Jam 原型

这是围绕「删除记忆 → 心理限制消失 → 允许进入 → 改变救援结果」制作的单关灰盒。
UE 5.8.4 / macOS Apple Silicon。当前环境没有 Unreal Editor MCP 或 UE 专用 Skills，因此使用原生 C++、UMG 和编辑器 Python 生成真实资产。没有调用 LLM，也没有添加复杂 AI、存档或自由文字编辑。

## 开始试玩

本机也可以双击 `Play.command`，直接用已安装编辑器启动独立游戏预览窗口。它仍依赖本机 UE 与已编译模块，不是发行安装包。

1. 用 UE 5.8.4 打开 `DirectorCutJam.uproject`。首次打开若提示构建模块，选择构建。
2. 默认地图为 `/Game/Maps/Firefighter`；Play 下拉菜单选择 **New Editor Window (PIE)**，点击游戏窗口获取键盘焦点。工程默认配置也设置为此模式。
3. WASD 移动，靠近入口标记，按 E 尝试进入。原始 Persona 会拒绝进入，并显示头顶思想泡泡。
4. Tab 打开 Persona，点击删除惩罚经历的整块文字，再关闭面板。重新按 E 可以进入。
5. 靠近 RESCUE / EXTINGUISH 标记按 E。两个目标均完成后显示结果；否则测试计时结束时结算。
6. R 或结果界面的 Restart 重新体验原始场景。这个按钮恢复 Persona、NPC、火灾、受困者和玩家位置。

调试：F6 恢复 Persona；F7 切换是否显示额外记忆原因；F8 立即结束当前测试演出。macOS 可能需要 Fn 才能发送功能键。调试恢复不是已决定的正式撤销机制。

## 最小结构

```text
UDCPersona（文字块、活动状态、效果查询、变化事件）
    ↓ 查询有效 EffectID
ADCInteraction.TryInteract（距离 → 场景条件 → Persona → 执行/思想泡泡）
    ↕
ADCPlayer（移动、输入、当前区域、头顶 UMG）
    ↕
UDCOverlay（Persona、交互提示、结果、调试信息）

ADCDirector（固定 NPC 事件、灰盒、火灾/救援事实）
    → EvaluateStars（只读取结果事实，不执行场景行动）
```

| 文件 | 职责 |
| --- | --- |
| `Source/DirectorCutJam/DCPersona.h/.cpp` | 块 ID、文本、删除/重排许可、活动状态、EffectID；删除、查询、恢复、事件 |
| `Source/DirectorCutJam/DCPlayer.h/.cpp` | 同一个消防员 Character；原生移动与按键；思想泡泡 WidgetComponent |
| `Source/DirectorCutJam/DCScene.h/.cpp` | 交互入口检查、固定事件序列、灰盒、火灾/救援状态与结果查询 |
| `Source/DirectorCutJam/DCUI.h/.cpp` | 原生创建的真实 UMG Widgets；按钮调用 Persona API |
| `Source/DirectorCutJam/DCValidation.cpp` | Editor 内的 PIE 自动验收，仅编辑器开发构建包含 |
| `Scripts/create_map.py` | 通过 Unreal API 创建真实地图及八个基础材质；默认不会覆盖已存在地图 |
| `Content/Maps/Firefighter.umap` | 已由 Unreal 保存的关卡，包含场景控制器、光源和 PlayerStart |

核心类与配置均向 Blueprint 暴露；本版没有制作 Blueprint Event Graph 或 Widget Blueprint 资产，UI 由 C++ 构造 UMG。需要修改文字块时，可以派生 `DCPlayer` Blueprint，在 Persona 组件的 Initial Blocks 中编辑，然后在自定义 GameMode 的 Default Pawn Class 中选用它。场景交互的最终权限仍在 TryInteract 中检查，不依赖 UI 隐藏按钮。

## 测试占位与需要确认的设计

选中地图中的 `DC_TestScene_Adjust_Placeholders_Here`，在 Details 的 `TEST PLACEHOLDERS` 分类调整下列参数。它们是测试参数，不是确定的游戏规则。

| 未决/临时项 | 当前测试占位 | 如何替换 |
| --- | --- | --- |
| 编辑时机 | 演出中可编辑，不暂停计时 | `Allow Editing During Take`；关闭后先开 Persona，关闭面板才开始计时，此后不能重开 |
| 提示强度 | 只显示服从命令的想法 | `Show Memory Reason` 或 F7；开启后附加 “Because of what happened last time.” |
| 正式删除撤销 | 没有玩家撤销按钮；仅整场重开及调试恢复 | 后续确认；F6 可通过 `Enable Debug Controls` 禁用 |
| 时间/失败条件 | 20 秒上司移动并继续命令；45 秒火块变大；120 秒演出结束 | `Captain Followup Seconds` / `Fire Expansion Seconds` / `Take Duration Seconds` |
| 简单救援 | 在标记范围按 E 即刻救出，隐藏受困者灰盒 | `ADCInteraction` 的 Rescue 分支；尚未设计携带/护送等机制 |
| 简单灭火 | 在标记范围按 E 即刻灭火；不要求水源或工具 | `ADCInteraction` 的 Extinguish 分支 |
| 进入/退出 | 成功交互后移到门另一侧；边界碰撞防止走路绕过检查 | Enter / Exit 分支中的目标坐标；用于权限验证，未决定最终过门表现 |
| 结算时机 | 两目标都完成即结算；只灭火时仍可继续救人，截止后结算 1 星 | `FinishTake` 调用点 |
| 星级 | 未灭火 0 星；只灭火 1 星；灭火且救援 2 星 | `EvaluateStars` 为独立查询；3 星仅 `QualifiesForDesignedThirdStar` 扩展事件，默认不会授予 |
| NPC 额外反应 | 固定命令与固定移动，无临时设计的额外反应 | 确认后追加；当前序列不读取 Persona |
| 重排、多效果 | 保留 CanReorder 字段；单块单 EffectID | 未实现重排；多正负效果如何处理待设计确认 |

## 验收与限制

实际结果见 `VALIDATION.md`；交付的机器报告、日志和截图在 `Validation/`，引擎原始报告在 `Saved/Validation/Report/index.json`。不要把静态代码检查、编译成功、PIE 自动化与真人完整试玩混为一谈。

未制作：三星谜题、正式人物动画/火灾特效、复杂 NPC 反应、正式撤销规则、打包发行版本、陌生玩家理解度测试。当前 UI 文案使用任务书的英语示例。

已知限制：本机 UE 5.8.4/macOS 27 的嵌入式编辑器视口在结束 PIE 时出现过间歇性的 `GameViewport.IsUnique()` 断言；UI 清理后仍复现，根因尚未确认。默认使用独立 PIE 窗口，验收也采用这一模式；不要把独立窗口通过视为嵌入式模式已修复。

## 本机重建

```sh
'/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh' \
  DirectorCutJamEditor Mac Development '/绝对路径/DirectorCutJam.uproject' -WaitMutex
```

已交付真实地图，正常打开无需再运行生成脚本。地图丢失时，在编辑器 Output Log 的 Python 模式运行：

```python
exec(open('/绝对路径/DirectorCutJam/Scripts/create_map.py').read())
```

PIE 自动验收可在 Session Frontend → Automation 中运行 `DirectorCut.PIE.Acceptance`。测试启动 PIE，使用当前地图中的真实类、UMG 回调与交互；部分定位通过测试代码移动角色，部分时间通过推进测试时钟加速，不代表真人已经走完全部路线。
