import QtQuick

QtObject {
    property bool darkMode: false

    property color primary:             darkMode ? "#e5e5e5" : "#000000"
    property color primaryForeground:   darkMode ? "#171717" : "#fafafa"
    property color secondary:           darkMode ? "#262626" : "#f5f5f5"
    property color secondaryForeground: darkMode ? "#fafafa" : "#171717"
    property color accent:              darkMode ? "#404040" : "#f5f5f5"
    property color accentForeground:    darkMode ? "#fafafa" : "#171717"
    property color background:          darkMode ? "#0a0a0a" : "#ffffff"
    property color popover:             darkMode ? "#171717" : "#ffffff"
    property color foreground:          darkMode ? "#fafafa" : "#000000"
    property color mutedForeground:     darkMode ? "#a1a1a1" : "#737373"
    property color border:              darkMode ? "#26ffffff" : "#e5e5e5"
    property color destructive:         darkMode ? "#ff6467" : "#e7000b"
    property color destructiveForeground: "#ffffff"
    property color ring:                darkMode ? "#737373" : "#a1a1a1"

    // Derived feedback colors. Keep the formulas in sync with SwbStyle,
    // which uses them to detect whether a palette customized these tokens.
    property color primaryHover:       withAlpha(primary, 0.8)
    property color secondaryHover:     Qt.darker(secondary, 1.05)
    property color accentHover:        Qt.darker(accent, 1.08)
    property color destructiveBg:      withAlpha(destructive, 0.1)
    property color destructiveBgHover: withAlpha(destructive, 0.2)
    property color focusRing:          withAlpha(ring, 0.5)

    function withAlpha(c, a) {
        return Qt.rgba(c.r, c.g, c.b, a)
    }
}
