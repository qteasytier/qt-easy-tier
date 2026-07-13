/* @brief 左侧面板：显示网络配置列表、支持新建/导入配置、右键菜单（重命名/删除）及配置启动/停止控制 */
/*
 * LeftPanel.qml - 左侧面板
 *
 * 主要功能：
 * - 显示网络配置列表（ListView），支持选中、播放/停止切换
 * - 新建/导入配置入口
 * - 右键菜单（重命名、删除）及对应弹窗
 *
 * 依赖的单例：
 * - ConfigListModel      配置列表数据模型
 * - NetworkPageViewModel 当前页面状态
 * - FontHelper           小字体辅助
 *
 * 所有颜色使用系统 palette，不硬编码 hex 色值。
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtEasyTier

/* @brief 左侧面板根容器，包含配置列表和操作按钮 */
Rectangle {
    id: root

    // 使用系统窗口背景色
    color: palette.window
    implicitWidth: 200
    implicitHeight: 400

    /* 选中指定配置实例，触发右侧面板加载 */
    signal configSelected(string instanceName)
    /* 请求新建一个配置实例 */
    signal createRequested()
    /* 请求删除指定配置实例 */
    signal deleteRequested(string instanceName)
    /* 请求重命名指定配置实例 */
    signal renameRequested(string instanceName, string newDisplayName)
    /* 请求启动指定配置实例 */
    signal startRequested(string instanceName)
    /* 请求停止指定配置实例 */
    signal stopRequested(string instanceName)
    /* 请求打开导入配置文件对话框 */
    signal importRequested()

    /* 待删除的实例名，弹窗确认后执行删除 */
    property string pendingDeleteInstance: ""
    /* 待重命名的实例名，弹窗确认后执行重命名 */
    property string pendingRenameInstance: ""

    /* 弹出删除确认对话框，绑定待删除实例名 */
    function requestDelete(instanceName, labelText) {
        root.pendingDeleteInstance = instanceName
        deleteDialog.text = qsTr("确定要删除配置「%1」吗？此操作不可撤销。").arg(labelText)
        deleteDialog.open()
    }

    /* 弹出重命名对话框，预填当前名称 */
    function requestRename(instanceName, labelText) {
        root.pendingRenameInstance = instanceName
        renameField.text = labelText
        renameDialog.open()
    }

    /* 选中并加载指定配置实例到右侧编辑面板 */
    function selectInstance(instanceName) {
        root.configSelected(instanceName)
    }

    // ============================================
    // 删除确认对话框
    // ============================================
    MessageDialog {
        id: deleteDialog
        title: qsTr("删除配置")
        buttons: MessageDialog.Yes | MessageDialog.No
        onAccepted: {
            if (root.pendingDeleteInstance !== "") {
                root.deleteRequested(root.pendingDeleteInstance)
                root.pendingDeleteInstance = ""
            }
        }
        onRejected: root.pendingDeleteInstance = ""
    }

    // ============================================
    // 重命名对话框
    // ============================================
    Dialog {
        id: renameDialog
        title: qsTr("重命名配置")
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        // 弹出层居中，避免覆盖底层
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(420, parent ? parent.width - 48 : 360)

        TextField {
            id: renameField
            width: parent.width
            placeholderText: qsTr("请输入新的配置名称")
        }

        onOpened: {
            renameField.forceActiveFocus()
            renameField.selectAll()
        }
        onAccepted: {
            var newName = renameField.text.trim()
            if (newName === "" || root.pendingRenameInstance === "") {
                root.pendingRenameInstance = ""
                return
            }
            root.renameRequested(root.pendingRenameInstance, newName)
            root.pendingRenameInstance = ""
        }
        onRejected: root.pendingRenameInstance = ""
    }

    // ============================================
    // 主体布局
    // ============================================
    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 16
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        anchors.bottomMargin: 8
        spacing: 12

        // 标题
        Label {
            text: qsTr("网络配置")
            font.pixelSize: 24
            font.bold: true
            color: palette.highlight
        }

        // 空状态提示
        Label {
            visible: configListView.count === 0
            text: qsTr("暂无网络配置，点击下方按钮新建")
            color: palette.placeholderText
            Layout.fillWidth: true
            Layout.topMargin: 20
            horizontalAlignment: Text.AlignHCenter
        }

        // 配置列表
        ListView {
            id: configListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: ConfigListModel

            ScrollBar.vertical: ScrollBar {}

            // 列表项代理
            delegate: Rectangle {
                width: configListView.width
                height: 50
                radius: 6
                color: palette.base
                // 选中时使用高亮色边框
                border.color: isSelected ? palette.highlight
                    : Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.12)
                border.width: isSelected ? 1.5 : 1

                property string instanceName: model.instanceName
                property string labelText: model.displayName || model.instanceName
                property bool isRunning: model.running || false
                // 判断是否与 ViewModel 当前编辑的配置一致
                readonly property bool isSelected:
                    NetworkPageViewModel.currentInstanceName === instanceName

                // 左侧运行状态指示条
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.topMargin: 4
                    anchors.bottomMargin: 4
                    width: 3
                    radius: 2
                    color: isRunning ? palette.highlight : "transparent"
                }

                // 单击选中/右键菜单/长按菜单
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: function(mouse) {
                        if (mouse.button === Qt.LeftButton) {
                            root.selectInstance(instanceName)
                        } else if (mouse.button === Qt.RightButton) {
                            root.selectInstance(instanceName)
                            contextMenu.popup()
                        }
                    }

                    Menu {
                        id: contextMenu
                        MenuItem {
                            text: qsTr("重命名")
                            onTriggered: root.requestRename(instanceName, labelText)
                        }
                        MenuItem {
                            text: qsTr("删除")
                            onTriggered: root.requestDelete(instanceName, labelText)
                        }
                    }

                    onPressAndHold: {
                        root.selectInstance(instanceName)
                        contextMenu.popup()
                    }
                }

                // 列表项内容：名称 + 状态 + 启动/停止按钮
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 4

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Layout.leftMargin: 6

                        Label {
                            text: labelText
                            font.bold: true
                            color: palette.windowText
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        // 运行/未运行状态
                        Label {
                            text: isRunning ? qsTr("运行中") : qsTr("未运行")
                            font: FontHelper.smallFont
                            color: isRunning ? palette.highlight : palette.placeholderText
                        }
                    }

                    // 启动/停止按钮，图标根据运行状态切换
                    ToolButton {
                        icon.source: isRunning ? "qrc:/icons/stop.svg" : "qrc:/icons/play.svg"
                        icon.width: 32
                        icon.height: 32
                        padding: 0
                        flat: true
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: {
                            root.selectInstance(instanceName)
                            if (isRunning) {
                                root.stopRequested(instanceName)
                            } else {
                                root.startRequested(instanceName)
                            }
                        }
                    }
                }
            }
        }

        // 底部操作按钮区
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
                Layout.fillWidth: true
                text: qsTr("新建配置")
                onClicked: root.createRequested()
            }

            Button {
                Layout.fillWidth: true
                text: qsTr("导入配置")
                onClicked: importRequested()
            }
        }
    }
}
