/* @file FormComboBox.qml (DDE)
 * @brief DDE 版下拉选择字段渲染器：选项来自元数据 options({text,value})，按 value 匹配当前索引
 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

/* @brief 下拉字段渲染器根布局 */
RowLayout {
    id: root

    /* 字段元数据（FormField 分发传入，options 为 {text, value} 列表） */
    required property var field

    width: parent ? parent.width : 0

    Label {
        text: root.field.title
        // 标签列钉死固定宽度，保证各行输入框左缘对齐
        Layout.preferredWidth: 140
        Layout.minimumWidth: 140
        Layout.maximumWidth: 140
        elide: Text.ElideRight
    }

    ComboBox {
        id: combo
        Layout.fillWidth: true
        textRole: "text"
        model: root.field.options

        // 根据当前 ViewModel 值查找对应索引，未命中回退到首项
        currentIndex: {
            var current = ConfigEditorViewModel[root.field.key]
            for (var i = 0; i < combo.model.length; i++) {
                if (combo.model[i].value === current)
                    return i
            }
            return 0
        }

        onActivated: function(index) {
            ConfigEditorViewModel.setFieldValue(root.field.key, combo.model[index].value)
        }
    }
}
