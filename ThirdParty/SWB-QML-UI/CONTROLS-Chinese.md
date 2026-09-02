# SwbControls 控件参考

[English](CONTROLS.md) | 简体中文

本库将 `QtQuick.Controls.Basic` 中的全部 51 个可视控件重绘为 shadcn Base UI 的简洁黑白风格。所有控件都位于同一个 QML 模块中：

```qml
import SwbControls
```

## 快速开始

```qml
import QtQuick
import SwbControls

Window {
    width: 420
    height: 240
    visible: true
    color: SwbTheme.background

    Column {
        anchors.centerIn: parent
        spacing: 12

        SwbTextField { placeholderText: "Email" }
        SwbButton { text: "Subscribe"; onClicked: console.log("clicked") }
        SwbSwitch { text: "Dark mode"; onToggled: SwbTheme.darkMode = checked }
    }
}
```

所有控件都保留其 Qt 基类的 API（例如 `Button` 的 `text`/`clicked`、`Slider` 的 `value`/`moved` 等），并在此基础上增加少量样式属性，其中最常用的是：

- `variant` —— 视觉强调级别，例如 `"default" | "destructive" | "outline" | "secondary" | "ghost" | "link"`
- `size` —— `"sm" | "default" | "lg"`

```qml
SwbButton { text: "Delete"; variant: "destructive"; size: "sm" }
```

## 主题 —— `SwbTheme` 单例

`SwbTheme` 是一个模块单例：导入模块后即可在任意位置直接使用，无需额外配置。所有控件都从中读取颜色与尺寸参数。

```qml
// 默认跟随系统深浅色；随时赋值即可覆盖。
SwbTheme.darkMode = true

// 在自己的界面中使用相同的令牌，与控件保持一致。
Rectangle {
    color: SwbTheme.background
    border.color: SwbTheme.border
    radius: SwbTheme.radius
}
```

浅色与深色调色板可以分别修改；切换 `darkMode` 后仍会使用对应的自定义调色板。尺寸令牌则可以直接赋值：

```qml
Component.onCompleted: {
    SwbTheme.lightPalette.primary = "#2563eb"
    SwbTheme.lightPalette.primaryForeground = "#ffffff"
    SwbTheme.darkPalette.primary = "#60a5fa"
    SwbTheme.radius = 6
    SwbTheme.controlHeight = 36
}
```

每个可视控件还公开一个可写的 `theme: SwbStyle` 对象。它的属性默认跟随 `SwbTheme`，只有被当前控件覆盖的属性才会脱离全局绑定，因此局部修改不会影响其他控件：

```qml
SwbButton {
    text: "Local style"
    theme.primary: "#7c3aed"
    theme.primaryForeground: "#ffffff"
    theme.radius: 18
}
```

覆写基础令牌时，该实例的派生令牌会自动级联——覆写 `theme.primary` 会重新派生 `theme.primaryHover`，覆写 `theme.radius` 会重新派生 `theme.radiusSm`——除非对应令牌在全局层被定制过，此时以全局定制为准。所有未覆写的属性继续跟随全局主题，包括深浅色切换。

固定值无法跟随深浅色切换（系统无从得知它的深色版本应该是什么）——而且你*没有*覆写的令牌（如 `primaryForeground`）仍会随模式切换，锁死的背景色可能配上另一模式的文字色。因此要么把配对令牌一起覆写，要么用 `adaptive` 同时给出两个模式的值：

```qml
SwbButton {
    // 浅色值、深色值——随深浅色模式自动切换
    theme.primary: theme.adaptive("#7c3aed", "#a78bfa")
}
```

`adaptive(light, dark)` 在每个控件的 `theme` 上（跟随该实例的 `darkMode`）和 `SwbTheme` 上（跟随全局模式）都可用，后者可用于你自己的组件。

`theme.darkMode` 本身也可以被覆写，因此控件实例可以自主决定自己的深浅色。它默认跟随 `SwbTheme.darkMode`；覆写后，这个实例的整套调色板——所有令牌、派生色以及 `adaptive` 的取值——都随之切换，且不影响任何其他控件：

