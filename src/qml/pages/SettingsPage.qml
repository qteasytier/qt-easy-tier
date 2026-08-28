/* @brief 设置页面：管理应用行为设置（自启/回连）、日志设置、版本信息和关于链接 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtEasyTier

// 设置页面：应用行为、日志、版本信息和关于
/* @brief 设置页面根容器，以 Card 分区展示各类设置 */
Rectangle {
    id: root

    color: palette.window

    /* 说明性文字颜色，用于开关下方的补充说明 */
    readonly property color dimTextColor: palette.highlight

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 10

            // 页面标题
            Label {
                text: qsTr("设置")
                font.pixelSize: 24
                font.bold: true
                color: palette.highlight
                Layout.topMargin: 16
                Layout.leftMargin: 16
            }

            // ========== 行为设置 ==========
            Card {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                Label {
                    text: qsTr("行为设置")
                    font.pixelSize: 14
                    font.bold: true
                    color: palette.highlight
                    Layout.bottomMargin: 4
                }

                // 开机自启开关：操作失败时恢复到系统真实状态（settings3.json 不再持久化该字段）
                Switch {
                    id: autoStartSwitch
                    text: qsTr("开机自启")
                    checked: SettingsViewModel.autoStart
                    Component.onCompleted: SettingsViewModel.refreshAutoStart()
                    onToggled: {
                        if (!SettingsViewModel.setAutoStart(checked)) {
                            // 设置失败：恢复开关，重建到 ViewModel 的声明式绑定
                            checked = Qt.binding(function() { return SettingsViewModel.autoStart })
                        }
                    }
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("开机自动启动前端 GUI 程序")
                    font: FontHelper.smallFont
                    color: dimTextColor
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Layout.leftMargin: 42
                    Layout.bottomMargin: 8
                }

                // 自动回连开关：通过后端 RPC 操作，未连接/正在连接/请求中时禁用
                Switch {
                    id: autoReconnectSwitch
                    text: qsTr("自动回连")
                    checked: SettingsViewModel.autoReconnect
                    enabled: BackendStatusViewModel.connected
                             && !BackendStatusViewModel.connecting
                             && !SettingsViewModel.autoReconnectBusy
                    onToggled: SettingsViewModel.setAutoReconnectEnabled(checked)
                    Layout.fillWidth: true
                }

                Connections {
                    target: SettingsViewModel
                    function onAutoReconnectOperationFailed(message) {
                        AppState.showError(message)
                        autoReconnectSwitch.checked = Qt.binding(function() {
                            return SettingsViewModel.autoReconnect
                        })
                    }
                }

                Label {
                    text: qsTr("程序启动时自动连接上次退出时正在运行的网络")
                    font: FontHelper.smallFont
                    color: dimTextColor
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Layout.leftMargin: 42
                    Layout.bottomMargin: 8
                }

                // 隐藏服务节点开关：只过滤运行状态页展示，不影响后台缓存
                Switch {
                    text: qsTr("隐藏服务节点")
                    checked: SettingsViewModel.hideServerNodes
                    onToggled: SettingsViewModel.hideServerNodes = checked
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("运行状态页面不显示服务器节点的信息")
                    font: FontHelper.smallFont
                    color: dimTextColor
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Layout.leftMargin: 42
                    Layout.bottomMargin: 8
                }

                Switch {
                    text: qsTr("自动检查更新")
                    checked: SettingsViewModel.autoCheckUpdates
                    onToggled: SettingsViewModel.autoCheckUpdates = checked
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("程序启动时自动检查更新，有新版本时通知")
                    font: FontHelper.smallFont
                    color: dimTextColor
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Layout.leftMargin: 42
                    Layout.bottomMargin: 8
                }
            }

            // ========== 日志设置 ==========
            Card {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                contentSpacing: 12

                Label {
                    text: qsTr("日志设置")
                    font.pixelSize: 14
                    font.bold: true
                    color: palette.highlight
                    Layout.bottomMargin: 4
                }

                Label { text: qsTr("日志输出等级") }

                ComboBox {
                    Layout.fillWidth: true
                    model: [qsTr("信息"), qsTr("警告"), qsTr("错误"), qsTr("关闭")]
                    currentIndex: SettingsViewModel.logLevel
                    onActivated: SettingsViewModel.logLevel = currentIndex
                }

                Label { text: qsTr("最大日志保存条数") }

                SpinBox {
                    Layout.fillWidth: true
                    from: 1
                    to: 1000
                    value: SettingsViewModel.maxLogEntries
                    onValueModified: SettingsViewModel.maxLogEntries = value
                }

            }

            // ========== 版本信息 ==========
            Card {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                Label {
                    text: qsTr("版本信息")
                    font.pixelSize: 14
                    font.bold: true
                    color: palette.highlight
                    Layout.bottomMargin: 4
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 4

                    Label {
                        text: qsTr("QtEasyTier %1 (Desktop)").arg(SettingsViewModel.frontendVersion)
                        Layout.fillWidth: true
                    }

                    Button {
                        text: SettingsViewModel.updateCheckBusy ? qsTr("检查中...") : qsTr("检查更新")
                        enabled: !SettingsViewModel.updateCheckBusy
                        onClicked: SettingsViewModel.checkForUpdates()
                    }
                }

                Label {
                    text: qsTr("基于 EasyTier %1").arg(SettingsViewModel.easyTierVersion)
                    font: FontHelper.smallFont
                    color: dimTextColor
                }
            }

            // ========== 关于 ==========
            Card {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                Label {
                    text: qsTr("关于")
                    font.pixelSize: 14
                    font.bold: true
                    color: palette.highlight
                    Layout.bottomMargin: 4
                }

                // 链接按钮网格
                GridLayout {
                    columns: 2
                    Layout.fillWidth: true
                    columnSpacing: 8
                    rowSpacing: 8
                    uniformCellWidths: true

                    Button {
                        text: qsTr("关于项目")
                        Layout.fillWidth: true
                        onClicked: Qt.openUrlExternally("https://qtet.cn")
                    }

                    Button {
                        text: qsTr("关于 EasyTier")
                        Layout.fillWidth: true
                        onClicked: Qt.openUrlExternally("https://easytier.cn")
                    }

                    Button {
                        text: qsTr("使用帮助")
                        Layout.fillWidth: true
                        onClicked: Qt.openUrlExternally("https://qtet.cn/docs-home")
                    }

                    Button {
                        text: qsTr("捐赠本项目")
                        Layout.fillWidth: true
                        onClicked: Qt.openUrlExternally("https://qtet.cn/other/donate/")
                    }
                }
            }

            // ========== 危险操作 ==========
            Card {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                borderColor: "#C62828"

                // 进入页面时刷新后端按钮状态
                Component.onCompleted: DangerousOperationViewModel.refreshDaemonStatus()

                Label {
                    text: qsTr("危险操作")
                    font.pixelSize: 14
                    font.bold: true
                    color: "#C62828"
                    Layout.bottomMargin: 4
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 4

                    // 后端安装/卸载：按钮文本与动作随后端状态切换
                    Button {
                        text: DangerousOperationViewModel.daemonInstalled ? qsTr("卸载后端") : qsTr("安装后端")
                        enabled: DangerousOperationViewModel.daemonOperationEnabled
                                 && !DangerousOperationViewModel.busy
                        Layout.fillWidth: true
                        onClicked: {
                            if (DangerousOperationViewModel.daemonInstalled)
                                uninstallDialog.open()
                            else
                                installDialog.open()
                        }
                    }

                    Button {
                        text: qsTr("清空配置")
                        enabled: !DangerousOperationViewModel.busy
                        Layout.fillWidth: true
                        onClicked: clearConfirmDialog.open()
                    }
                }
            }

            // 版权信息
            Label {
                text: qsTr("Copyright 2026 明月清风. All rights reserved.")
                font: FontHelper.smallFont
                color: dimTextColor
                Layout.alignment: Qt.AlignHCenter
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                Layout.topMargin: 8
            }

            // 底部留白
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 16
            }
        }
    }

    // ========== 危险操作确认对话框 ==========

    // 安装后端确认：安装包括注册并启动系统服务
    Dialog {
        id: installDialog
        title: qsTr("安装后端")
        standardButtons: Dialog.Yes | Dialog.No
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(360, parent ? parent.width - 48 : 320)

        Label {
            text: qsTr("将把后端 qtet-daemon 注册为系统服务并启动，需要管理员权限。\n\n是否继续？")
            wrapMode: Text.WordWrap
            width: parent ? parent.width : 320
        }

        onAccepted: DangerousOperationViewModel.performDaemonOperation()
    }

    // 卸载后端确认：卸载包括停止并移除服务
    Dialog {
        id: uninstallDialog
        title: qsTr("卸载后端")
        standardButtons: Dialog.Yes | Dialog.No
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(360, parent ? parent.width - 48 : 320)

        Label {
            text: qsTr("将停止并卸载后端 qtet-daemon 系统服务，需要管理员权限。\n卸载后将无法连接 EasyTier VPN。\n\n是否继续？")
            wrapMode: Text.WordWrap
            width: parent ? parent.width : 320
        }

        onAccepted: DangerousOperationViewModel.performDaemonOperation()
    }

    // 清空配置确认：先停止所有网络服务，再删除全部数据并退出
    Dialog {
        id: clearConfirmDialog
        title: qsTr("清空配置")
        standardButtons: Dialog.Yes | Dialog.No
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(360, parent ? parent.width - 48 : 320)

        Label {
            text: qsTr("卸载配置需要先停止所有运行中的网络配置，然后清空全部数据（网络配置、收藏节点、日志、全局设置）。\n此操作不可恢复，操作完成后需要重启本程序。\n\n是否继续？")
            wrapMode: Text.WordWrap
            width: parent ? parent.width : 320
        }

        onAccepted: DangerousOperationViewModel.clearAllData()
    }

    // 危险操作结果处理：仅失败时需要提示（成功时应用会自动退出）
    Connections {
        target: DangerousOperationViewModel
        function onOperationFinished(success, message) {
            if (!success)
                AppState.showError(message)
        }
    }
}
