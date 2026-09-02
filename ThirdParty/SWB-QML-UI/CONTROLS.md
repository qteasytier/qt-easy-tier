# SwbControls Component Reference

English | [简体中文](CONTROLS-Chinese.md)

All 51 visual controls from `QtQuick.Controls.Basic` restyled in the shadcn Base UI look. Everything lives in one QML module:

```qml
import SwbControls
```

## Quick start

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

Controls keep their Qt base-type API (`Button`'s `text`/`clicked`, `Slider`'s `value`/`moved`, …) and add a few styling properties on top, most commonly:

- `variant` — visual emphasis, e.g. `"default" | "destructive" | "outline" | "secondary" | "ghost" | "link"`
- `size` — `"sm" | "default" | "lg"`

```qml
SwbButton { text: "Delete"; variant: "destructive"; size: "sm" }
```

## Theming — the `SwbTheme` singleton

`SwbTheme` is a module singleton: import the module and it is available everywhere, no setup required. Every control reads its colors and metrics from it.

```qml
// Follows the system color scheme by default; assign to override at any time.
SwbTheme.darkMode = true

// Use the same tokens in your own UI so it matches the controls.
Rectangle {
    color: SwbTheme.background
    border.color: SwbTheme.border
    radius: SwbTheme.radius
}
```

Customize the light and dark palettes independently; switching `darkMode` keeps using the corresponding customized palette. Metric tokens are writable directly:

```qml
Component.onCompleted: {
    SwbTheme.lightPalette.primary = "#2563eb"
    SwbTheme.lightPalette.primaryForeground = "#ffffff"
    SwbTheme.darkPalette.primary = "#60a5fa"
    SwbTheme.radius = 6
    SwbTheme.controlHeight = 36
}
```

Every visual control also exposes a writable `theme: SwbStyle` object. Its properties follow `SwbTheme` until that control overrides them, so a local change never affects other controls:

```qml
SwbButton {
    text: "Local style"
    theme.primary: "#7c3aed"
    theme.primaryForeground: "#ffffff"
    theme.radius: 18
}
```

Overriding a base token cascades to its derived tokens on that instance — `theme.primary` re-derives `theme.primaryHover`, `theme.radius` re-derives `theme.radiusSm` — unless the corresponding token was customized globally, in which case the global customization wins. Everything you did not override keeps following the global theme, including dark-mode switches.

A plain fixed value cannot follow dark mode (nothing can guess its dark variant) — and the tokens you did *not* override (such as `primaryForeground`) keep switching, so a pinned background can end up paired with the other mode's text color. Either override the paired tokens together, or pass both mode values through `adaptive`:

```qml
SwbButton {
    // light value, dark value — keeps switching with dark mode
    theme.primary: theme.adaptive("#7c3aed", "#a78bfa")
}
```

`adaptive(light, dark)` exists on each control's `theme` (follows that instance's `darkMode`) and on `SwbTheme` (follows the global mode) for use in your own components.

Since `theme.darkMode` is itself overridable, a control can decide its own color scheme. It defaults to following `SwbTheme.darkMode`; override it and this one instance switches its entire palette — every token, the derived colors, and `adaptive` results included — without affecting any other control:

```qml
SwbButton { theme.darkMode: true }                // always dark, e.g. on a dark hero banner
SwbButton { theme.darkMode: false }               // always light, even when the app goes dark
SwbButton { theme.darkMode: card.isDarkSurface }  // follow your own condition
SwbButton { theme.darkMode: !SwbTheme.darkMode }  // always the opposite of the app
```

The control still draws from the two global palettes (including your customizations); the override only decides *which* of them applies. Imperative switching works too — with the usual QML rule that an assignment permanently detaches the property from its binding, which here is exactly the self-governing semantic. Hand control back to the app with `Qt.binding` when needed:

```qml
SwbButton {
    text: "Toggle myself"
    onClicked: theme.darkMode = !theme.darkMode
    // Re-follow the app later:
    // theme.darkMode = Qt.binding(() => SwbTheme.darkMode)
}
```

To share one local style between several controls, declare an `SwbStyle` and assign it to their `theme`:

