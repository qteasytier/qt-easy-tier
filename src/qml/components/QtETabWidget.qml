/* @brief 自定义 Tab 控件：水平标签栏 + 滑动高亮指示条 + StackLayout 内容区，子组件通过 tabTitle 属性自动注册 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 自定义 Tab 控件：水平标签栏 + 滑动高亮指示条 + StackLayout 内容区
// 子组件通过 tabTitle 属性自动注册为标签页
/* @brief Tab 控件根布局，包含标签栏、分隔线和内容区 */
ColumnLayout {
    id: root

    /* 当前激活的标签页索引 */
    property int currentIndex: 0
    /* 默认内容容器：子组件直接放入 StackLayout，通过 tabTitle 属性注册为标签页 */
    default property alias contentData: stack.data
    spacing: 0

    // 标签栏区域
    Item {
        Layout.fillWidth: true
        implicitHeight: tabRow.implicitHeight

        RowLayout {
            id: tabRow
            anchors.fill: parent
            spacing: 0

            Repeater {
                model: tabsModel

                delegate: Rectangle {
                    id: tabBtn
                    property bool isSelected: root.currentIndex === index

                    Layout.fillWidth: true
                    implicitWidth: Math.max(72, tabLabel.implicitWidth + 32)
                    implicitHeight: 36
                    // 悬停时的背景反馈（非选中态）
                    color: tabMouse.containsMouse && !isSelected
                        ? Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.06)
                        : "transparent"

                    Label {
                        id: tabLabel
                        anchors.centerIn: parent
                        text: model.title
                        color: tabBtn.isSelected ? palette.highlight : palette.windowText
                        font.bold: tabBtn.isSelected

                        // 选中态颜色渐变过渡
                        Behavior on color {
                            ColorAnimation { duration: 180; easing.type: Easing.InOutQuad }
                        }
                    }

                    MouseArea {
                        id: tabMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.currentIndex = index
                    }
                }
            }
        }

        // 高亮指示条：随 currentIndex 滑动
        Rectangle {
            id: highlightBar
            height: 2
            y: parent.height - height
            color: palette.highlight

            // 每个标签宽度 = 总宽 / 标签数
            readonly property real tabWidth: tabsModel.count > 0 ? parent.width / tabsModel.count : 0
            x: root.currentIndex * tabWidth
            width: tabWidth

            // 滑动动画
            Behavior on x {
                NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
            }
            Behavior on width {
                NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
            }
        }
    }

    // 标签栏底部分隔线
    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.12)
    }

    // 内容区：通过 StackLayout 按索引切换
    StackLayout {
        id: stack
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: root.currentIndex
    }

    // 标签标题模型
    ListModel {
        id: tabsModel
    }

    // 组件完成时，自动收集子组件的 tabTitle 属性注册为标签
    Component.onCompleted: {
        for (var i = 0; i < stack.children.length; i++) {
            var child = stack.children[i]
            if (child && child.tabTitle !== undefined)
                tabsModel.append({ title: child.tabTitle })
        }
    }
}
