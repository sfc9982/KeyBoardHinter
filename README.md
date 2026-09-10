# KeyBoardHinter

一个 Windows 系统托盘小工具：当你在打字时按下 **Caps Lock / Num Lock / Scroll Lock**，
屏幕底部会浮现一张小卡片，显示该锁定键是**开**还是**关**，随后自动淡出隐藏。

A Windows tray utility: when you press **Caps Lock / Num Lock / Scroll Lock** while typing,
a small card slides up near the bottom of the screen showing whether that lock key is **On**
or **Off**, then hides by itself.

---

## 功能 Features

- 监听三个锁定键的释放事件，实时显示开关状态
  Listens for the three lock-key releases and shows the new On/Off state instantly.
- 锁体图标随开关状态开合（闭合 = On，打开 = Off）
  The padlock icon opens or closes to reflect the On/Off state.
- 每个键有专属图标：双 A（Caps Lock）、数字 1（Num Lock）、上下箭头（Scroll Lock）
  Each key has its own icon: double-A (Caps), digit 1 (Num), up/down arrows (Scroll).
- 托盘图标 + 右键菜单：开机自启、显示当前状态、退出
  Tray icon with a right-click menu: auto-start at login, show current state, quit.
- 单实例运行：重复启动会提示"已在运行"
  Single instance: a second launch shows "already running".
- 使用 `SW_SHOWNOACTIVATE` 显示，绝不抢走当前活动窗口的键盘焦点
  Shown with `SW_SHOWNOACTIVATE` — never steals keyboard focus from the active window.
- Num Lock 相关提示优先于 Caps Lock 显示
  A Num Lock hint takes display priority over Caps Lock.
- 配置持久化到 `%APPDATA%\KeyBoardHinter\KeyBoardHinter.ini`
  Settings persist to `%APPDATA%\KeyBoardHinter\KeyBoardHinter.ini`.

---

## 前提 Prerequisites

- Windows 10/11（64 位）
- [wxWidgets 3.x](https://www.wxwidgets.org/) 预编译包（vc14x, x64）放在仓库的 `wxWidgets/` 目录
  A wxWidgets 3.x prebuilt package (vc14x, x64) placed in the repo's `wxWidgets/` folder.
  - 目录布局：`wxWidgets/include`、`wxWidgets/lib/vc14x_x64_dll`
  - Layout: `wxWidgets/include`, `wxWidgets/lib/vc14x_x64_dll`
- CMake 3.16+，以及 MSVC（Visual Studio）工具链
  CMake 3.16+ and the MSVC (Visual Studio) toolchain.

---

## 构建 Build

【CMake】推荐方式（支持脚本化、自带单元测试）：

```
cmake -S . -B build
cmake --build build --config Release
```

可执行文件位于 `build/Release/KeyBoardHinter.exe`（wxWidgets 的 DLL 构建后会自动复制过去）。
The executable lands at `build/Release/KeyBoardHinter.exe` (wxWidgets DLLs are copied automatically).

【MSVC】也可以直接用 `KeyBoardHinter.vcxproj`（v141 工具集）在 Visual Studio 中打开构建。
Alternatively, open `KeyBoardHinter.vcxproj` (toolset v141) in Visual Studio.

---

## 运行与使用 Run / Use

运行 `KeyBoardHinter.exe`，程序只驻留于系统托盘，没有主窗口。

- 按下任一锁定键 → 屏幕底部弹卡提示开关状态
- 左键单击托盘图标 → 显示三个锁定键的当前总览
- 托盘右键菜单：
  - **开机自启**：勾选后随 Windows 登录自动启动
  - **显示当前状态**：同左键单击
  - **退出**：结束程序

---

## 配置 Configuration

配置文件默认不存在，首次启动时自动生成带注释的模板，位于：

```
%APPDATA%\KeyBoardHinter\KeyBoardHinter.ini
```

| 键 Key            | 默认 Default | 含义 Meaning                                          |
| ----------------- | ------------ | ----------------------------------------------------- |
| `hideDelayMs`     | `1000`       | OSD 停留时间（毫秒）/ how long the OSD stays (ms)     |
| `windowWidth`     | `160`        | OSD 宽度（DIP）/ OSD width (DIP)                      |
| `windowHeight`    | `112`        | OSD 高度（DIP）/ OSD height (DIP)                     |
| `verticalPosNum`  | `7`          | 纵向位置分子 / vertical position numerator            |
| `verticalPosDen`  | `8`          | 纵向位置分母 / vertical position denominator          |

纵向位置 = 屏幕高度 × `verticalPosNum / verticalPosDen`（默认即屏幕靠下 7/8 处）。
Vertical position = screen height × `verticalPosNum / verticalPosDen` (default bottom 7/8).

> 改配置需重启程序生效。注释行以 `;` 或 `#` 开头。
> Changes take effect on restart. Comment lines start with `;` or `#`.

---

## 单元测试 Tests

纯逻辑模块不依赖 wxWidgets 与窗口，可脱离 GUI 运行，由 CTest 驱动：

```
ctest --test-dir build -C Debug --output-on-failure
```

- **LockStateTest** — 锁定键状态机：各键切换、Num Lock 优先级、启动快照
- **ConfigTest** — 配置读写：往返、注释/空白/非法值处理、Sanitize（写入构建目录临时文件，不碰真实配置）
- **PlacementTest** — 悬浮窗定位的整数运算
- **OSDWindowTest** — 把卡片离屏渲染到位图后逐像素校验图标与文字绘制（无需显示窗口）

---

## 项目结构 Project Layout

```
KeyBoardHinter/
├─ main.cpp        入口：装配各模块（单实例、DPI、钩子接线）
├─ OSDWindow.{h,cpp}  悬浮卡片窗口与绘制（矢量图标、概览文案）
├─ TrayIcon.{h,cpp}   托盘图标与右键菜单
├─ KeyboardHook.{h,cpp}  WH_KEYBOARD_LL 低级键盘钩子
├─ AutoStart.{h,cpp}   开机自启（注册表 Run 项）
├─ Config.{h,cpp}      配置读写
├─ LockState.{h,cpp}   锁定键状态机（纯逻辑，可测）
├─ Placement.h         悬浮窗定位整数运算（纯逻辑，可测）
├─ *Test.cpp           各模块单元测试
├─ CMakeLists.txt
└─ KeyBoardHinter.vcxproj   MSVC 备用工程（v141）
```

分层 `layering`：
- **纯逻辑（不含 wxWidgets）**：`Config`、`LockState`、`Placement`
- **UI / Win32 交互**：`OSDWindow`、`TrayIcon`、`KeyboardHook`、`AutoStart`
- **装配**：`main.cpp` 只负责把上述模块接起来

---

## 已知局限 Known Limitations

- 低级键盘钩子收不到发送给**更高权限（管理员）进程**的按键，因而在那些窗口里打字时不会触发提示。
  A low-level hook cannot capture keys sent to higher-privilege (admin) processes, so it won't fire while typing into those windows.
- 打开、关闭状态卡片需待按键**释放**才显示，避免自动重复触发。
  The card appears on key **release** so key auto-repeat doesn't spam it.

---

## License

未指定（自用工具）。Unless noted otherwise, this is a self-use utility with no license.
