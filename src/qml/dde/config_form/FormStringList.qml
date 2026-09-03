/* @file FormStringList.qml (DDE)
 * @brief DDE 版字符串列表字段渲染器：包装 EditableList，按字段 key 泛化加载/写回
 *
 * ViewModel 值变化（实例切换/加载）时重建本地列表；写回相同内容时
 * ViewModel 不发信号，无自反循环。
 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

ColumnLayout {
    id: root

    /* 字段元数据（addTitle/addDefault/dedupe 为添加对话框配置） */
    required property var field

    width: parent ? parent.width : 0

    /* ViewModel 侧当前值（NOTIFY 驱动；数组引用比较必然不等，赋值即触发重载） */
    property var values: ConfigEditorViewModel[root.field.key]
    onValuesChanged: reload()

    /* 从 ViewModel 拉取列表数据到本地 ListModel */
    function reload() {
        editList.model.clear()
        for (var i = 0; i < root.values.length; i++)
            editList.model.append({ value: root.values[i] })
    }

    /* 将本地 ListModel 整体写回 ViewModel */
    function commit() {
        var arr = []
        for (var i = 0; i < editList.model.count; i++)
            arr.push(editList.model.get(i).value)
        ConfigEditorViewModel.setFieldValue(root.field.key, arr)
    }

    Label {
        text: root.field.title
        font.bold: true
        visible: root.field.title !== ""
        Layout.topMargin: 4
    }

    EditableList {
        id: editList
        Layout.fillWidth: true
        addDialogTitle: root.field.addTitle ?? qsTr("添加项")
        defaultAddValue: root.field.addDefault ?? ""
        checkDuplicates: root.field.dedupe ?? false
        onChanged: root.commit()
        onDuplicateDetected: function(msg) { AppState.showError(msg) }
    }
}
