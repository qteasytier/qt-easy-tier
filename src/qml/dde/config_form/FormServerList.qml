/* @file FormServerList.qml (DDE)
 * @brief DDE 版服务器列表字段渲染器：包装 ServerListItem，附"从收藏导入"入口（uri 去重）
 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

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
    LinkButton {
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
