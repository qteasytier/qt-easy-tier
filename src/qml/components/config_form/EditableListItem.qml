/* @brief 可编辑列表的单个条目：显示文本 + 编辑/删除按钮，悬停浅色反馈与行底分隔线 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import SwbControls

// 可编辑列表的单个条目：显示文本 + 编辑/删除按钮
/* @brief 列表项根容器，提供悬停反馈、行底分隔线和编辑/删除交互 */
Rectangle {
    id: root

    /* 在列表中的索引，用于编辑/删除回调时定位 */
    required property int itemIndex
    /* 显示的文本内容 */
    required property string itemText

    /* 请求编辑当前项，携带自身索引 */
    signal editRequested(int index)
    /* 请求从列表中删除当前项，携带自身索引 */
    signal removeRequested(int index)

    width: parent ? parent.width : 200
    height: 38
    color: rowHover.hovered ? SwbTheme.withAlpha(SwbTheme.foreground, 0.04)
         : "transparent"
    radius: SwbTheme.radiusSm

    Behavior on color {
        ColorAnimation { duration: SwbTheme.animationDuration }
    }

    HoverHandler {
        id: rowHover
    }

    // 行底分隔线（最后一行不显示）
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: SwbTheme.border
        visible: root.itemIndex < (parent.ListView.view ? parent.ListView.view.count - 1 : 0)
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 4
        spacing: 4

        SwbLabel {
            text: root.itemText
            Layout.fillWidth: true
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        IconToolButton {
            iconSource: "qrc:/icons/edit.svg"
            flat: true
            onClicked: root.editRequested(root.itemIndex)
        }

        IconToolButton {
            iconSource: "qrc:/icons/delete.svg"
            flat: true
            onClicked: root.removeRequested(root.itemIndex)
        }
    }
}
