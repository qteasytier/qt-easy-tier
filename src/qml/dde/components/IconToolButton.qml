/* @file IconToolButton.qml (DDE)
 * @brief DDE 版统一图标工具按钮：IconImage 实时着色，图标色随 DTK 主题亮暗切换
 *
 * 与共享版（SwbControls）同款技术：QtQuick.Controls.impl 的 IconImage 对
 * 单色 SVG 按 color 实时着色。默认取主题前景色，选中态用高亮对比色。
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

    /* 图标着色：显式 iconTint > 选中态白色（dtk 高亮底色上）> 主题前景色。
     * 注：不读 palette —— dtk ToolButton 的 palette 会从 contentItem 反向同步，
     * contentItem 内 IconImage.color 再读 palette 即成绑定环；故以 DTK 全局
     * 主题类型推导前景色（dtk 前景即随亮暗主题黑白切换，语义等效）。 */
    readonly property color effectiveTint: {
        if (iconTint.a > 0)
            return iconTint
        if (checked)
            return "#ffffff"
        return D.DTK.themeType === D.ApplicationHelper.DarkType ? "#ffffff" : "#000000"
    }

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
