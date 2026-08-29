/* @file EditableListItem.qml (DDE)
 * @brief DDE 版可编辑列表条目：DTK ItemDelegate 原生高亮，文本 + 删除按钮
 */
import QtQuick
import QtQuick.Layouts
import org.deepin.dtk 1.0 as D

D.ItemDelegate {
    id: root

    required property int itemIndex
    required property string itemText

    signal removeRequested(int index)

    implicitHeight: 38
    width: parent ? parent.width : 200

    // 覆写内容：文本 + 删除按钮；保留 ItemDelegate 原生 hover/选中背景
    contentItem: Item {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 4
            spacing: 4

            D.Label {
                text: root.itemText
                Layout.fillWidth: true
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            IconToolButton {
                iconSource: "qrc:/icons/delete.svg"
                flat: true
                onClicked: root.removeRequested(root.itemIndex)
            }
        }
    }
}
