/* @file FormServerList.qml (DDE)
 * @brief DDE 版服务器列表字段渲染器：包装 DDE ServerListItem，附“从收藏导入”入口（含去重）与导入对话框 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

/*
 * 服务器列表渲染器（数据驱动，DTK 控件版）
 * 按 key 泛化服务器加载/写回逻辑；“从收藏导入”按钮与
 * ImportNodesDialog（含 uri 去重追加）一并内聚于此。
 */
/* @brief 服务器列表渲染器根布局 */
ColumnLayout {
    id: root

    /* 字段元数据（key 固定为 servers） */
    required property var field

    width: parent ? parent.width : 0

    /* ViewModel 侧当前值（NOTIFY 驱动；数组引用比较必然不等，赋值即触发重载） */
    property var values: ConfigEditorViewModel[root.field.key]
    onValuesChanged: reload()

    /* 从 ViewModel 拉取服务器列表到本地 ListModel */
    function reload() {
        serverList.model.clear()
        for (var i = 0; i < root.values.length; i++)
            serverList.model.append({ uri: root.values[i].uri, publicKey: root.values[i].publicKey })
    }

    /* 将本地 ListModel 整体写回 ViewModel */
    function commit() {
        var servers = []
        for (var i = 0; i < serverList.model.count; i++) {
            var item = serverList.model.get(i)
            servers.push({ uri: item.uri, publicKey: item.publicKey })
        }
        ConfigEditorViewModel.setFieldValue(root.field.key, servers)
    }

    ServerListItem {
        id: serverList
        Layout.fillWidth: true
        onChanged: root.commit()
        onDuplicateDetected: function(msg) { AppState.showError(msg) }
    }

    // 从已收藏的服务器节点中导入
    Button {
        text: qsTr("从收藏导入")
        Layout.fillWidth: true
        onClicked: importNodesDialog.open()
    }

    ImportNodesDialog {
        id: importNodesDialog
        onNodesSelected: function(nodes) {
            // 导入时去重：检查 uri 是否已存在
            for (var i = 0; i < nodes.length; i++) {
                var dup = false
                for (var j = 0; j < serverList.model.count; j++) {
                    if (serverList.model.get(j).uri === nodes[i].uri) {
                        dup = true
                        break
                    }
                }
                if (!dup)
                    serverList.model.append({
                        uri: nodes[i].uri,
                        publicKey: nodes[i].publicKey || ""
                    })
            }
            root.commit()
        }
    }
}
