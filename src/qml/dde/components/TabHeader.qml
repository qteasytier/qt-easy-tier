/* @file TabHeader.qml (DDE)
 * @brief DDE 下划线风格页签头：页签均分宽度，选中项高亮、下划线滑动指示
 *
 * tabs 为标题文本数组（qsTr 字面量，retranslate 即时切换）。与 StackLayout
 * 的 currentIndex 绑定配合使用；不用 Chameleon 风格注入与 QQC TabBar
 * （本环境 QQC C++ 插件类型不可用）。
 */
import QtQuick
import QtQuick.Layouts
import org.deepin.dtk

/* @brief 页签头根容器：均分宽度的页签项 + 滑动下划线 + 底部分隔线 */
Item {
    id: root

    /* 页签标题文本数组 */
    property var tabs: []
    /* 当前选中页签索引 */
    property int currentIndex: 0

    /* 当前选中页签项（依赖 tabRepeater.count：delegate 全部创建完成后自动求值，
     * 保证初始状态下指示条就能定位到选中页签；itemAt 本身无通知，不能直接用于绑定） */
    readonly property Item currentItem: tabRepeater.count > 0 ? tabRepeater.itemAt(currentIndex) : null

    implicitHeight: 38

    RowLayout {
        id: rowLayout
        anchors.fill: parent
        spacing: 0

        Repeater {
            id: tabRepeater

            model: root.tabs

            delegate: Item {
                id: tabItem

                /* 模型角色：页签标题文本 */
                required property string modelData
                required property int index

                /* 页签项均分整行宽度 */
                Layout.fillWidth: true
                Layout.fillHeight: true

                Label {
                    id: tabLabel
                    anchors.centerIn: parent
                    text: tabItem.modelData
                    font.bold: tabItem.index === root.currentIndex
                    color: tabItem.index === root.currentIndex ? palette.highlight : palette.windowText

                    // 选中/取消选中时文字颜色平滑过渡
                    Behavior on color {
                        ColorAnimation { duration: 150 }
                    }
                }

                TapHandler {
                    onTapped: root.currentIndex = tabItem.index
                }
            }
        }
    }

    // 滑动下划线指示条：位置/宽度跟随当前页签，切换时平滑移动
    // （放在 root 而非 RowLayout 内，避免被布局管理覆盖 x/width；
    //   RowLayout 与 root 同几何，页签项 x/width 可直接使用）
    Rectangle {
        id: indicator

        anchors.bottom: parent.bottom
        height: 2
        color: palette.highlight

        x: root.currentItem ? root.currentItem.x : 0
        width: root.currentItem ? root.currentItem.width : 0

        Behavior on x {
            NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
        }
        Behavior on width {
            NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
        }
    }

    // 底部分隔线（贯穿整宽，作为页签区与内容区的界线）
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.15)
    }
}
