/* @file NetworkOptions.qml (DDE)
 * @brief DDE 版网络配置编辑器薄壳：DTK 控件重写，按 ConfigEditorViewModel.formSections 元数据动态渲染两标签页表单，页面自身只保留布局骨架与页面级动作
 *
 * 架构：数据与 UI 分离——
 * - 字段清单、显示名、控件类型、下拉选项、数值范围、分组结构全部由
 *   ConfigEditorViewModel.formSections（CONSTANT 元数据）提供；
 * - 本页面仅按 tab 过滤卡片分组，经 FormField 分发器渲染各字段
 *   （渲染器实现位于 dde/config_form/，经 QtEasyTier 显式导入解析）；
 * - 字段值统一经 ConfigEditorViewModel[fieldKey] 读、setFieldValue(key, value) 写，
 *   继承 ViewModel 的防抖自动保存；
 * - 字段间联动禁用（dhcp→ipv4、白名单开关→输入框）属 UI 逻辑，在本壳实现；
 * - 页面级动作（导出/清空、确认与展示对话框，均为模态 DialogWindow）保留在本壳。
 *
 * 依赖的单例：
 * - ConfigEditorViewModel  表单元数据、字段读写、即时保存（防抖 300ms）
 * - ConfigListModel        导出配置
 * - AppState               错误提示、主目录路径
 * - FontHelper             小字体
 */
import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

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
    // Tab 页签容器：DTK 下划线页签头（自带底部分隔线）
    // ============================================
    TabHeader {
        id: tabBar
        Layout.fillWidth: true
        tabs: [qsTr("基础设置"), qsTr("高级设置")]
    }

    // 内容区：两个标签页按索引切换
    StackLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: tabBar.currentIndex

        // ============================================
        // Tab 1: 基础设置
        // ============================================
        Flickable {
            id: basicScroll
            clip: true
            contentWidth: width
            contentHeight: basicColumn.implicitHeight + 16

            ScrollBar.vertical: ScrollBar {}

            ColumnLayout {
                id: basicColumn
                width: basicScroll.width
                spacing: 6

                // 区块标题
                Label {
                    text: qsTr("基础设置")
                    font.pixelSize: 14
                    font.bold: true
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
                        // 加粗标题由 Card 内置渲染（元数据未提供时不显示）
                        title: modelData.cardTitle

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

                // 当前配置实例名提示
                Label {
                    text: ConfigEditorViewModel.currentInstanceName
                    font: FontHelper.smallFont
                    color: palette.placeholderText
                    Layout.leftMargin: 16
                }
                Item { Layout.preferredHeight: 8 }
            }
        }

        // ============================================
        // Tab 2: 高级设置
        // ============================================
        Flickable {
            id: advancedScroll
            clip: true
            contentWidth: width
            contentHeight: advancedColumn.implicitHeight + 16

            ScrollBar.vertical: ScrollBar {}

            ColumnLayout {
                id: advancedColumn
                width: advancedScroll.width
                spacing: 6

                Label {
                    text: qsTr("高级设置")
                    font.pixelSize: 14
                    font.bold: true
                    Layout.topMargin: 12
                    Layout.leftMargin: 16
                    Layout.bottomMargin: 8
                }

                Repeater {
                    model: root.sectionsFor("advanced")

                    delegate: Card {
                        id: advancedCard
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        contentSpacing: 6
                        title: modelData.cardTitle

                        Repeater {
                            model: advancedCard.modelData.fields

                            delegate: FormField {
                                id: advancedField
                                required property var modelData

                                field: modelData
                                fieldEnabled: root.fieldEnabledByKey(modelData.key)
                            }
                        }
                    }
                }

                // 底部留白，防止内容靠底
                Item { Layout.preferredHeight: 16 }
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

        // 弹性空间，将导出/清空按钮统一推至右侧
        Item { Layout.fillWidth: true }

        Button {
            text: qsTr("导出配置")
            // 无当前配置时禁用
            enabled: ConfigEditorViewModel.currentInstanceName !== ""
            onClicked: exportChoiceDialog.open()
        }

        Button {
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
    ConfirmDialog {
        id: resetConfirmDialog

        headerTitle: qsTr("清空配置")
        danger: true
        confirmText: qsTr("清空")
        message: ConfigEditorViewModel.displayName !== ""
            ? qsTr("将把当前实例「%1」的全部网络设置恢复为默认值。此操作不可恢复！\n\n是否继续？")
                .arg(ConfigEditorViewModel.displayName)
            : ""

        onAccepted: ConfigEditorViewModel.resetToDefaults()
    }

    // 导出方式选择对话框（两个并列入口按钮）
    DialogWindow {
        id: exportChoiceDialog

        modality: Qt.ApplicationModal

        title: ""
        width: 380

        function open() {
            visible = true
            requestActivate()
        }

        function close() {
            visible = false
        }

        header: DialogTitleBar {
            title: qsTr("导出配置")
        }

        ColumnLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            spacing: 12

            RecommandButton {
                Layout.fillWidth: true
                Layout.topMargin: 12
                text: qsTr("导出为文件")
                onClicked: {
                    exportChoiceDialog.close()
                    exportFileDialog.open()
                }
            }

            Button {
                Layout.fillWidth: true
                text: qsTr("导出为 URL")
                onClicked: {
                    exportChoiceDialog.close()
                    var urlStr = ConfigListModel.exportConfigUrl(ConfigEditorViewModel.currentInstanceName)
                    if (urlStr !== "") {
                        exportUrlField.text = urlStr
                        exportUrlDialog.open()
                    }
                }
            }

            Item { Layout.preferredHeight: 4 }
        }
    }

    // 导出 URL 展示对话框（只读展示 + 一键复制）
    DialogWindow {
        id: exportUrlDialog

        modality: Qt.ApplicationModal

        title: ""
        width: 520

        function open() {
            visible = true
            requestActivate()
        }

        function close() {
            visible = false
        }

        header: DialogTitleBar {
            title: qsTr("导出 URL")
        }

        ColumnLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            spacing: 16

            Label {
                Layout.fillWidth: true
                Layout.topMargin: 12
                text: qsTr("复制以下 URL 即可分享配置：")
            }

            // 只读多行 URL 展示：DTK TextArea，自绘背景覆盖为表单风格边框
            TextArea {
                id: exportUrlField
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                readOnly: true
                wrapMode: TextEdit.WrapAnywhere

                background: Rectangle {
                    radius: 8
                    color: palette.base
                    border.width: 1
                    border.color: Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.15)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: 12
                spacing: 10

                Item { Layout.fillWidth: true }

                Button {
                    text: qsTr("复制")
                    onClicked: {
                        exportUrlField.selectAll()
                        exportUrlField.copy()
                    }
                }

                RecommandButton {
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
