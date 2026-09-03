/* @file EditableListItem.qml (DDE)
 * @brief DDE 版可编辑列表条目：DTK ItemDelegate 原生高亮，文本 + 编辑/删除按钮
 *
 * 与共享版接口一致：itemIndex/itemText 属性 + editRequested/removeRequested 信号
 * （编辑/删除回调时以索引定位）。
 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

D.ItemDelegate {
    id: root

    required property int itemIndex
    required property string itemText

    /* 请求编辑当前项，携带自身索引 */
    signal editRequested(int index)
    /* 请求从列表中删除当前项，携带自身索引 */
    signal removeRequested(int index)

    implicitHeight: 38
    width: parent ? parent.width : 200

    // 覆写内容：文本 + 编辑/删除按钮；保留 ItemDelegate 原生 hover/选中背景
    contentItem: Item {
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

            // IconToolButton 位于 dde/components/（跨子目录），经 QtEasyTier 显式导入解析
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
}
