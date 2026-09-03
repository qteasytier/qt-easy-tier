/* @file IconToolButton.qml (DDE)
 * @brief DDE 版统一图标工具按钮：IconImage 实时着色，图标色随 DTK 主题亮暗自动切换
 *
 * 与共享版（SwbControls）同款技术：QtQuick.Controls.impl 的 IconImage 对单色
 * SVG 按 color 属性实时着色（光栅层面，无 shader 后处理）。默认取 palette
 * 前景色（DTK 主题驱动，亮暗即时切换），选中态用高亮对比色保证可读性。
 */
import QtQuick
import QtQuick.Controls.impl
import org.deepin.dtk 1.0 as D

D.ToolButton {
    id: root

    property url iconSource: ""
    // 显式着色优先（兼容旧 API）；透明时按选中态/主题前景色自动着色
    property color iconTint: "transparent"
    property int iconSize: 18
    property int buttonSize: 32

    /* 图标着色：显式 iconTint > 选中态高亮对比色 > 主题前景色（随亮暗切换） */
    readonly property color effectiveTint:
        iconTint.a > 0 ? iconTint
        : checked ? palette.highlightedText
        : palette.windowText

    padding: 0
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0
    implicitWidth: buttonSize
    implicitHeight: buttonSize

    // 纯图标按钮，避免 ToolButton 默认 TextUnderIcon 叠加空文本
    display: D.IconLabel.IconOnly

    // IconImage 实时着色渲染（替代 D.ToolButton 自带的 dtk 图标管线）：
    // 外层定尺寸 Item 防止 Control 布局把图标拉伸到整个按钮内容区
    contentItem: Item {
        implicitWidth: root.iconSize
        implicitHeight: root.iconSize

        IconImage {
            anchors.centerIn: parent
            width: root.iconSize
            height: root.iconSize
            source: root.iconSource
            sourceSize: Qt.size(root.iconSize * 2, root.iconSize * 2)
            color: root.effectiveTint
            opacity: root.enabled ? 1.0 : 0.4
        }
    }
}
