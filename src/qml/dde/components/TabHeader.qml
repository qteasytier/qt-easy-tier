/* @file TabHeader.qml (DDE)
 * @brief DDE 下划线风格页签头：DTK 配色自绘，替代共享版 SwbTabBar(line)
 *
 * tabs 为标题文本数组（元素用 qsTr 字面量，engine.retranslate() 会让数组
 * 绑定重求值实现语言即时切换）；点击切换 currentIndex，选中项文字高亮加粗
 * 并带底部下划线指示条，整行自带底部分隔线。与 StackLayout 的
 * currentIndex 绑定配合使用（不用 Chameleon 风格注入，不引入 QQC TabBar）。
 */
import QtQuick
import QtQuick.Layouts
import org.deepin.dtk

/* @brief 页签头根容器：横向排列的页签项 + 底部分隔线 */
Item {
    id: root

    /* 页签标题文本数组 */
    property var tabs: []
    /* 当前选中页签索引 */
    property int currentIndex: 0

    implicitHeight: 38

    RowLayout {
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.right: parent.right
        spacing: 32

        Repeater {
            model: root.tabs

            delegate: Item {
                id: tabItem

                /* 模型角色：页签标题文本 */
                required property string modelData
                required property int index

                implicitWidth: tabLabel.implicitWidth
                Layout.fillHeight: true

                Label {
                    id: tabLabel
                    anchors.centerIn: parent
                    text: tabItem.modelData
                    font.bold: tabItem.index === root.currentIndex
                    color: tabItem.index === root.currentIndex ? palette.highlight : palette.windowText
                }

                // 选中页签底部下划线指示条
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 2
                    visible: tabItem.index === root.currentIndex
                    color: palette.highlight
                }

                TapHandler {
                    onTapped: root.currentIndex = tabItem.index
                }
            }
        }

        // 弹簧占位，页签保持左对齐自然宽度
        Item { Layout.fillWidth: true }
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
