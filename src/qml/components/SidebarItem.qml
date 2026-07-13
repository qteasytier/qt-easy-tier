/* @brief 侧边栏单个导航项：图标 + 标签 + 选中指示条，支持点击切换 */
import QtQuick
import QtQuick.Controls
import QtQuick.Effects

// 侧边栏单个导航项：图标 + 标签 + 选中指示条
// 图标使用 SVG 并通过 MultiEffect 着色为主题高亮色
/* @brief 导航项根容器，提供背景色、指示条和点击交互 */
Rectangle {
    id: root
    radius: 6
    // 选中态与未选中态的背景色
    color: selected ? palette.button : "transparent"

    /* SVG 图标的资源路径，如 "qrc:/icons/net-page.svg" */
    property string icon: ""
    /* 导航项底部标签文字 */
    property string label: ""
    /* 是否为当前选中项，控制指示条显隐和背景色 */
    property bool selected: false

    /* 点击导航项时发出 */
    signal clicked

    implicitHeight: 56

    // 背景层：与 root 重叠，提供选中态背景色（避免 root 颜色变更时产生不必要的事件影响）
    Rectangle {
        id: highlight
        anchors.fill: parent
        anchors.margins: 3
        radius: 5
        color: root.selected ? palette.button : "transparent"
    }

    // 左侧选中指示条
    Rectangle {
        id: indicator

        width: 3
        height: parent.height * 0.6
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 3
        color: palette.highlight
        radius: 1.5
        visible: root.selected
    }

    // 图标 + 标签纵向排列
    Column {
        anchors.left: indicator.right
        anchors.right: parent.right
        anchors.rightMargin: 3
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        // 图标容器
        Item {
            width: parent.width
            height: 22

            Image {
                id: iconImage
                width: 22
                height: 22
                anchors.centerIn: parent
                source: root.icon
                sourceSize: Qt.size(22, 22)
                fillMode: Image.PreserveAspectFit
                // 通过图层效果将 SVG 图标着色为主题高亮色
                layer.enabled: true
                layer.effect: MultiEffect {
                    colorizationColor: palette.highlight
                    colorization: 1.0
                }
            }
        }

        Label {
            text: root.label
            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 10
            color: root.selected ? palette.buttonText : palette.windowText
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
