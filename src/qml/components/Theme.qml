/* @brief 应用颜色主题常量：集中管理所有硬编码颜色值，状态指示灯、按钮等特定场景通过具名常量引用 */
import QtQuick

// 应用颜色主题常量
// 集中管理所有硬编码颜色值，指示灯/按钮等特定场景通过具名常量引用
/* @brief 颜色常量容器，暴露状态色和通用色给其他 QML 组件 */
QtObject {
    // 状态指示灯色
    readonly property color statusGreen:  "#4caf50"
    readonly property color statusOrange: "#ff9800"
    readonly property color statusRed:    "#f44336"
    readonly property color statusBlue:   "#2196f3"

    // 通用颜色
    readonly property color textOnColor: "#ffffff"
}