```qml
SwbStyle { id: brandStyle; primary: "#7c3aed"; radius: 2 }

SwbButton { text: "Save"; theme: brandStyle }
SwbButton { text: "Export"; theme: brandStyle }
```

The color tokens below exist on all three levels: override them on a palette (`SwbTheme.lightPalette.…`), read the effective value from `SwbTheme`, or override them per control through `theme`. Metric tokens live on `SwbTheme` and on each control's `theme`.

Beyond `theme`, most controls also expose their **resolved** style values as writable properties — `bgColor`, `textColor`, `radius`, `controlHeight`, and similar — an escape hatch for one-off tweaks. Overriding one replaces its state logic too: a fixed `bgColor` no longer reacts to hover or press.

### Color tokens

| Token | Purpose |
|---|---|
| `primary` / `primaryForeground` | Solid emphasis fill and the text on it |
| `secondary` / `secondaryForeground` | Subtle fill and the text on it |
| `accent` / `accentForeground` | Hover/selection highlight and the text on it |
| `background` / `foreground` | Window background and default text |
| `popover` | Background for menus, popups, and dialogs |
| `mutedForeground` | Secondary/placeholder text |
| `border` | Hairline borders |
| `destructive` / `destructiveForeground` | Dangerous-action red and the text on it |
| `ring` / `focusRing` | Focused border color and the translucent focus halo |
| `primaryHover`, `secondaryHover`, `accentHover`, `destructiveBg`, `destructiveBgHover` | Pre-derived hover/pressed feedback fills |

### Metric tokens

| Token | Default | Purpose |
|---|---|---|
| `radius` / `radiusSm` | 10 / 8 | Corner radii |
| `fontSize` / `fontSizeSm` | 14 / 13 | Text sizes |
| `fontWeight` | `Font.Medium` | Default text weight |
| `controlHeight` (`Sm` / `Lg`) | 32 (28 / 36) | Control heights per size |
| `iconSize` | 16 | Built-in icon side length |
| `handleSize` / `trackThickness` | 12 / 4 | Slider-like handle and track |
| `handleColor` | white | Handle fill in both modes |
| `calendarCellSize` / `calendarSpacing` | 28 / 2 | Calendar grid geometry |
| `animationDuration` | 150 | Transition duration (ms) |
| `focusRingWidth` | 3 | Focus halo thickness |
| `withAlpha(color, a)` | — | Helper returning `color` at alpha `a` |
| `adaptive(light, dark)` | — | Helper returning `light` or `dark` by the current color mode |

## Components

### Buttons

| Component | Restyles | Description & key API |
|---|---|---|
| `SwbButton` | `Button` | `variant: default \| destructive \| outline \| secondary \| ghost \| link`; `size: sm \| default \| lg` |
| `SwbToolButton` | `ToolButton` | Toolbar button; `variant: default \| outline`; `size: sm \| default \| lg`; checkable; icon-only content collapses to a square |
| `SwbRoundButton` | `RoundButton` | Same variants/sizes as `SwbButton`; circle for short content, pill for longer text |
| `SwbDelayButton` | `DelayButton` | Press-and-hold confirmation; `variant: default \| destructive`; `size`; fills up while held, then emits `activated()` |

**Icons.** These buttons inherit the standard `AbstractButton` icon API: set `icon.source` (or `icon.name`), size it with `icon.width` / `icon.height` (default: the `iconSize` token, 16), position it with `display` (`IconOnly` / `TextOnly` / `TextBesideIcon` / `TextUnderIcon`), and tune the icon–text gap with `spacing`. To place the icon to the right of the text, enable `LayoutMirroring.enabled: true` on the button.

> **Note — icons are tinted by default.** Icons are treated as monochrome glyphs and tinted with the variant's text color (via `icon.color`), so they follow the theme and variant automatically — ideal for single-color line icons. A colorful image will collapse into a single-color silhouette. To keep an icon's original colors, opt out of tinting:
>
> ```qml
> SwbButton {
>     text: "Open"
>     icon.source: "qrc:/icons/picture.svg"
>     icon.color: "transparent"   // keep the icon's own colors
> }
> ```

### Selection & value input