```qml
SwbButton { theme.darkMode: true }                // 永远深色，比如放在深色横幅上
SwbButton { theme.darkMode: false }               // 永远浅色，全局切深色也不跟
SwbButton { theme.darkMode: card.isDarkSurface }  // 跟随你自己的条件
SwbButton { theme.darkMode: !SwbTheme.darkMode }  // 永远与全局相反
```

控件取用的仍是全局那两套调色板（包含你的定制）；覆写只决定*用哪一套*。运行时命令式切换同样可行——注意 QML 的常规规则：赋值会让该属性永久脱离原有绑定，而在这里这恰好就是"自主决定"的语义。之后想交还给全局，用 `Qt.binding` 重新挂上：

```qml
SwbButton {
    text: "切换我自己"
    onClicked: theme.darkMode = !theme.darkMode
    // 之后想重新跟随全局：
    // theme.darkMode = Qt.binding(() => SwbTheme.darkMode)
}
```

如果多个控件需要共享同一套局部样式，可以声明一个 `SwbStyle` 对象并赋给它们的 `theme` 属性：

```qml
SwbStyle { id: brandStyle; primary: "#7c3aed"; radius: 2 }

SwbButton { text: "Save"; theme: brandStyle }
SwbButton { text: "Export"; theme: brandStyle }
```

下方的颜色令牌在三层结构中同名可用：既可以在调色板上覆写（`SwbTheme.lightPalette.…`），也可以从 `SwbTheme` 读取当前生效值，还可以通过控件的 `theme` 按实例覆写。尺寸令牌存在于 `SwbTheme` 和每个控件的 `theme` 上。

除 `theme` 之外，大多数控件还把**解析后**的样式值公开为可写属性——`bgColor`、`textColor`、`radius`、`controlHeight` 等——作为一次性微调的逃生舱。注意覆写后其状态逻辑也会被替换：固定的 `bgColor` 不再响应悬停或按下。

### 颜色令牌

| 令牌 | 用途 |
|---|---|
| `primary` / `primaryForeground` | 强调色填充及其前景文字 |
| `secondary` / `secondaryForeground` | 次要填充及其前景文字 |
| `accent` / `accentForeground` | 悬停或选中状态的高亮及其前景文字 |
| `background` / `foreground` | 窗口背景与默认文字 |
| `popover` | 菜单、弹出层和对话框的背景 |
| `mutedForeground` | 次要文字和占位文字 |
| `border` | 细边框颜色 |
| `destructive` / `destructiveForeground` | 危险操作的红色及其前景文字 |
| `ring` / `focusRing` | 聚焦边框颜色与半透明焦点光环 |
| `primaryHover`, `secondaryHover`, `accentHover`, `destructiveBg`, `destructiveBgHover` | 预先计算的悬停与按下反馈填充色 |

### 尺寸令牌

| 令牌 | 默认值 | 用途 |
|---|---|---|
| `radius` / `radiusSm` | 10 / 8 | 圆角半径 |
| `fontSize` / `fontSizeSm` | 14 / 13 | 文字大小 |
| `fontWeight` | `Font.Medium` | 默认字重 |
| `controlHeight` (`Sm` / `Lg`) | 32 (28 / 36) | 各尺寸控件的高度 |
| `iconSize` | 16 | 内置图标的边长 |
| `handleSize` / `trackThickness` | 12 / 4 | 滑块类控件的手柄尺寸与轨道厚度 |
| `handleColor` | white | 两种模式下的手柄填充色 |
| `calendarCellSize` / `calendarSpacing` | 28 / 2 | 日历网格的单元格尺寸与间距 |
| `animationDuration` | 150 | 过渡动画时长（毫秒） |
| `focusRingWidth` | 3 | 焦点光环厚度 |
| `withAlpha(color, a)` | — | 返回透明度为 `a` 的 `color` |
| `adaptive(light, dark)` | — | 按当前深浅色模式返回 `light` 或 `dark` |

## 控件

### 按钮

| 控件 | 基类 | 说明与主要 API |
|---|---|---|
| `SwbButton` | `Button` | `variant: default \| destructive \| outline \| secondary \| ghost \| link`；`size: sm \| default \| lg` |
| `SwbToolButton` | `ToolButton` | 工具栏按钮；`variant: default \| outline`；`size: sm \| default \| lg`；支持可选中状态；仅有图标时自动变为正方形 |
| `SwbRoundButton` | `RoundButton` | 与 `SwbButton` 使用相同的 variant 和 size；短内容显示为圆形，较长文字显示为胶囊形 |
| `SwbDelayButton` | `DelayButton` | 长按确认按钮；`variant: default \| destructive`；支持 `size`；按住时逐渐填充，完成后发出 `activated()` 信号 |

