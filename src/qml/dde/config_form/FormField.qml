/* @file FormField.qml (DDE)
 * @brief DDE 版表单字段分发器：按 ViewModel 提供的字段元数据 type 动态选择 DTK 渲器组件
 *
 * 与共享版逻辑一致：field 为 ConfigEditorViewModel.formSections 中的字段描述
 *   { key, title, type, placeholder?, options?, from?, to?, addTitle?, addDefault?, dedupe? }
 * fieldEnabled 由页面壳传入，承载字段间联动禁用等 UI 逻辑；
 * 渲染器实现全部换用 org.deepin.dtk 控件（dde/config_form/ 同目录）。
 */
import QtQuick
import QtQuick.Layouts

/* @brief 分发器根 Loader，sourceComponent 按 field.type 解析 */
Loader {
    id: root

    /* 字段元数据（来自 ViewModel，CONSTANT） */
    required property var field
    /* 是否可用：默认 true，页面壳按联动规则覆盖 */
    property bool fieldEnabled: true

    /* 占满卡片内容宽度，渲染器内部以 parent.width 对齐 */
    Layout.fillWidth: true

    sourceComponent: {
        switch (root.field.type) {
        case "switch": return switchComp
        case "textField": return textFieldComp
        case "password": return passwordComp
        case "comboBox": return comboBoxComp
        case "spinBox": return spinBoxComp
        case "stringList": return stringListComp
        case "serverList": return serverListComp
        case "proxyNetworkList": return proxyNetworkListComp
        case "filePath": return filePathComp
        case "keyActions": return keyActionsComp
        }
        return null
    }

    Component { id: switchComp; FormSwitch { field: root.field; enabled: root.fieldEnabled } }
    Component { id: textFieldComp; FormTextField { field: root.field; enabled: root.fieldEnabled } }
    Component { id: passwordComp; FormPassword { field: root.field; enabled: root.fieldEnabled } }
    Component { id: comboBoxComp; FormComboBox { field: root.field; enabled: root.fieldEnabled } }
    Component { id: spinBoxComp; FormSpinBox { field: root.field; enabled: root.fieldEnabled } }
    Component { id: stringListComp; FormStringList { field: root.field } }
    Component { id: serverListComp; FormServerList { field: root.field } }
    Component { id: proxyNetworkListComp; FormProxyNetworkList { field: root.field } }
    Component { id: filePathComp; FormFilePath { field: root.field; enabled: root.fieldEnabled } }
    Component { id: keyActionsComp; FormKeyActions { field: root.field } }
}