| Component | Restyles | Description & key API |
|---|---|---|
| `SwbCheckBox` | `CheckBox` | Check box with canvas-drawn check mark; `boxSize` sets the box side length |
| `SwbRadioButton` | `RadioButton` | Radio button; `boxSize` sets the circle diameter |
| `SwbSwitch` | `Switch` | Toggle switch; `size: sm \| default`; `trackWidth`/`trackHeight`/`thumbSize` are adjustable |
| `SwbSlider` | `Slider` | Single-value slider, filled track up to the handle |
| `SwbRangeSlider` | `RangeSlider` | Two handles; filled between `first` and `second` |
| `SwbDial` | `Dial` | Rotary knob with canvas-drawn ring track and progress arc |
| `SwbSpinBox` | `SpinBox` | Integer stepper; editable text with side-by-side step buttons; `stepWidth` sizes them |
| `SwbDoubleSpinBox` | `DoubleSpinBox` | Floating-point stepper; `decimals` precision; `stepWidth` sizes the step buttons |
| `SwbTumbler` | `Tumbler` | Wheel picker; highlighted current item, edge fade; `scrollAnimationDuration` tunes the snap speed |
| `SwbComboBox` | `ComboBox` | Editable, searchable dropdown; `placeholderText`; typing filters items, Enter picks the first match; optional `removable` (delete button, emits `removeRequested(index)`) and `reorderable` (press-and-drag to reorder, emits `moveRequested(from, to)`); both report source-model indices and adjust `currentIndex` to keep the selected item |

### Text input

| Component | Restyles | Description & key API |
|---|---|---|
| `SwbTextField` | `TextField` | Single-line input with focus ring and placeholder |
| `SwbSearchField` | `SearchField` | Search box: magnifier icon + clear button |
| `SwbTextArea` | `TextArea` | Multi-line input |
| `SwbTextEditingContextMenu` | — | Themed right-click editing menu (undo/redo, cut/copy/paste/delete, select-all, with canvas-drawn icons). Already built into all six text-editing controls (`SwbTextField`, `SwbTextArea`, `SwbSearchField`, `SwbSpinBox`, `SwbDoubleSpinBox`, `SwbComboBox`); attach to your own editor via `ContextMenu.menu` with its `editor` property |

### Display & feedback

| Component | Restyles | Description & key API |
|---|---|---|
| `SwbLabel` | `Label` | Text label with the theme's size, weight, and disabled state |
| `SwbProgressBar` | `ProgressBar` | Progress bar; supports `indeterminate`; set `animationPaused` to suspend its loop while offscreen |
| `SwbBusyIndicator` | `BusyIndicator` | Canvas-drawn spinning arc; `size: sm \| default \| lg`, custom `diameter` and `color`; set `animationPaused` to suspend rotation while offscreen |
| `SwbPageIndicator` | `PageIndicator` | Page dots, current page emphasized; `interactive` by default |

### Menus

| Component | Restyles | Description & key API |
|---|---|---|
| `SwbMenu` | `Menu` | Rounded popup menu with fade/zoom animation and full keyboard interaction |
| `SwbMenuItem` | `MenuItem` | Menu entry; `variant: default \| destructive`; `shortcutText` for a right-aligned hint; `inset` reserves icon space; submenus and check marks supported |
| `SwbMenuSeparator` | `MenuSeparator` | Thin separator for grouping |
| `SwbMenuBar` | `MenuBar` | In-window menu bar with mouse and keyboard navigation |
| `SwbMenuBarItem` | `MenuBarItem` | Menu bar entry with hover and open states |

### Popups & overlays

| Component | Restyles | Description & key API |
|---|---|---|
| `SwbDialog` | `Dialog` | Modal dialog: title, close button (`closeButtonVisible`), `standardButtons`, dimmed overlay, fade+zoom |
| `SwbDialogButtonBox` | `DialogButtonBox` | Standard button row; styles buttons by role (primary / outline / destructive) |
| `SwbPopup` | `Popup` | General rounded floating panel; fade+zoom, optional modal overlay, close-on-outside-click |
| `SwbDrawer` | `Drawer` | Edge drawer for all four edges; size adapts to edge, optional drag handle (`handleVisible`), overlay, slide animation |
| `SwbToolTip` | `ToolTip` | Inverted-color bubble with pointer arrow, width-capped wrapping, fade+zoom |

