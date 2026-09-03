# QtEasyTier 对 SWB-QML-UI 的本地修改记录

上游来源：<https://github.com/xxmzwf/SWB-QML-UI>（MIT License，作者 xxmzwf）。
本目录是 vendored 副本，升级上游时请对照本文件重新套用以下修改。

## 目录裁剪（非代码修改）

- 删除 `examples/`、`assets/`（截图）与 `docs/` 目录；`SWB_BUILD_EXAMPLES` 嵌入时本就默认关闭。
- `docs/CONTROLS.md`、`docs/CONTROLS-Chinese.md`、`docs/README-Chinese.md` 提升到库根目录。

## 新增文件（上游不存在）

- `assets/theme.qrc` + `assets/sun.svg` + `assets/moon.svg`：SwbTheme 明暗切换专属图标
  （取自上游 `examples/assets/icons/`，QtEasyTier 侧边栏主题切换按钮使用），
  资源前缀 `/swb/theme`（即 `qrc:/swb/theme/sun.svg`）。
  注意：上游原有的 `assets/`（截图）已被裁剪删除，现目录为 QtEasyTier 新建的图标资源目录。
  该 qrc 由宿主根 `CMakeLists.txt` 加入 `appQtEasyTier` 源列表经 AUTORCC 编译，
  未改动库自身的 `components/CMakeLists.txt`。

## CMakeLists.txt 补丁

1. **Qt 版本要求 6.10 → 6.8**：`qt_standard_project_setup(REQUIRES 6.8)`。
   宿主仓库（QtEasyTier）及其 CI 基于 Qt 6.8；库内版本敏感 API
   （`Popup.popupType`、`T.ContextMenu` 附加属性）均自 Qt 6.8 起可用，
   上游仅声明未测试 6.8。
2. **QT_PATH 默认值**：上游默认硬编码 Windows 路径 `D:/Code/Libs/Qt/...` 并无条件追加到
   `CMAKE_PREFIX_PATH`；改为默认为空，且仅在路径存在时追加。
3. **移除 QuickVectorImage 组件**：`find_package` 与 `target_link_libraries(SwbControls)`
   不再引用 `QuickVectorImage` / `QuickVectorImageHelpers`（配合下方 SwbIconLabel 源码补丁）。
4. **移除 QuickEffects 组件**：库根 `find_package(Qt6 REQUIRED COMPONENTS Quick QuickEffects)`
   改为仅 `Quick`，`target_link_libraries(SwbControls)` 移除 `Qt6::QuickEffects`
   （aqt 默认归档与 deepin crimson 源的 Qt 6.8 均不含 Qt6QuickEffects 开发配置）。

## 源码补丁

- **`components/SwbIconLabel.qml`**：移除 `import QtQuick.VectorImage(.Helpers)`、
  `useCurveRenderer`/`hasExplicitIconColor` 属性与 `vectorIconComponent`（CurveRenderer 路径），
  SVG 图标统一经 `IconImage`（QtQuick.Controls.impl，随 QtQuick 标配）按 DPR 栅格化渲染。
  原实现中 VectorImage 仅服务"Windows 且图标未着色"的分支，其余平台本就走 IconImage/IconLabel。
- **`components/SwbProgressBar.qml`**：移除 `import QtQuick.Effects`、`MultiEffect`
  渲染块与仅为遮罩服务的 `progressMask`（layer 纹理），indeterminate 扫掠动画源
  （本身已带圆角 + `clip: true`）改为直接显示。
- **`components/SwbTextEditingContextMenu.qml`**：七个文本编辑动作
  （`UndoAction`/`RedoAction`/`CutAction`/`CopyAction`/`PasteAction`/`DeleteAction`/
  `SelectAllAction`）为 Qt 6.9 才引入 QtQuick.Controls.impl 的类型，Qt 6.8 上报
  `RedoAction is not a type`，导致菜单连同所有文本控件整体加载失败。改为普通
  `Action` 直调编辑器既有 API（`undo/redo/cut/copy/paste/remove/selectAll` 与
  `canUndo/canRedo/canPaste/selectedText/selectionStart/selectionEnd`，Qt 5 起即有，
  TextInput 与 TextArea 通用），`icon.name` 保留以驱动 Canvas 手绘图标；菜单项文案
  为中文 qsTr 源串（vendored 库不纳入宿主 .ts 翻译体系）。

## 宿主构建接线（记录，不在本目录内）

- 宿主根 `CMakeLists.txt`：`add_subdirectory(ThirdParty/SWB-QML-UI)`，并（仅非 DDE 构建）
  `target_link_libraries(appQtEasyTier PRIVATE SwbControls SwbControlsplugin)`。
  `find_package(Qt6 ...)` 不要求 `QuickEffects`（SWB 补丁后零依赖，且 aqt 默认归档与
  crimson 均无此组件开发配置）。
- **静态集成**（按上游 README Option 1 的静态变体）：宿主在 `add_subdirectory` 之前
  `set(BUILD_SHARED_LIBS OFF)`，SwbControls 与自动生成的 QML 插件均成为静态库，
  编译产物全部链入可执行文件——部署不再携带 `libSwbControls.so`，
  打包脚本对 `*.a` 的既有跳过规则即覆盖静态库产物。
  静态集成必须同时链接 `SwbControlsplugin`（QML 插件与类型注册），否则模块加载即失败。
- QML 侧直接 `import SwbControls`，模块资源位于 `qrc:/qt/qml/SwbControls`，
  引擎默认路径即可解析。
- 运行时 QML 依赖仅 QtQuick / QtQuick.Controls(.Basic/.impl) / QtQuick.Layouts——
  均为发行版标配/常规拆包，无 addon 依赖；打包 deb 无需额外 VectorImage /
  QuickEffects 依赖（见"源码补丁"）。