**图标。** 以上按钮均继承标准的 `AbstractButton` 图标 API：用 `icon.source`（或 `icon.name`）设置图标，`icon.width` / `icon.height` 调整尺寸（默认取 `iconSize` 令牌，16），`display` 控制图标与文字的位置关系（`IconOnly` / `TextOnly` / `TextBesideIcon` / `TextUnderIcon`），`spacing` 调整图标与文字的间距。想让图标显示在文字右侧，可在按钮上开启 `LayoutMirroring.enabled: true`。

> **注意——图标默认会被染色。** 图标被视为单色剪影，自动染成当前 variant 的文字颜色（通过 `icon.color`），从而跟随主题与变体联动——最适合单色线条图标。彩色图片会因此变成单色色块。若要保留图标原本的颜色，请关闭染色：
>
> ```qml
> SwbButton {
>     text: "打开"
>     icon.source: "qrc:/icons/picture.svg"
>     icon.color: "transparent"   // 保留图标自身的颜色
> }
> ```

### 选择与数值输入

| 控件 | 基类 | 说明与主要 API |
|---|---|---|
| `SwbCheckBox` | `CheckBox` | 复选框，勾选标记由 Canvas 绘制；`boxSize` 设置方框边长 |
| `SwbRadioButton` | `RadioButton` | 单选按钮；`boxSize` 设置圆圈直径 |
| `SwbSwitch` | `Switch` | 开关；`size: sm \| default`；`trackWidth`/`trackHeight`/`thumbSize` 均可调整 |
| `SwbSlider` | `Slider` | 单值滑块，从起点到手柄之间显示填充轨道 |
| `SwbRangeSlider` | `RangeSlider` | 双手柄滑块，在 `first` 与 `second` 之间显示填充轨道 |
| `SwbDial` | `Dial` | 旋钮，环形轨道和进度弧由 Canvas 绘制 |
| `SwbSpinBox` | `SpinBox` | 整数步进输入框；支持编辑文字，并在一侧排列增减按钮；`stepWidth` 设置按钮宽度 |
| `SwbDoubleSpinBox` | `DoubleSpinBox` | 浮点数步进输入框；使用 `decimals` 设置精度；`stepWidth` 设置增减按钮宽度 |
| `SwbTumbler` | `Tumbler` | 滚轮选择器；高亮当前项，并在边缘显示渐隐效果；`scrollAnimationDuration` 调节吸附动画速度 |
| `SwbComboBox` | `ComboBox` | 可编辑、可搜索的下拉框；支持 `placeholderText`；输入时过滤选项，按 Enter 选择第一个匹配项；可选 `removable`（显示删除按钮，发出 `removeRequested(index)`）与 `reorderable`（按住拖拽排序，发出 `moveRequested(from, to)`）；两者均使用源模型索引，并自动调整 `currentIndex` 以保持选中项不变 |

### 文本输入

| 控件 | 基类 | 说明与主要 API |
|---|---|---|
| `SwbTextField` | `TextField` | 单行输入框，带焦点光环和占位文字 |
| `SwbSearchField` | `SearchField` | 搜索框，包含搜索图标和清除按钮 |
| `SwbTextArea` | `TextArea` | 多行输入框 |
| `SwbTextEditingContextMenu` | — | 统一样式的右键编辑菜单（撤销/重做、剪切/复制/粘贴/删除、全选，图标均由 Canvas 绘制）。已内置于六个文本编辑控件（`SwbTextField`、`SwbTextArea`、`SwbSearchField`、`SwbSpinBox`、`SwbDoubleSpinBox`、`SwbComboBox`）；也可通过 `ContextMenu.menu` 和它的 `editor` 属性绑定到自定义编辑器 |

### 显示与反馈

