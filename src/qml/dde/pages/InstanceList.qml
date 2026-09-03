/* @file InstanceList.qml (DDE)
 * @brief DDE 版实例列表：网络配置实例的选择、新建/导入、右键重命名/删除与启动/停止控制
 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

Rectangle {
    id: root

    color: palette.window
    implicitWidth: 200
    implicitHeight: 400

    Theme { id: appTheme }

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

    /* 待删除的实例名与显示名，弹窗确认后执行删除 */
    property string pendingDeleteInstance: ""
    property string pendingDeleteLabel: ""
    /* 待重命名的实例名，弹窗确认后执行重命名 */
    property string pendingRenameInstance: ""

    /* 弹出删除确认对话框，绑定待删除实例名 */
    function requestDelete(instanceName, labelText) {
        root.pendingDeleteInstance = instanceName
        root.pendingDeleteLabel = labelText
        deleteDialog.open()
    }

    /* 弹出重命名对话框，预填当前名称 */
    function requestRename(instanceName, labelText) {
        root.pendingRenameInstance = instanceName
        renameDialog.inputText = labelText
        renameDialog.open()
    }

    /* 选中并加载指定配置实例到右侧编辑面板 */
    function selectInstance(instanceName) {
        root.configSelected(instanceName)
    }

    // ============================================
    // 删除确认对话框（模态 DialogWindow 基座）
    // ============================================
    ConfirmDialog {
        id: deleteDialog

        headerTitle: qsTr("删除配置")
        danger: true
        confirmText: qsTr("删除")
        message: root.pendingDeleteLabel !== ""
            ? qsTr("确定要删除配置「%1」吗？此操作不可撤销。").arg(root.pendingDeleteLabel)
            : ""

        onAccepted: {
            if (root.pendingDeleteInstance !== "") {
                root.deleteRequested(root.pendingDeleteInstance)
                root.pendingDeleteInstance = ""
                root.pendingDeleteLabel = ""
            }
        }
        onRejected: {
            root.pendingDeleteInstance = ""
            root.pendingDeleteLabel = ""
        }
    }

    // ============================================
    // 重命名对话框（输入模式基座）
    // ============================================
    ConfirmDialog {
        id: renameDialog

        headerTitle: qsTr("重命名配置")
        inputMode: true
        confirmText: qsTr("保存")
        inputPlaceholder: qsTr("请输入新的配置名称")

        onAccepted: {
            var newName = inputText.trim()
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
            text: qsTr("实例列表")
            font.pixelSize: 24
            font.bold: true
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

            // 列表项代理：卡片式条目，选中项叠加高亮底色与描边，运行状态以状态色指示条表达
            delegate: Rectangle {
                id: itemRoot

                width: configListView.width
                height: 50
                radius: 8
                color: itemRoot.isSelected
                     ? Qt.rgba(palette.highlight.r, palette.highlight.g, palette.highlight.b, 0.18)
                     : palette.base
                border.width: 1
                border.color: (itemRoot.isSelected || itemHover.hovered)
                              ? palette.highlight
                              : Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.15)

                Behavior on color {
                    ColorAnimation { duration: 120 }
                }
                Behavior on border.color {
                    ColorAnimation { duration: 120 }
                }

                property string instanceName: model.instanceName
                property string labelText: model.displayName || model.instanceName
                property bool isRunning: model.running || false
                property int runState: model.runState || 0
                // 启动/停止过渡期间禁止重复启停操作（ConfigRunState: Starting=1, Stopping=3）
                property bool isBusy: runState === 1 || runState === 3
                // 外部实例：daemon 中存在但本地配置列表中没有的实例
                property bool isExternal: model.isExternal || false
                // 判断是否与 ViewModel 当前编辑的配置一致
                readonly property bool isSelected:
                    NetworkPageViewModel.currentInstanceName === instanceName

                HoverHandler {
                    id: itemHover
                    cursorShape: Qt.PointingHandCursor
                }

                // 左侧运行状态指示条（状态色：运行=绿，过渡=橙，错误=红）
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.topMargin: 4
                    anchors.bottomMargin: 4
                    width: 3
                    radius: 2
                    visible: isRunning || isBusy || runState === 4
                    color: runState === 4 ? appTheme.statusRed
                         : isBusy ? appTheme.statusOrange
                         : appTheme.statusGreen
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
                            // 外部实例无右键菜单（重命名/删除对无本地配置的实例无意义）
                            if (!isExternal)
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
                        if (!isExternal)
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
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        // 运行状态文案（完整状态，区分启动/停止过渡与错误）
                        Label {
                            text: {
                                if (isExternal)
                                    return qsTr("运行中 · 外部实例")
                                if (runState === 1)
                                    return qsTr("启动中")
                                if (runState === 2)
                                    return qsTr("运行中")
                                if (runState === 3)
                                    return qsTr("停止中")
                                if (runState === 4)
                                    return qsTr("运行错误")
                                return qsTr("未运行")
                            }
                            font: FontHelper.smallFont
                            color: runState === 4 ? appTheme.statusRed
                                 : isBusy ? appTheme.statusOrange
                                 : isRunning ? appTheme.statusGreen
                                 : palette.placeholderText
                        }
                    }

                    // 启动/停止按钮，图标根据运行状态切换；过渡期间禁用防止重复操作
                    IconToolButton {
                        iconSource: isRunning ? "qrc:/icons/stop.svg" : "qrc:/icons/play.svg"
                        iconSize: 24
                        buttonSize: 36
                        flat: true
                        enabled: !isBusy
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
            spacing: 6

            RecommandButton {
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
