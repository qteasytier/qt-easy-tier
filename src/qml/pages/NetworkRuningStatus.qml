/* @brief 网络运行状态页面：通过自定义 Tab 展示当前网络的节点信息列表和运行日志，支持日志导出 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtEasyTier

// 网络运行状态页面：展示当前网络的节点信息与运行日志
/* @brief 运行状态根容器，包含节点信息 Tab 和运行日志 Tab */
Item {
    Layout.fillWidth: true
    Layout.fillHeight: true

    Theme { id: theme }

    /* 根据连接类型等级字符串返回对应颜色 */
    function statusColor(level) {
        if (level === "green") return theme.statusGreen
        if (level === "orange") return theme.statusOrange
        if (level === "red") return theme.statusRed
        if (level === "blue") return theme.statusBlue
        return Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.15)
    }

    // 自定义 Tab 控件，包含两个标签页
    QtETabWidget {
        anchors.fill: parent

        // ========== 标签页1：运行状态 ==========
        Item {
            property string tabTitle: qsTr("运行状态")

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 0

                // 空状态：暂无节点信息
                Label {
                    visible: VpnManager.nodeInfoModel.count === 0
                    text: qsTr("暂无节点信息")
                    font.pixelSize: 24
                    color: palette.placeholderText
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                // 节点信息列表
                ListView {
                    id: nodeListView
                    visible: VpnManager.nodeInfoModel.count > 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: VpnManager.nodeInfoModel
                    spacing: 6
                    clip: true

                    // 每个节点用 Card 组件展示
                    delegate: Card {
                        width: nodeListView.width
                        contentSpacing: 4

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            // 第一行：虚拟IP + 主机名 + 本机标记 + 连接类型标记
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                // 虚拟 IP（高亮显示）
                                Label {
                                    text: virtualIp || ""
                                    font.bold: true
                                    color: palette.highlight
                                }

                                // 主机名
                                Label {
                                    text: hostname || ""
                                    color: palette.windowText
                                }

                                Item { Layout.fillWidth: true }

                                // 本机标记：半透明高亮背景
                                Rectangle {
                                    visible: isLocalNode === true
                                    radius: 4
                                    color: Qt.rgba(palette.highlight.r,
                                        palette.highlight.g,
                                        palette.highlight.b, 0.15)
                                    implicitWidth: localLabel.implicitWidth + 8
                                    implicitHeight: localLabel.implicitHeight + 4

                                    Label {
                                        id: localLabel
                                        anchors.centerIn: parent
                                        text: qsTr("本机")
                                        font: FontHelper.smallFont
                                        color: palette.highlight
                                    }
                                }

                                // 连接类型标记：颜色通过 Theme 单例统一管理
                                Rectangle {
                                    visible: !(isLocalNode === true)
                                    radius: 4
                                    color: statusColor(connectionTypeLevel)
                                    implicitWidth: typeLabel.implicitWidth + 8
                                    implicitHeight: typeLabel.implicitHeight + 4

                                    Label {
                                        id: typeLabel
                                        anchors.centerIn: parent
                                        text: connectionTypeText || ""
                                        font: FontHelper.smallFont
                                        color: connectionTypeTextColor === "onColor" ? theme.textOnColor : palette.windowText
                                    }
                                }
                            }

                            // 第二行：延迟 + 协议信息
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12

                                // 延迟显示：绿(<60ms) / 橙(<200ms) / 红(>=200ms)
                                Label {
                                    text: latencyText
                                    font: FontHelper.smallFont
                                    color: latencyLevel === "unknown" ? palette.placeholderText : statusColor(latencyLevel)
                                    // 非本机节点或本机有延迟时显示
                                    visible: showLatency
                                }

                                // 协议信息（如 UDP/TCP）
                                Label {
                                    text: protocol || ""
                                    font: FontHelper.smallFont
                                    color: palette.placeholderText
                                    visible: (protocol || "") !== ""
                                }
                            }
                        }
                    }
                }
            }
        }

        // ========== 标签页2：运行日志 ==========
        Item {
            property string tabTitle: qsTr("运行日志")

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 0

                Item { Layout.preferredHeight: 8 }

                // 空状态：暂无日志
                Label {
                    visible: VpnManager.runtimeLogModel.count === 0
                    text: qsTr("暂无日志")
                    font.pixelSize: 24
                    color: palette.placeholderText
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                // 日志文本框：使用控件内置滚动条，文本自动避开滚动区域边界
                ScrollView {
                    id: logScrollView
                    visible: VpnManager.runtimeLogModel.count > 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    TextArea {
                        id: runtimeLogTextArea
                        text: VpnManager.runtimeLogModel.plainText
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextEdit.WrapAnywhere
                        color: palette.windowText
                        background: null

                        onTextChanged: cursorPosition = length
                    }
                }

                // 底部工具栏：导出日志按钮
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 4

                    Item { Layout.fillWidth: true }

                    Button {
                        text: qsTr("导出日志")
                        onClicked: exportLogDialog.open()
                    }
                }
            }

            // 导出日志文件对话框
            FileDialog {
                id: exportLogDialog
                title: qsTr("导出日志")
                nameFilters: [qsTr("日志文件 (*.log)"), qsTr("所有文件 (*)")]
                fileMode: FileDialog.SaveFile
                currentFile: AppState.homeDirectory + "/qteasytier.log"
                onAccepted: {
                    VpnManager.exportLog(selectedFile.toString())
                }
            }
        }
    }
}
