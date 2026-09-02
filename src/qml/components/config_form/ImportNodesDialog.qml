/* @brief 从收藏导入节点对话框：展示收藏节点和公共节点列表，支持多选导入到当前配置的服务器列表，Swb 控件迁移版 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtEasyTier
import SwbControls

// 从收藏导入节点对话框
// 展示收藏节点和公共节点列表，支持多选导入
/* @brief 导入节点对话框，通过 ImportNodesViewModel 获取可选节点，按分区展示 */
SwbDialog {
    id: root
    title: qsTr("从收藏导入节点")
    standardButtons: Dialog.Ok | Dialog.Cancel
    parent: Overlay.overlay
    anchors.centerIn: parent

    width: Math.min(480, parent ? parent.width - 48 : 360)
    height: Math.min(400, parent ? parent.height - 80 : 300)

    /* 用户点击确定后发出，携带选中的节点数组 */
    signal nodesSelected(var nodes)

    // 每次打开时重新加载收藏和公共节点数据
    onOpened: ImportNodesViewModel.reload()

    // 确认时收集所有勾选项
    onAccepted: root.nodesSelected(ImportNodesViewModel.selectedNodes())

    contentItem: Item {
        implicitWidth: 432
        implicitHeight: 300

        ListView {
            id: listView
            anchors.fill: parent
            visible: ImportNodesViewModel.count > 0
            clip: true
            model: ImportNodesViewModel

            // 按 section 字段分组显示
            section.property: "section"
            section.criteria: ViewSection.FullString
            section.delegate: Rectangle {
                width: listView.width
                height: 32
                color: SwbTheme.secondary

                SwbLabel {
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: section
                    font.bold: true
                    font.pixelSize: 13
                    color: SwbTheme.foreground
                }
            }

            delegate: Rectangle {
                id: importRow

                width: listView.width
                height: 48
                color: rowHover.hovered ? SwbTheme.withAlpha(SwbTheme.foreground, 0.04)
                     : "transparent"

                Behavior on color {
                    ColorAnimation { duration: SwbTheme.animationDuration }
                }

                HoverHandler {
                    id: rowHover
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    spacing: 6

                    // 勾选框：直接写回 model.checked
                    SwbCheckBox {
                        id: itemCheck
                        checked: model.checked
                        onCheckedChanged: ImportNodesViewModel.setChecked(index, checked)
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        SwbLabel {
                            text: model.name + (model.publicKey ? qsTr("【安全】") : "")
                            font.pixelSize: 13
                            font.bold: model.publicKey !== ""
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        SwbLabel {
                            text: model.uri
                            font.pixelSize: 10
                            color: SwbTheme.mutedForeground
                            font.family: "monospace"
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            ScrollBar.vertical: SwbScrollBar {}
        }

        // 空列表占位提示
        Item {
            anchors.fill: parent
            visible: ImportNodesViewModel.count === 0

            SwbLabel {
                anchors.centerIn: parent
                text: qsTr("暂无收藏节点和公共节点")
                color: SwbTheme.mutedForeground
                font.pixelSize: 13
                visible: ImportNodesViewModel.count === 0
            }
        }
    }
}
