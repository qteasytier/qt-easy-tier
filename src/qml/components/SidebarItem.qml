/* @brief 侧边栏单个导航项：图标 + 标签 + 悬停/选中背景，支持点击切换 */
import QtQuick
import QtQuick.Controls
import QtQuick.Effects

// 侧边栏单个导航项：图标 + 标签 + 悬停/选中背景
// 图标使用 SVG 并通过 MultiEffect 着色为主题高亮色
Rectangle {
    id: root
    radius: 6
    implicitHeight: 56

    /* SVG 图标的资源路径，如 "qrc:/icons/net-page.svg" */
    property string icon: ""
    /* 导航项底部标签文字 */
    property string label: ""
    /* 是否为当前选中项，控制背景色 */
    property bool selected: false
    /* 是否处于悬停状态，由 MouseArea.containsMouse 驱动 */
    property bool hovered: false
    /* 未选中且未悬停时的背景色 */
    readonly property color normalBackground: "transparent"
    /* 悬停态的柔和背景色 */
    readonly property color hoverBackground: Qt.rgba(palette.highlight.r,
                                                      palette.highlight.g,
                                                      palette.highlight.b,
                                                      0.08)

    /* 点击导航项时发出 */
    signal clicked

    // 背景层：内缩 3px 的圆角矩形，与旧版选中背景几何一致
    Rectangle {
        anchors.fill: parent
        anchors.margins: 3
        radius: 5
        color: root.selected ? palette.button
                             : root.hovered ? root.hoverBackground
                                            : root.normalBackground

        Behavior on color {
            ColorAnimation {
                duration: 140
                easing.type: Easing.OutQuad
            }
        }
    }

    // 图标 + 标签纵向排列
    Column {
        anchors.left: parent.left
        anchors.leftMargin: 6
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
        hoverEnabled: true
        onContainsMouseChanged: root.hovered = containsMouse
        onClicked: root.clicked()
    }
}