| 控件 | 基类 | 说明与主要 API |
|---|---|---|
| `SwbLabel` | `Label` | 使用主题字号、字重和禁用状态的文本标签 |
| `SwbProgressBar` | `ProgressBar` | 进度条；支持 `indeterminate` 不确定进度模式；滚出视口时可用 `animationPaused` 暂停循环动画 |
| `SwbBusyIndicator` | `BusyIndicator` | 由 Canvas 绘制的旋转圆弧；支持 `size: sm \| default \| lg` 及自定义 `diameter`、`color`；滚出视口时可用 `animationPaused` 暂停旋转 |
| `SwbPageIndicator` | `PageIndicator` | 页面圆点指示器，突出显示当前页；默认允许交互 |

### 菜单

| 控件 | 基类 | 说明与主要 API |
|---|---|---|
| `SwbMenu` | `Menu` | 带圆角、淡入与缩放动画的弹出菜单，支持完整键盘交互 |
| `SwbMenuItem` | `MenuItem` | 菜单项；`variant: default \| destructive`；`shortcutText` 可显示右对齐快捷键提示；`inset` 可预留图标空间；支持子菜单和勾选标记 |
| `SwbMenuSeparator` | `MenuSeparator` | 用于分组的细分隔线 |
| `SwbMenuBar` | `MenuBar` | 窗口内菜单栏，支持鼠标与键盘导航 |
| `SwbMenuBarItem` | `MenuBarItem` | 带悬停和打开状态的菜单栏项目 |

### 弹出层与遮罩

| 控件 | 基类 | 说明与主要 API |
|---|---|---|
| `SwbDialog` | `Dialog` | 模态对话框：标题、关闭按钮（`closeButtonVisible`）、`standardButtons`、变暗遮罩以及淡入和缩放动画 |
| `SwbDialogButtonBox` | `DialogButtonBox` | 标准按钮栏；根据按钮角色应用主要、描边或危险样式 |
| `SwbPopup` | `Popup` | 通用圆角浮动面板；支持淡入和缩放动画、可选模态遮罩以及点击外部关闭 |
| `SwbDrawer` | `Drawer` | 可从四条边缘打开的抽屉；尺寸根据边缘自适应，并带可选拖动手柄（`handleVisible`）、遮罩和滑入动画 |
| `SwbToolTip` | `ToolTip` | 使用反色的气泡提示，带指示箭头、最大宽度换行以及淡入和缩放动画 |

### 导航与分页

| 控件 | 基类 | 说明与主要 API |
|---|---|---|
| `SwbTabBar` | `TabBar` | 标签栏；`variant: default \| line`；带共享的选中高亮移动动画；通过 `currentIndex` 与视图关联 |
| `SwbTabButton` | `TabButton` | 带选中、悬停和禁用状态的标签按钮；自动继承所在 `SwbTabBar` 的 `variant` |
| `SwbStackView` | `StackView` | 页面导航容器，支持 push、pop 和 replace；`orientation: Qt.Horizontal \| Qt.Vertical` 设置滑动方向 |
| `SwbSwipeView` | `SwipeView` | 可滑动页面，可通过索引与指示器或标签栏联动 |

### 容器与布局

| 控件 | 基类 | 说明与主要 API |
|---|---|---|
| `SwbFrame` | `Frame` | 卡片容器，带背景、边框、圆角和内边距 |
| `SwbGroupBox` | `GroupBox` | 带标题的无边框语义分组 |
| `SwbPane` | `Pane` | 使用次要背景的无边框面板 |
| `SwbPage` | `Page` | 使用主题背景并保留原生 `header`/`footer` 插槽的页面 |
| `SwbScrollView` | `ScrollView` | 带统一样式滚动条和键盘焦点光环的滚动视图 |
| `SwbSplitView` | `SplitView` | 可拖动调整的横向或纵向分割布局 |
| `SwbToolBar` | `ToolBar` | 横向或纵向工具栏；`floating: true` 时由贴边直角切换为浮动圆角 |
| `SwbToolSeparator` | `ToolSeparator` | 根据方向调整的分隔线 |

### 滚动辅助

| 控件 | 基类 | 说明与主要 API |
|---|---|---|
| `SwbScrollBar` | `ScrollBar` | 纤细的圆角滚动条，悬停时加深，支持横向与纵向 |
| `SwbScrollIndicator` | `ScrollIndicator` | 不可交互的位置提示，滚动停止后淡出 |

### 日历

