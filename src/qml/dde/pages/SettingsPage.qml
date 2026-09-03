/* @file SettingsPage.qml (DDE)
 * @brief DDE 版设置页面：DTK 控件重写，管理应用行为设置（自启/回连）、日志设置、版本信息和关于链接
 *
 * 相对共享版无主题设置项：DDE 前端主题由标题栏 DTK ThemeMenu（系统级主题）接管。
 * 危险操作确认对话框均为模态 DialogWindow（ConfirmDialog 基座）。
 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

// 设置页面：应用行为、日志、版本信息和关于
/* @brief 设置页面根容器，以 Card 分区展示各类设置 */
Rectangle {
    id: root

    color: palette.window

    /* 说明性文字颜色，用于开关下方的补充说明 */
    readonly property color dimTextColor: palette.placeholderText

    Theme { id: appTheme }

    Flickable {
        id: scrollView
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: settingsColumn.implicitHeight + 16

        ScrollBar.vertical: ScrollBar {}

        ColumnLayout {
            id: settingsColumn
            width: scrollView.width
            spacing: 10

            // 页面标题
            Label {
                text: qsTr("设置")
                font.pixelSize: 24
                font.bold: true
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
                    Layout.bottomMargin: 4
                }

                // 开机自启开关：操作失败时恢复到系统真实状态（settings3.json 不再持久化该字段）
                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("开机自启")
                    }

                    Switch {
                        id: autoStartSwitch
                        checked: SettingsViewModel.autoStart
                        Component.onCompleted: SettingsViewModel.refreshAutoStart()
                        onToggled: {
                            if (!SettingsViewModel.setAutoStart(checked)) {
                                // 设置失败：恢复开关，重建到 ViewModel 的声明式绑定
                                checked = Qt.binding(function() { return SettingsViewModel.autoStart })
                            }
                        }
                    }
                }

                Label {
                    text: qsTr("开机自动启动前端 GUI 程序")
                    font: FontHelper.smallFont
                    color: dimTextColor
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Layout.bottomMargin: 8
                }

                // 自动回连开关：通过后端 RPC 操作，未连接/正在连接/请求中时禁用
                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("自动回连")
                    }

                    Switch {
                        id: autoReconnectSwitch
                        checked: SettingsViewModel.autoReconnect
                        enabled: BackendStatusViewModel.connected
                                 && !BackendStatusViewModel.connecting
                                 && !SettingsViewModel.autoReconnectBusy
                        onToggled: SettingsViewModel.setAutoReconnectEnabled(checked)
                    }
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
                    Layout.bottomMargin: 8
                }

                // 隐藏服务节点开关：只过滤运行状态页展示，不影响后台缓存
                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("隐藏服务节点")
                    }

                    Switch {
                        checked: SettingsViewModel.hideServerNodes
                        onToggled: SettingsViewModel.hideServerNodes = checked
                    }
                }

                Label {
                    text: qsTr("运行状态页面不显示服务器节点的信息")
                    font: FontHelper.smallFont
                    color: dimTextColor
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Layout.bottomMargin: 8
                }

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("自动检查更新")
                    }

                    Switch {
                        checked: SettingsViewModel.autoCheckUpdates
                        onToggled: SettingsViewModel.autoCheckUpdates = checked
                    }
                }

                Label {
                    text: qsTr("程序启动时自动检查更新，有新版本时通知")
                    font: FontHelper.smallFont
                    color: dimTextColor
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
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
                borderColor: appTheme.statusRed

                // 进入页面时刷新后端按钮状态
                Component.onCompleted: DangerousOperationViewModel.refreshDaemonStatus()

                Label {
                    text: qsTr("危险操作")
                    font.pixelSize: 14
                    font.bold: true
                    color: appTheme.statusRed
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

                    WarningButton {
                        text: qsTr("清空配置")
                        enabled: !DangerousOperationViewModel.busy
                        Layout.fillWidth: true
                        onClicked: clearConfirmDialog.open()
                    }
                }
            }

            // 版权信息
            Label {
                text: qsTr("Copyright 2026 QtEasyTier. All rights reserved.")
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

    // ========== 危险操作确认对话框（模态 DialogWindow 基座） ==========

    // 安装后端确认：安装包括注册并启动系统服务
    ConfirmDialog {
        id: installDialog

        headerTitle: qsTr("安装后端")
        confirmText: qsTr("继续")
        message: qsTr("将把后端 qtet-daemon 注册为系统服务并启动，需要管理员权限。\n\n是否继续？")

        onAccepted: DangerousOperationViewModel.performDaemonOperation()
    }

    // 卸载后端确认：卸载包括停止并移除服务
    ConfirmDialog {
        id: uninstallDialog

        headerTitle: qsTr("卸载后端")
        danger: true
        confirmText: qsTr("卸载")
        message: qsTr("将停止并卸载后端 qtet-daemon 系统服务，需要管理员权限。\n卸载后将无法连接 EasyTier VPN。\n\n是否继续？")

        onAccepted: DangerousOperationViewModel.performDaemonOperation()
    }

    // 清空配置确认：先停止所有网络服务，再删除全部数据并退出
    ConfirmDialog {
        id: clearConfirmDialog

        headerTitle: qsTr("清空配置")
        danger: true
        confirmText: qsTr("清空")
        message: qsTr("卸载配置需要先停止所有运行中的网络配置，然后清空全部数据（网络配置、收藏节点、日志、全局设置）。\n此操作不可恢复，操作完成后需要重启本程序。\n\n是否继续？")

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
