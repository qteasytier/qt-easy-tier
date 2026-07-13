/* @brief 从收藏导入节点对话框：展示收藏节点和公共节点列表，支持多选导入到当前配置的服务器列表 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtEasyTier

// 从收藏导入节点对话框
// 展示收藏节点和公共节点列表，支持多选导入
/* @brief 导入节点对话框，通过 ImportNodesViewModel 获取可选节点，按分区展示 */
Dialog {
    id: root
    title: qsTr("从收藏导入节点")
    standardButtons: Dialog.Ok | Dialog.Cancel
    modal: true
    anchors.centerIn: parent

    width: Math.min(480, parent ? parent.width - 48 : 360)
    height: Math.min(400, parent ? parent.height - 80 : 300)

    /* 用户点击确定后发出，携带选中的节点数组 */
    signal nodesSelected(var nodes)

    // 每次打开时重新加载收藏和公共节点数据
    onOpened: ImportNodesViewModel.reload()

    // 确认时收集所有勾选项
    onAccepted: root.nodesSelected(ImportNodesViewModel.selectedNodes())

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: ImportNodesViewModel.count > 0
            visible: ImportNodesViewModel.count > 0
            clip: true
            model: ImportNodesViewModel

            // 按 section 字段分组显示
            section.property: "section"
            section.criteria: ViewSection.FullString
            section.delegate: Rectangle {
                width: listView.width
                height: 32
                color: palette.alternateBase

                Label {
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: section
                    font.bold: true
                    font.pixelSize: 13
                    color: palette.highlight
                }
            }

            delegate: Rectangle {
                width: listView.width
                height: 48
                // 斑马条纹背景
                color: index % 2 === 0 ? "transparent"
                       : Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.04)

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    spacing: 6

                    // 勾选框：直接写回 model.checked
                    CheckBox {
                        id: itemCheck
                        checked: model.checked
                        onCheckedChanged: ImportNodesViewModel.setChecked(index, checked)
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        Label {
                            text: model.name + (model.publicKey ? qsTr("【安全】") : "")
                            font.pixelSize: 13
                            font.bold: model.publicKey !== ""
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Label {
                            text: model.uri
                            font.pixelSize: 10
                            color: palette.placeholderText
                            font.family: "monospace"
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }

        // 空列表占位提示
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: ImportNodesViewModel.count === 0
            visible: ImportNodesViewModel.count === 0

            Label {
                anchors.centerIn: parent
                text: qsTr("暂无收藏节点和公共节点")
                color: palette.placeholderText
                font.pixelSize: 13
                visible: ImportNodesViewModel.count === 0
            }
        }
    }
}
