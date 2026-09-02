/* @brief 网络运行状态页面：通过自定义 Tab 展示当前网络的节点信息列表和运行日志，支持日志导出，Swb 控件迁移版 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtEasyTier
import SwbControls

// 网络运行状态页面：展示当前网络的节点信息与运行日志
/* @brief 运行状态根容器，包含节点信息 Tab、运行日志 Tab 与页面级导出日志按钮 */
ColumnLayout {
    id: root

    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: 0

    Theme { id: appTheme }

    /* 根据连接类型等级字符串返回对应颜色 */
    function statusColor(level) {
        if (level === "green") return appTheme.statusGreen
        if (level === "orange") return appTheme.statusOrange
        if (level === "red") return appTheme.statusRed
        if (level === "blue") return appTheme.statusBlue
        return SwbTheme.withAlpha(SwbTheme.foreground, 0.15)
    }

    // 标签栏：直接使用 SwbControls 的 SwbTabBar（line 变体，按钮均分宽度）
    SwbTabBar {
        id: tabBar
        Layout.fillWidth: true
        variant: "line"

        SwbTabButton {
            text: qsTr("运行状态")
            // 绑定页面根宽而非 tabBar.width：后者经 implicitWidth 依赖子项宽度，会形成绑定循环
            width: root.width / 2
        }
        SwbTabButton {
            text: qsTr("运行日志")
            width: root.width / 2
        }
    }

    // 标签栏底部分隔线
    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: SwbTheme.border
    }

    // 内容区：两个标签页按索引切换
    StackLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: tabBar.currentIndex

        // ========== 标签页1：运行状态 ==========
        Item {

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 0

                // 空状态：暂无节点信息
                SwbLabel {
                    visible: VpnRuntimeService.nodeInfoModel.count === 0
                    text: qsTr("暂无节点信息")
                    font.pixelSize: 24
                    color: SwbTheme.mutedForeground
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                // 节点信息列表
                ListView {
                    id: nodeListView
                    visible: VpnRuntimeService.nodeInfoModel.count > 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: VpnRuntimeService.nodeInfoModel
                    spacing: 6
                    clip: true

                    ScrollBar.vertical: SwbScrollBar {}

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

                                // 虚拟 IP（加粗前景色强调）
                                SwbLabel {
                                    text: virtualIp || ""
                                    font.bold: true
                                }

                                // 主机名
                                SwbLabel {
                                    text: hostname || ""
                                }

                                Item { Layout.fillWidth: true }

                                // 本机标记：柔和前景色徽章
                                Rectangle {
                                    visible: isLocalNode === true
                                    radius: 4
                                    color: SwbTheme.withAlpha(SwbTheme.foreground, 0.12)
                                    implicitWidth: localLabel.implicitWidth + 8
                                    implicitHeight: localLabel.implicitHeight + 4

                                    SwbLabel {
                                        id: localLabel
                                        anchors.centerIn: parent
                                        text: qsTr("本机")
                                        font: FontHelper.smallFont
                                    }
                                }

                                // 连接类型标记：颜色通过状态色统一管理
                                Rectangle {
                                    visible: !(isLocalNode === true)
                                    radius: 4
                                    color: statusColor(connectionTypeLevel)
                                    implicitWidth: typeLabel.implicitWidth + 8
                                    implicitHeight: typeLabel.implicitHeight + 4

                                    SwbLabel {
                                        id: typeLabel
                                        anchors.centerIn: parent
                                        text: connectionTypeText || ""
                                        font: FontHelper.smallFont
                                        color: connectionTypeTextColor === "onColor" ? appTheme.textOnColor : SwbTheme.foreground
                                    }
                                }
                            }

                            // 第二行：延迟 + 协议信息
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12

                                // 延迟显示：绿(<60ms) / 橙(<200ms) / 红(>=200ms)
                                SwbLabel {
                                    text: latencyText
                                    font: FontHelper.smallFont
                                    color: latencyLevel === "unknown" ? SwbTheme.mutedForeground : statusColor(latencyLevel)
                                    // 非本机节点或本机有延迟时显示
                                    visible: showLatency
                                }

                                // 协议信息（如 UDP/TCP）
                                SwbLabel {
                                    text: protocol || ""
                                    font: FontHelper.smallFont
                                    color: SwbTheme.mutedForeground
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

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 0

                Item { Layout.preferredHeight: 8 }

                // 空状态：暂无日志
                SwbLabel {
                    visible: VpnRuntimeService.runtimeLogModel.count === 0
                    text: qsTr("暂无日志")
                    font.pixelSize: 24
                    color: SwbTheme.mutedForeground
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                // 日志文本框：使用控件内置滚动条，文本自动避开滚动区域边界
                SwbScrollView {
                    id: logScrollView
                    visible: VpnRuntimeService.runtimeLogModel.count > 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    SwbTextArea {
                        id: runtimeLogTextArea
                        text: VpnRuntimeService.runtimeLogModel.plainText
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextEdit.WrapAnywhere
                        // 日志展示用无背景纯文本形态
                        background: null

                        onTextChanged: cursorPosition = length
                    }
                }
            }
        }
    }

    // 页面底部工具栏：导出日志按钮（运行状态 / 运行日志两个标签页均可见）
    RowLayout {
        Layout.fillWidth: true
        Layout.margins: 8

        Item { Layout.fillWidth: true }

        SwbButton {
            variant: "outline"
            text: qsTr("管理临时节点密钥")
            enabled: NetworkPageViewModel.currentInstanceSecureMode
            onClicked: {
                // 懒加载：active 同步创建实例后立即 open（onLoaded 时机 Dialog 尚未 attach Overlay，open 不生效）
                manageCredentialDialogLoader.active = true
                manageCredentialDialogLoader.item.open()
            }
        }

        SwbButton {
            text: qsTr("导出日志")
            onClicked: exportLogDialog.open()
        }
    }

    // 管理临时节点密钥对话框（懒加载：用到时创建，关闭即卸载释放资源）
    Component {
        id: manageCredentialDialogComponent
        CredentialManageDialog {
            onClosed: manageCredentialDialogLoader.active = false
        }
    }

    Loader {
        id: manageCredentialDialogLoader
        active: false
        sourceComponent: manageCredentialDialogComponent
    }

    // 导出日志文件对话框
    FileDialog {
        id: exportLogDialog
        title: qsTr("导出日志")
        nameFilters: [qsTr("日志文件 (*.log)"), qsTr("所有文件 (*)")]
        fileMode: FileDialog.SaveFile
        currentFile: AppState.homeDirectory + "/qteasytier.log"
        onAccepted: {
            VpnRuntimeService.exportLog(selectedFile.toString())
        }
    }
}