### Navigation & paging

| Component | Restyles | Description & key API |
|---|---|---|
| `SwbTabBar` | `TabBar` | Tab bar; `variant: default \| line`; animated shared selection highlight; pairs with views via `currentIndex` |
| `SwbTabButton` | `TabButton` | Tab with selected, hover, and disabled states; picks up `variant` from its `SwbTabBar` automatically |
| `SwbStackView` | `StackView` | Page navigation container: push, pop, replace; `orientation: Qt.Horizontal \| Qt.Vertical` sets the slide direction |
| `SwbSwipeView` | `SwipeView` | Swipeable pages, index-linked with indicators/tab bars |

### Containers & layout

| Component | Restyles | Description & key API |
|---|---|---|
| `SwbFrame` | `Frame` | Card: background, border, radius, padding |
| `SwbGroupBox` | `GroupBox` | Borderless semantic group with a title |
| `SwbPane` | `Pane` | Borderless panel with a muted background |
| `SwbPage` | `Page` | Themed page background with native `header`/`footer` slots |
| `SwbScrollView` | `ScrollView` | Scroll view with themed bars and a keyboard focus ring |
| `SwbSplitView` | `SplitView` | Draggable horizontal/vertical split layout |
| `SwbToolBar` | `ToolBar` | Horizontal or vertical toolbar; `floating: true` swaps the docked square corners for rounded ones |
| `SwbToolSeparator` | `ToolSeparator` | Orientation-aware separator line |

### Scrolling helpers

| Component | Restyles | Description & key API |
|---|---|---|
| `SwbScrollBar` | `ScrollBar` | Slim rounded bar, darkens on hover, both orientations |
| `SwbScrollIndicator` | `ScrollIndicator` | Non-interactive position hint, fades out after scrolling stops |

### Calendar

| Component | Restyles | Description & key API |
|---|---|---|
| `SwbMonthGrid` | `MonthGrid` | Month grid: clicking a day sets `selectedDate`, today highlighted, out-of-month days dimmed |
| `SwbDayOfWeekRow` | `DayOfWeekRow` | Weekday header row, column-aligned with the month grid |
| `SwbWeekNumberColumn` | `WeekNumberColumn` | Week number column, row-aligned with the month grid |

### Table helpers

| Component | Restyles | Description & key API |
|---|---|---|
| `SwbHorizontalHeaderView` | `HorizontalHeaderView` | Column header: left-aligned text, bottom separator, subtle highlight for the selected column |
| `SwbVerticalHeaderView` | `VerticalHeaderView` | Row header: centered text, right separator, subtle highlight for the selected row |
| `SwbSelectionRectangle` | `SelectionRectangle` | Drag-to-select for tables; corner handles match the slider handle style |

## Recipes

### Variants and sizes

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

The `Dialog.Ok`-style enum values come from the base module, so also `import QtQuick.Controls.Basic`:

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

### Menu with destructive entry

```qml
SwbMenu {
    id: contextMenu
    SwbMenuItem { text: "Rename"; shortcutText: "F2" }
    SwbMenuItem { text: "Duplicate" }
    SwbMenuSeparator {}
    SwbMenuItem { text: "Delete"; variant: "destructive" }
}
```

### Composed calendar

`SwbDayOfWeekRow`, `SwbWeekNumberColumn`, and `SwbMonthGrid` share deterministic cell geometry. When a week-number column is present, size the header spacer and weekday row from the actual controls so per-instance theme overrides stay aligned:

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
        SwbMonthGrid { id: grid; month: 5; year: 2026 }  // click a day → selectedDate
    }
}
```

### Table with headers and drag selection

The header views follow the table through `syncView`. Drag selection needs a `selectionModel` on the table (`ItemSelectionModel` comes from `QtQml.Models`, the `SelectionRectangle.Drag` enum from `QtQuick.Controls.Basic`):

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

### Right-click editing menu on a custom editor

The six text-editing controls already show the themed menu. To attach it to any other editable item:

```qml
import QtQuick.Templates as T

TextInput {
    id: myEditor
    T.ContextMenu.menu: SwbTextEditingContextMenu { editor: myEditor }
}
```