| 控件 | 基类 | 说明与主要 API |
|---|---|---|
| `SwbMonthGrid` | `MonthGrid` | 月份网格：点击某天即写入 `selectedDate`、高亮今天、淡化非本月日期 |
| `SwbDayOfWeekRow` | `DayOfWeekRow` | 星期标题行，与月份网格的列对齐 |
| `SwbWeekNumberColumn` | `WeekNumberColumn` | 周数列，与月份网格的行对齐 |

### 表格辅助

| 控件 | 基类 | 说明与主要 API |
|---|---|---|
| `SwbHorizontalHeaderView` | `HorizontalHeaderView` | 列表头：文字左对齐、底部分隔线，并为选中列显示轻微高亮 |
| `SwbVerticalHeaderView` | `VerticalHeaderView` | 行表头：文字居中、右侧分隔线，并为选中行显示轻微高亮 |
| `SwbSelectionRectangle` | `SelectionRectangle` | 表格拖动选区；角部手柄与滑块手柄样式一致 |

## 用法示例

### Variant 与 size

```qml
Row {
    spacing: 8
    SwbButton { text: "Save" }                            // solid primary
    SwbButton { text: "Cancel"; variant: "outline" }
    SwbButton { text: "Delete"; variant: "destructive" }
    SwbButton { text: "Docs"; variant: "link"; size: "sm" }
}
```

### Dialog

`Dialog.Ok` 形式的枚举值来自基础模块，因此还需要导入 `QtQuick.Controls.Basic`：

```qml
SwbButton {
    text: "Open dialog"
    onClicked: dialog.open()
}

SwbDialog {
    id: dialog
    title: "Confirm"
    standardButtons: Dialog.Ok | Dialog.Cancel
    anchors.centerIn: parent

    SwbLabel { text: "Apply these changes?" }
}
```

### 带危险操作的菜单

```qml
SwbMenu {
    id: contextMenu
    SwbMenuItem { text: "Rename"; shortcutText: "F2" }
    SwbMenuItem { text: "Duplicate" }
    SwbMenuSeparator {}
    SwbMenuItem { text: "Delete"; variant: "destructive" }
}
```

### 组合日历

`SwbDayOfWeekRow`、`SwbWeekNumberColumn` 和 `SwbMonthGrid` 使用确定的共享单元格尺寸。存在周数列时，应根据实际控件宽度设置星期行的占位和宽度，这样使用局部主题覆写时也能保持对齐：

```qml
Column {
    spacing: SwbTheme.calendarSpacing

    Row {
        spacing: SwbTheme.calendarSpacing
        Item { width: weekNumbers.width; height: 1 }
        SwbDayOfWeekRow { width: grid.width }
    }
    Row {
        spacing: SwbTheme.calendarSpacing
        SwbWeekNumberColumn { id: weekNumbers; month: grid.month; year: grid.year }
        SwbMonthGrid { id: grid; month: 5; year: 2026 }  // 点击某天 → selectedDate
    }
}
```

### 带表头和拖动选区的表格

表头视图通过 `syncView` 跟随表格；拖动选区需要为表格设置 `selectionModel`（`ItemSelectionModel` 来自 `QtQml.Models`，`SelectionRectangle.Drag` 枚举来自 `QtQuick.Controls.Basic`）：

```qml
SwbHorizontalHeaderView {
    id: hHeader
    anchors.left: table.left; anchors.top: parent.top
    syncView: table
    model: ["Invoice", "Status", "Amount"]
    clip: true
}
SwbVerticalHeaderView {
    id: vHeader
    anchors.left: parent.left; anchors.top: table.top
    syncView: table
    clip: true
}
TableView {
    id: table
    anchors { left: vHeader.right; top: hHeader.bottom; right: parent.right; bottom: parent.bottom }
    clip: true
    model: myTableModel
    selectionModel: ItemSelectionModel { model: table.model }
    delegate: myCellDelegate
}
SwbSelectionRectangle { target: table; selectionMode: SelectionRectangle.Drag }
```

### 为自定义编辑器添加右键编辑菜单

六个文本编辑控件已经内置统一样式菜单。若要将其绑定到其他可编辑项目：

```qml
import QtQuick.Templates as T

TextInput {
    id: myEditor
    T.ContextMenu.menu: SwbTextEditingContextMenu { editor: myEditor }
}
```
