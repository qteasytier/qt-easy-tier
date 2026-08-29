/* @file ImportNodesDialog.qml (DDE)
 * @brief DDE 版节点导入对话框：DTK DialogWindow（独立顶层窗），接口与共享版一致
 *
 * 参照 ErrorDialog：标题栏用 DialogTitleBar，取消/关闭发出 closed（兼容共享版 onClosed）。
 */
import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk 1.0 as D

D.DialogWindow {
    id: root

    title: ""
    width: 560

    // 用户点击确定后发出，携带选中的节点数组
    signal nodesSelected(var nodes)
    // 窗口关闭（含标题栏 X）时发出，兼容共享版 QQC.Dialog.onClosed
    signal closed()

    // 真正打开过标记：仅在 open→close 结束才发 closed
    property bool hadShown: false

    function open() {
        // DialogWindow 无 onOpened，故在 open() 中主动刷新收藏与公共节点数据
        ImportNodesViewModel.reload()
        hadShown = true
        visible = true
        requestActivate()
    }

    function close() {
        visible = false
    }

    // DialogWindow 关闭走 hide()，标题栏 X 关闭时也同步 closed
    onVisibleChanged: {
        if (!visible && hadShown) {
            hadShown = false
            closed()
        }
    }

    header: D.DialogTitleBar {
        title: qsTr("从收藏导入节点")
    }

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        spacing: 12

        QQC.ListView {
            id: listView
            Layout.fillWidth: true
            Layout.topMargin: 20
            Layout.preferredHeight: Math.min(360, Math.max(0, ImportNodesViewModel.count * 56 + 36))
            visible: ImportNodesViewModel.count > 0
            clip: true
            model: ImportNodesViewModel

            // 按 section 字段分组显示
            section.property: "section"
            section.criteria: ViewSection.FullString
            section.delegate: Rectangle {
                width: listView.width
                height: 36
                color: palette.alternateBase

                D.Label {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: section
                    font.bold: true
                    font.pixelSize: 13
                    color: palette.highlight
                }
            }

            delegate: Rectangle {
                width: listView.width
                height: 56
                // 斑马条纹背景
                color: index % 2 === 0 ? "transparent"
                       : Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.04)

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 10

                    // 勾选框：直接写回 model.checked
                    D.CheckBox {
                        id: itemCheck
                        checked: model.checked
                        onCheckedChanged: ImportNodesViewModel.setChecked(index, checked)
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        D.Label {
                            text: model.name + (model.publicKey ? qsTr("【安全】") : "")
                            font.pixelSize: 14
                            font.bold: model.publicKey !== ""
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        D.Label {
                            text: model.uri
                            font.pixelSize: 11
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
        D.Label {
            Layout.fillWidth: true
            visible: ImportNodesViewModel.count === 0
            text: qsTr("暂无收藏节点和公共节点")
            color: palette.placeholderText
            font.pixelSize: 13
            horizontalAlignment: Text.AlignHCenter
            Layout.preferredHeight: 140
        }

        // 底部按钮：取消 / 确定（右下角）
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 16
            spacing: 10

            Item { Layout.fillWidth: true }

            D.Button {
                text: qsTr("取消")
                onClicked: root.close()
            }
            D.Button {
                text: qsTr("确定")
                highlighted: true
                onClicked: {
                    root.nodesSelected(ImportNodesViewModel.selectedNodes())
                    root.close()
                }
            }
        }

        // 底部留白占位
        Item {
            Layout.preferredHeight: 8
        }
    }
}
