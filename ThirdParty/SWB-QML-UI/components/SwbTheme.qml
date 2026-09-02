pragma Singleton
import QtQuick

QtObject {
    // This single property switches every control between light and dark mode.
    property bool darkMode: Application.styleHints.colorScheme === Qt.Dark

    // Applications can customize either palette without breaking dark-mode switching.
    readonly property SwbPalette lightPalette: SwbPalette {}
    readonly property SwbPalette darkPalette: SwbPalette { darkMode: true }
    readonly property SwbPalette palette: darkMode ? darkPalette : lightPalette

    // Effective color tokens retained for compatibility with existing applications.
    readonly property color primary:             palette.primary
    readonly property color primaryForeground:   palette.primaryForeground
    readonly property color secondary:           palette.secondary
    readonly property color secondaryForeground: palette.secondaryForeground
    readonly property color accent:              palette.accent
    readonly property color accentForeground:    palette.accentForeground
    readonly property color background:          palette.background
    readonly property color popover:             palette.popover
    readonly property color foreground:          palette.foreground
    readonly property color mutedForeground:     palette.mutedForeground
    readonly property color border:              palette.border
    readonly property color destructive:         palette.destructive
    readonly property color destructiveForeground: palette.destructiveForeground
    readonly property color ring:                palette.ring
    readonly property color primaryHover:        palette.primaryHover
    readonly property color secondaryHover:      palette.secondaryHover
    readonly property color accentHover:         palette.accentHover
    readonly property color destructiveBg:       palette.destructiveBg
    readonly property color destructiveBgHover:  palette.destructiveBgHover
    readonly property color focusRing:           palette.focusRing

    property int focusRingWidth: 3

    // Sizing and typography tokens.
    property int radius: 10              // Base corner radius.
    property int radiusSm: radius - 2    // Small radius for sm buttons, list items, and inset highlights.
    property int fontSize: 14            // Base font size.
    property int fontSizeSm: fontSize - 1 // Font size for sm controls.
    property int fontWeight: Font.Medium // Default text weight.
    property int controlHeight: 32       // Default control height.
    property int controlHeightSm: controlHeight - 4  // Height of sm controls.
    property int controlHeightLg: controlHeight + 4  // Height of lg controls.
    property int iconSize: 16            // Embedded icon and checkbox side length.
    property int handleSize: 12          // Diameter of slider-like handles.
    property color handleColor: "#ffffff" // Handle fill in both color modes.
    property int trackThickness: 4       // Thickness of slider-like tracks.
    property int calendarCellSize: 28    // Shared calendar cell size for aligned rows and columns.
    property int calendarSpacing: 2      // Spacing between calendar cells.
    property int animationDuration: 150  // Transition duration.

    function withAlpha(c, a) { return Qt.rgba(c.r, c.g, c.b, a) }

    // Pick a value by the current color scheme, so custom bindings keep
    // switching with dark mode: color: SwbTheme.adaptive("#fff", "#111")
    function adaptive(light, dark) { return darkMode ? dark : light }
}
