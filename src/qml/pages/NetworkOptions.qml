/* @brief 网络配置编辑器薄壳：按 ConfigEditorViewModel.formSections 元数据动态渲染两标签页表单，页面自身只保留布局骨架与页面级动作 */
/*
 * NetworkOptions.qml - 网络配置编辑器（数据驱动薄壳）
 *
 * 架构：数据与 UI 分离——
 * - 字段清单、显示名、控件类型、下拉选项、数值范围、分组结构全部由
 *   ConfigEditorViewModel.formSections（CONSTANT 元数据）提供；
 * - 本页面仅按 tab 过滤卡片分组，经 FormField 分发器渲染各字段；
 * - 字段值统一经 ConfigEditorViewModel[fieldKey] 读、setFieldValue(key, value) 写，
 *   继承 ViewModel 的防抖自动保存；
 * - 字段间联动禁用（dhcp→ipv4、白名单开关→输入框）属 UI 逻辑，在本壳实现；
 * - 页面级动作（导出/清空、确认与展示对话框）保留在本壳。
 *
 * 依赖的单例：
 * - ConfigEditorViewModel  表单元数据、字段读写、即时保存（防抖 300ms）
 * - ConfigListModel        导出配置
 * - AppState               错误提示、主目录路径
 * - FontHelper             小字体
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtEasyTier
import SwbControls

/* @brief 配置编辑器根布局，包含 Tab 页签和底部操作栏 */
ColumnLayout {
    id: root

    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: 0

    // 启动/停止过渡期间禁用整个编辑器，避免在异步收敛阶段修改配置
    enabled: !NetworkPageViewModel.currentInstanceBusy
    opacity: enabled ? 1.0 : 0.6
    Behavior on opacity { NumberAnimation { duration: 150 } }

    /* 按标签页 key（basic/advanced）过滤表单卡片分组 */
    function sectionsFor(tabKey) {
        var result = []
        var sections = ConfigEditorViewModel.formSections
        for (var i = 0; i < sections.length; i++) {
            if (sections[i].tab === tabKey)
                result.push(sections[i])
        }
        return result
    }

    /*
     * 字段间联动禁用（纯 UI 逻辑，按 key 特判）：
     * - ipv4 仅在 DHCP 关闭时可编辑
     * - 白名单输入框仅在白名单开关开启时可编辑
     */
    function fieldEnabledByKey(key) {
        if (key === "ipv4")
            return !ConfigEditorViewModel.dhcp
        if (key === "foreignNetworkWhitelist")
            return ConfigEditorViewModel.enableForeignNetworkWhitelist
        return true
    }

    // ============================================
    // Tab 页签容器
    // ============================================
    QtETabWidget {
        id: tabWidget
        Layout.fillWidth: true
        Layout.fillHeight: true

        // ============================================
        // Tab 1: 基础设置
        // ============================================
        SwbScrollView {
            property string tabTitle: qsTr("基础设置")
            id: basicScroll
            contentWidth: availableWidth

            ColumnLayout {
                width: basicScroll.availableWidth
                spacing: 6

                // 区块标题
                SwbLabel {
                    text: qsTr("基础设置")
                    font.pixelSize: 14
                    font.bold: true
                    color: SwbTheme.foreground
                    Layout.topMargin: 12
                    Layout.leftMargin: 16
                    Layout.bottomMargin: 8
                }

                // 表单卡片分组（按元数据动态渲染）
                Repeater {
                    model: root.sectionsFor("basic")

                    delegate: Card {
                        id: formCard
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        contentSpacing: 6

                        // 卡片标题（元数据未提供时不渲染）
                        SwbLabel {
                            text: formCard.modelData.cardTitle
                            font.bold: true
                            visible: formCard.modelData.cardTitle !== ""
                            Layout.bottomMargin: 2
                        }

                        // 字段渲染：FormField 按字段 type 分发到具体渲染器
                        Repeater {
                            model: formCard.modelData.fields

                            delegate: FormField {
                                id: formField
                                required property var modelData

                                field: modelData
                                fieldEnabled: root.fieldEnabledByKey(modelData.key)
                            }
                        }
                    }
                }

                // 底部占位：保证内容不满时卡片不拉伸
                Item { Layout.fillHeight: true }
                // 当前配置实例名提示
                SwbLabel {
                    text: ConfigEditorViewModel.currentInstanceName
                    font: FontHelper.smallFont
                    color: SwbTheme.mutedForeground
                    Layout.leftMargin: 16
                }
                Item { Layout.preferredHeight: 8 }
            }
        }

        // ============================================
        // Tab 2: 高级设置
        // ============================================
        SwbScrollView {
            property string tabTitle: qsTr("高级设置")
            id: advancedScroll
            contentWidth: availableWidth

            ColumnLayout {
                width: advancedScroll.availableWidth
                spacing: 6

                SwbLabel {
                    text: qsTr("高级设置")
                    font.pixelSize: 14
                    font.bold: true
                    color: SwbTheme.foreground
                    Layout.topMargin: 12
                    Layout.leftMargin: 16
                    Layout.bottomMargin: 8
                }

                Repeater {
                    model: root.sectionsFor("advanced")

                    delegate: Card {
                        id: formCard
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        contentSpacing: 6

                        SwbLabel {
                            text: formCard.modelData.cardTitle
                            font.bold: true
                            visible: formCard.modelData.cardTitle !== ""
                            Layout.bottomMargin: 2
                        }

                        Repeater {
                            model: formCard.modelData.fields

                            delegate: FormField {
                                id: formField
                                required property var modelData

                                field: modelData
                                fieldEnabled: root.fieldEnabledByKey(modelData.key)
                            }
                        }
                    }
                }

                // 底部留白，防止内容靠底
                Item { Layout.fillHeight: true; Layout.preferredHeight: 16 }
            }
        }
    }

    // ============================================
    // 底部操作栏：导出配置 / 清空配置
    // 配置采用即时保存（防抖自动落库），不再需要手动保存/取消按钮
    // ============================================
    RowLayout {
        Layout.fillWidth: true
        Layout.margins: 8
        spacing: 8

        SwbButton {
            text: qsTr("导出配置")
            // 无当前配置时禁用
            enabled: ConfigEditorViewModel.currentInstanceName !== ""
            onClicked: exportChoiceDialog.open()
        }

        // 弹性空间，将导出按钮推至左侧、清空按钮推至右侧
        Item { Layout.fillWidth: true }

        SwbButton {
            variant: "outline"
            text: qsTr("清空配置")
            // 无当前配置时禁用
            enabled: ConfigEditorViewModel.currentInstanceName !== ""
            onClicked: resetConfirmDialog.open()
        }
    }

    // 即时保存失败时弹出错误提示，让用户感知配置未能落库
    Connections {
        target: ConfigEditorViewModel
        function onErrorMessagesChanged() {
            var msgs = ConfigEditorViewModel.errorMessages
            if (msgs.length > 0)
                AppState.showError(msgs[0])
        }
    }

    // 页面销毁前刷写防抖窗口内尚未落库的修改，避免最后几秒的编辑丢失
    Component.onDestruction: ConfigEditorViewModel.flushAutoSave()

    // 清空配置确认：将当前实例的全部网络设置恢复为默认值（不可恢复）
    SwbDialog {
        id: resetConfirmDialog
        title: qsTr("清空配置")
        standardButtons: Dialog.Yes | Dialog.No
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(360, parent ? parent.width - 48 : 320)

        SwbLabel {
            text: qsTr("将把当前实例「%1」的全部网络设置恢复为默认值。此操作不可恢复！\n\n是否继续？")
                .arg(ConfigEditorViewModel.displayName)
            wrapMode: Text.WordWrap
            width: parent ? parent.width : 320
        }

        onAccepted: ConfigEditorViewModel.resetToDefaults()
    }

    // 导出方式选择对话框
    SwbDialog {
        id: exportChoiceDialog
        title: qsTr("导出配置")
        parent: Overlay.overlay
        anchors.centerIn: parent
        RowLayout {
            spacing: 12
            SwbButton {
                text: qsTr("导出为文件")
                Layout.fillWidth: true
                onClicked: {
                    exportChoiceDialog.close()
                    exportFileDialog.open()
                }
            }
            SwbButton {
                variant: "outline"
                text: qsTr("导出为 URL")
                Layout.fillWidth: true
                onClicked: {
                    exportChoiceDialog.close()
                    var urlStr = ConfigListModel.exportConfigUrl(ConfigEditorViewModel.currentInstanceName)
                    if (urlStr !== "") {
                        exportUrlField.text = urlStr
                        exportUrlDialog.open()
                    }
                }
            }
        }
    }

    // 导出 URL 展示对话框
    SwbDialog {
        id: exportUrlDialog
        title: qsTr("导出 URL")
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(520, parent ? parent.width - 48 : 480)
        ColumnLayout {
            width: parent ? parent.width : 480
            spacing: 8
            SwbLabel { text: qsTr("复制以下 URL 即可分享配置：") }
            SwbScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 100

                SwbTextArea {
                    id: exportUrlField
                    readOnly: true
                    wrapMode: TextEdit.WrapAnywhere
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                SwbButton {
                    text: qsTr("复制")
                    onClicked: {
                        exportUrlField.selectAll()
                        exportUrlField.copy()
                    }
                }
                SwbButton {
                    variant: "outline"
                    text: qsTr("确定")
                    onClicked: exportUrlDialog.close()
                }
            }
        }
    }

    // 导出配置文件对话框
    FileDialog {
        id: exportFileDialog
        title: qsTr("导出配置")
        nameFilters: [qsTr("TOML 文件 (*.toml)"), qsTr("所有文件 (*)")]
        fileMode: FileDialog.SaveFile
        currentFile: AppState.homeDirectory + "/" + ConfigEditorViewModel.currentInstanceName + ".toml"
        onAccepted: {
            var url = selectedFile.toString()
            // 自动补全 .toml 后缀
            if (!url.endsWith(".toml"))
                url += ".toml"
            ConfigListModel.exportConfigFile(ConfigEditorViewModel.currentInstanceName, url)
        }
    }
}
