/* @brief 可编辑列表的单个条目：显示文本 + 删除按钮，带斑马条纹背景 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 可编辑列表的单个条目：显示文本 + 删除按钮，带斑马条纹背景
/* @brief 列表项根容器，提供奇偶行交替背景和删除交互 */
Rectangle {
    id: root

    /* 在列表中的索引，用于删除回调时定位 */
    required property int itemIndex
    /* 显示的文本内容 */
    required property string itemText

    /* 请求从列表中删除当前项，携带自身索引 */
    signal removeRequested(int index)

    width: parent ? parent.width : 200
    height: 38
    // 奇偶行交替背景色（斑马条纹）
    color: itemIndex % 2 === 0 ? "transparent" : Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.04)
    radius: 4

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 4
        spacing: 4

        Label {
            text: root.itemText
            Layout.fillWidth: true
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        ToolButton {
            icon.source: "qrc:/icons/delete.svg"
            icon.color: "transparent"
            icon.width: 14
            icon.height: 14
            padding: 0
            flat: true
            implicitWidth: 28
            implicitHeight: 28
            onClicked: root.removeRequested(root.itemIndex)
        }
    }
}
