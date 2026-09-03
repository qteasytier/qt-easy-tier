/* @file NetworkRuningStatus.qml (DDE)
 * @brief DDE 版网络运行状态页面：DTK 控件重写，通过自定义页签展示当前网络的节点信息列表和运行日志，支持日志导出
 *
 * 页签用 DTK 下划线页签头（TabHeader）；管理临时节点密钥对话框经 Loader 懒加载
 * （dde/components/CredentialManageDialog，模态 DialogWindow）。
 */
import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QtEasyTier
import org.deepin.dtk

// 网络运行状态页面：展示当前网络的节点信息与运行日志
/* @brief 运行状态根容器，包含节点信息 Tab、运行日志 Tab 与页面级导出日志按钮 */
ColumnLayout {
    id: root

    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: 0

    Theme { id: appTheme }

    /* 分隔线颜色（palette 派生） */
    readonly property color dividerColor: Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.15)

    /* 根据连接类型等级字符串返回对应颜色 */
    function statusColor(level) {
        if (level === "green") return appTheme.statusGreen
        if (level === "orange") return appTheme.statusOrange
        if (level === "red") return appTheme.statusRed
        if (level === "blue") return appTheme.statusBlue
        return root.dividerColor
    }

    /* 状态色的半透明变体（徽章底色） */
    function statusAlpha(color, alpha) {
        return Qt.rgba(color.r, color.g, color.b, alpha)
    }

    // 标签栏：DTK 下划线页签头（自带底部分隔线）
    TabHeader {
        id: tabBar
        Layout.fillWidth: true
        tabs: [qsTr("运行状态"), qsTr("运行日志")]
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
                Label {
                    visible: VpnRuntimeService.nodeInfoModel.count === 0
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
                    visible: VpnRuntimeService.nodeInfoModel.count > 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: VpnRuntimeService.nodeInfoModel
                    spacing: 6
                    clip: true

                    ScrollBar.vertical: ScrollBar {}

                    // 每个节点用 Card 组件展示（dde/components/，经 QtEasyTier 导入解析）
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
                                Label {
                                    text: virtualIp || ""
                                    font.bold: true
                                }

                                // 主机名
                                Label {
                                    text: hostname || ""
                                }

                                Item { Layout.fillWidth: true }

                                // 本机标记：柔和前景色徽章
                                Rectangle {
                                    visible: isLocalNode === true
                                    radius: 4
                                    color: Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.12)
                                    implicitWidth: localLabel.implicitWidth + 8
                                    implicitHeight: localLabel.implicitHeight + 4

                                    Label {
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
                                    color: root.statusColor(connectionTypeLevel)
                                    implicitWidth: typeLabel.implicitWidth + 8
                                    implicitHeight: typeLabel.implicitHeight + 4

                                    Label {
                                        id: typeLabel
                                        anchors.centerIn: parent
                                        text: connectionTypeText || ""
                                        font: FontHelper.smallFont
                                        color: connectionTypeTextColor === "onColor" ? appTheme.textOnColor : palette.windowText
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

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 0

                Item { Layout.preferredHeight: 8 }

                // 空状态：暂无日志
                Label {
                    visible: VpnRuntimeService.runtimeLogModel.count === 0
                    text: qsTr("暂无日志")
                    font.pixelSize: 24
                    color: palette.placeholderText
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                // 日志只读文本：Flickable 承载滚动 + TextArea 无背景纯文本形态
                Flickable {
                    id: logScrollView
                    visible: VpnRuntimeService.runtimeLogModel.count > 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: width
                    contentHeight: runtimeLogTextArea.implicitHeight

                    ScrollBar.vertical: ScrollBar {}

                    TextArea {
                        id: runtimeLogTextArea
                        width: logScrollView.width
                        text: VpnRuntimeService.runtimeLogModel.plainText
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextEdit.WrapAnywhere
                        // 日志展示用无背景纯文本形态
                        background: null

                        onTextChanged: cursorPosition = length

                        // 文本变化时滚动到底部
                        onImplicitHeightChanged: {
                            logScrollView.contentY = Math.max(0, logScrollView.contentHeight - logScrollView.height)
                        }
                    }
                }
            }
        }
    }

    // 页面底部工具栏：管理密钥 / 导出日志按钮（两个标签页均可见）
    RowLayout {
        Layout.fillWidth: true
        Layout.margins: 8

        Item { Layout.fillWidth: true }

        Button {
            text: qsTr("管理临时节点密钥")
            enabled: NetworkPageViewModel.currentInstanceSecureMode
            onClicked: {
                // 懒加载：active 同步创建实例后立即 open（DialogWindow 为独立窗，无须等待 Overlay）
                manageCredentialDialogLoader.active = true
                manageCredentialDialogLoader.item.open()
            }
        }

        RecommandButton {
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
