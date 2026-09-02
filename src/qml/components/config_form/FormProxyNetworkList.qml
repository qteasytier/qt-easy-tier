/* @brief 代理子网列表字段渲染器：包装 ProxyNetworkListItem，allow 协议缺省时回退为 tcp/udp/icmp */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import SwbControls

/*
 * 代理子网列表渲染器（数据驱动改造）
 * 按 key 泛化原页面的 proxyNetworks 加载/写回逻辑；
 * allow 为空时以 ["tcp","udp","icmp"] 兜底（与 ViewModel 侧归一化保持一致）。
 */
/* @brief 代理子网列表渲染器根布局 */
ColumnLayout {
    id: root

    /* 字段元数据（key 固定为 proxyNetworks） */
    required property var field

    width: parent ? parent.width : 0

    /* allow 协议缺省值 */
    readonly property var defaultAllow: ["tcp", "udp", "icmp"]

    /* ViewModel 侧当前值（NOTIFY 驱动；数组引用比较必然不等，赋值即触发重载） */
    property var values: ConfigEditorViewModel[root.field.key]
    onValuesChanged: reload()

    /* 从 ViewModel 拉取代理子网列表到本地 ListModel */
    function reload() {
        proxyList.model.clear()
        for (var i = 0; i < root.values.length; i++) {
            var src = root.values[i]
            proxyList.model.append({
                cidr: src.cidr,
                mappedCidr: src.mappedCidr || "",
                allow: src.allow && src.allow.length > 0 ? src.allow : root.defaultAllow
            })
        }
    }

    /* 将本地 ListModel 整体写回 ViewModel */
    function commit() {
        var proxies = []
        for (var i = 0; i < proxyList.model.count; i++) {
            var proxy = proxyList.model.get(i)
            proxies.push({
                cidr: proxy.cidr,
                mappedCidr: proxy.mappedCidr || "",
                allow: proxy.allow && proxy.allow.length > 0 ? proxy.allow : root.defaultAllow
            })
        }
        ConfigEditorViewModel.setFieldValue(root.field.key, proxies)
    }

    SwbLabel {
        text: root.field.title
        font.bold: true
        visible: root.field.title !== ""
        Layout.topMargin: 4
    }

    ProxyNetworkListItem {
        id: proxyList
        Layout.fillWidth: true
        onChanged: root.commit()
        onDuplicateDetected: function(msg) { AppState.showError(msg) }
    }
}
