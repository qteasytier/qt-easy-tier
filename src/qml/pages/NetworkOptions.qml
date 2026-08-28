/* @brief 网络配置编辑器：基础/高级设置 Tab 页签，动态列表管理，导出 TOML 配置，所有字段通过 ConfigEditorViewModel 双向绑定 */
/*
 * NetworkOptions.qml - 网络配置编辑器
 *
 * 主要功能：
 * - 基础设置 Tab：主机名、网络名称/密钥、DHCP、IPv4、协议偏好等
 * - 高级设置 Tab：传输协议、P2P 连接、性能系统、网络服务、安全模式
 * - 动态列表管理（服务器、监听地址、代理子网、路由、出口节点）
 * - 导出配置文件（TOML 格式）
 *
 * 数据流：所有字段通过 ConfigEditorViewModel 双向绑定，采用即时保存：
 * 字段一旦修改即由 ConfigEditorViewModel 内部防抖自动落库，无需手动点击保存。
 * 列表数据通过 loadListsFromConfig() 从 ViewModel 拉取，
 * 通过 commitListsToViewModel() 写回 ViewModel。
 *
 * 依赖的单例：
 * - ConfigEditorViewModel  字段读写、即时保存（防抖 300ms）
 * - ConfigListModel        导出配置
 * - AppState               错误提示、主目录路径
 * - FontHelper             小字体
 *
 * 所有颜色使用系统 palette，不硬编码 hex 色值。
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtEasyTier

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

    /* 绑定到 ViewModel 的当前实例名，变化时自动重载列表数据 */
    property string currentInstance: ConfigEditorViewModel.currentInstanceName
    property bool networkSecretVisible: false
    property bool secureKeyVisible: false

    /* 从 ViewModel 拉取所有动态列表数据到本地 ListModel */
    function loadListsFromConfig() {
        serverList.model.clear()
        var servers = ConfigEditorViewModel.servers
        for (var i = 0; i < servers.length; i++)
            serverList.model.append({ uri: servers[i].uri, publicKey: servers[i].publicKey })

        listenList.model.clear()
        var addrs = ConfigEditorViewModel.listenAddresses
        for (var i = 0; i < addrs.length; i++)
            listenList.model.append({ value: addrs[i] })

        proxyList.model.clear()
        var proxies = ConfigEditorViewModel.proxyNetworks
        for (var i = 0; i < proxies.length; i++) {
            proxyList.model.append({
                cidr: proxies[i].cidr,
                mappedCidr: proxies[i].mappedCidr || "",
                allow: proxies[i].allow && proxies[i].allow.length > 0 ? proxies[i].allow : ["tcp", "udp", "icmp"]
            })
        }

        routeList.model.clear()
        var routes = ConfigEditorViewModel.customRoutes
        for (var i = 0; i < routes.length; i++)
            routeList.model.append({ value: routes[i] })

        exitNodeList.model.clear()
        var exitNodes = ConfigEditorViewModel.exitNodes
        for (var i = 0; i < exitNodes.length; i++)
            exitNodeList.model.append({ value: exitNodes[i] })
    }

    /* 将本地 ListModel 中所有动态列表数据写回 ViewModel */
    function commitListsToViewModel() {
        var servers = []
        for (var i = 0; i < serverList.model.count; i++) {
            var item = serverList.model.get(i)
            servers.push({ uri: item.uri, publicKey: item.publicKey })
        }
        ConfigEditorViewModel.servers = servers

        var addrs = []
        for (var i = 0; i < listenList.model.count; i++)
            addrs.push(listenList.model.get(i).value)
        ConfigEditorViewModel.listenAddresses = addrs

        var proxies = []
        for (var i = 0; i < proxyList.model.count; i++) {
            var proxy = proxyList.model.get(i)
            proxies.push({
                cidr: proxy.cidr,
                mappedCidr: proxy.mappedCidr || "",
                allow: proxy.allow && proxy.allow.length > 0 ? proxy.allow : ["tcp", "udp", "icmp"]
            })
        }
        ConfigEditorViewModel.proxyNetworks = proxies

        var routes = []
        for (var i = 0; i < routeList.model.count; i++)
            routes.push(routeList.model.get(i).value)
        ConfigEditorViewModel.customRoutes = routes

        var exitNodes = []
        for (var i = 0; i < exitNodeList.model.count; i++)
            exitNodes.push(exitNodeList.model.get(i).value)
        ConfigEditorViewModel.exitNodes = exitNodes
    }

    // 切换配置实例或组件完成时，重新加载列表数据
    onCurrentInstanceChanged: loadListsFromConfig()
    Component.onCompleted: loadListsFromConfig()

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
        ScrollView {
            property string tabTitle: qsTr("基础设置")
            id: basicScroll
            contentWidth: availableWidth

            ColumnLayout {
                width: basicScroll.availableWidth
                spacing: 6

                // 区块标题
                Label {
                    text: qsTr("基础设置")
                    font.pixelSize: 14
                    font.bold: true
                    color: palette.highlight
                    Layout.topMargin: 12
                    Layout.leftMargin: 16
                    Layout.bottomMargin: 8
                }

                // 身份与网络基本信息
                Card {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    contentSpacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("主机名")
                            Layout.preferredWidth: 110
                        }
                        TextField {
                            Layout.fillWidth: true
                            text: ConfigEditorViewModel.hostname
                            onTextEdited: ConfigEditorViewModel.hostname = text
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("网络名称")
                            Layout.preferredWidth: 110
                        }
                        TextField {
                            Layout.fillWidth: true
                            text: ConfigEditorViewModel.networkName
                            onTextEdited: ConfigEditorViewModel.networkName = text
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("网络密钥")
                            Layout.preferredWidth: 110
                        }
                        Item {
                            Layout.fillWidth: true
                            implicitHeight: secretField.implicitHeight

                            TextField {
                                id: secretField
                                anchors.fill: parent
                                rightPadding: secretToggle.implicitWidth + 8
                                text: ConfigEditorViewModel.networkSecret
                                onTextEdited: ConfigEditorViewModel.networkSecret = text
                                echoMode: root.networkSecretVisible ? TextInput.Normal : TextInput.Password
                            }

                            IconToolButton {
                                id: secretToggle
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                iconSource: root.networkSecretVisible ? "qrc:/icons/eye-slash.svg" : "qrc:/icons/eye.svg"
                                onClicked: root.networkSecretVisible = !root.networkSecretVisible
                            }
                        }
                    }

                    Switch {
                        text: qsTr("DHCP")
                        checked: ConfigEditorViewModel.dhcp
                        onToggled: ConfigEditorViewModel.dhcp = checked
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("虚拟 IPv4")
                            Layout.preferredWidth: 110
                        }
                        TextField {
                            Layout.fillWidth: true
                            enabled: !ConfigEditorViewModel.dhcp
                            text: ConfigEditorViewModel.ipv4
                            onTextEdited: ConfigEditorViewModel.ipv4 = text
                        }
                    }

                    Switch {
                        text: qsTr("低延迟优先")
                        checked: ConfigEditorViewModel.latencyFirst
                        onToggled: ConfigEditorViewModel.latencyFirst = checked
                    }

                    Switch {
                        text: qsTr("私有模式")
                        checked: ConfigEditorViewModel.privateMode
                        onToggled: ConfigEditorViewModel.privateMode = checked
                    }
                }

                // 初始节点（服务器）列表（可增删的 URI + publicKey 列表）
                Card {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    contentSpacing: 6

                    Label {
                        text: qsTr("初始节点（服务器）")
                        font.bold: true
                    }

                    ServerListItem {
                        id: serverList
                        Layout.fillWidth: true
                        onChanged: commitListsToViewModel()
                        onDuplicateDetected: function(msg) { AppState.showError(msg) }
                    }

                    // 从已收藏的服务器节点中导入
                    Button {
                        text: qsTr("从收藏导入")
                        flat: true
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
                            commitListsToViewModel()
                        }
                    }
                }

                // 底部占位：保证内容不满时卡片不拉伸
                Item { Layout.fillHeight: true }
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
        ScrollView {
            property string tabTitle: qsTr("高级设置")
            id: advancedScroll
            contentWidth: availableWidth

            ColumnLayout {
                width: advancedScroll.availableWidth
                spacing: 6

                Label {
                    text: qsTr("高级设置")
                    font.pixelSize: 14
                    font.bold: true
                    color: palette.highlight
                    Layout.topMargin: 12
                    Layout.leftMargin: 16
                    Layout.bottomMargin: 8
                }

                // 传输协议组：KCP / QUIC / 加密 相关开关
                Card {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    contentSpacing: 6

                    Label {
                        text: qsTr("传输协议")
                        font.bold: true
                        color: palette.highlight
                        Layout.bottomMargin: 2
                    }

                    Switch { text: qsTr("启用 KCP 代理"); checked: ConfigEditorViewModel.enableKcpProxy; onToggled: ConfigEditorViewModel.enableKcpProxy = checked }
                    Switch { text: qsTr("禁用 KCP 输入"); checked: ConfigEditorViewModel.disableKcpInput; onToggled: ConfigEditorViewModel.disableKcpInput = checked }
                    Switch { text: qsTr("启用 QUIC 代理"); checked: ConfigEditorViewModel.enableQuicProxy; onToggled: ConfigEditorViewModel.enableQuicProxy = checked }
                    Switch { text: qsTr("禁用 QUIC 输入"); checked: ConfigEditorViewModel.disableQuicInput; onToggled: ConfigEditorViewModel.disableQuicInput = checked }
                    Switch { text: qsTr("禁止转发 KCP"); checked: ConfigEditorViewModel.disableRelayKcp; onToggled: ConfigEditorViewModel.disableRelayKcp = checked }
                    Switch { text: qsTr("禁止转发 QUIC"); checked: ConfigEditorViewModel.disableRelayQuic; onToggled: ConfigEditorViewModel.disableRelayQuic = checked }
                    Switch { text: qsTr("允许转发其他网络 KCP"); checked: ConfigEditorViewModel.enableRelayForeignNetworkKcp; onToggled: ConfigEditorViewModel.enableRelayForeignNetworkKcp = checked }
                    Switch { text: qsTr("允许转发其他网络 QUIC"); checked: ConfigEditorViewModel.enableRelayForeignNetworkQuic; onToggled: ConfigEditorViewModel.enableRelayForeignNetworkQuic = checked }
                    Switch { text: qsTr("启用加密"); checked: ConfigEditorViewModel.enableEncryption; onToggled: ConfigEditorViewModel.enableEncryption = checked }

                    // 加密算法下拉框
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("加密算法"); Layout.preferredWidth: 110 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["aes-gcm", "xor", "chacha20", "aes-gcm256"]
                            // 根据当前 ViewModel 值查找对应索引
                            currentIndex: {
                                var algo = ConfigEditorViewModel.encryptionAlgorithm
                                for (var i = 0; i < model.length; i++) {
                                    if (model[i] === algo) return i
                                }
                                return 0
                            }
                            onActivated: function(index) { ConfigEditorViewModel.encryptionAlgorithm = model[index] }
                        }
                    }

                    // 默认连接协议下拉框
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("默认连接协议"); Layout.preferredWidth: 110 }
                        ComboBox {
                            Layout.fillWidth: true
                            textRole: "text"
                            model: [
                                { text: qsTr("不指定"), value: "" },
                                { text: "udp", value: "udp" },
                                { text: "tcp", value: "tcp" },
                                { text: "wg",  value: "wg" },
                                { text: "ws",  value: "ws" },
                                { text: "wss", value: "wss" }
                            ]
                            currentIndex: {
                                var proto = ConfigEditorViewModel.defaultProtocol
                                for (var i = 0; i < model.length; i++) {
                                    if (model[i].value === proto) return i
                                }
                                return 0
                            }
                            onActivated: function(index) { ConfigEditorViewModel.defaultProtocol = model[index].value }
                        }
                    }
                }

                // P2P 连接组
                Card {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    contentSpacing: 6

                    Label {
                        text: qsTr("P2P 连接")
                        font.bold: true
                        color: palette.highlight
                        Layout.bottomMargin: 2
                    }

                    Switch { text: qsTr("仅 P2P"); checked: ConfigEditorViewModel.p2pOnly; onToggled: ConfigEditorViewModel.p2pOnly = checked }
                    Switch { text: qsTr("禁用 P2P"); checked: ConfigEditorViewModel.disableP2p; onToggled: ConfigEditorViewModel.disableP2p = checked }
                    Switch { text: qsTr("需要 P2P"); checked: ConfigEditorViewModel.needP2p; onToggled: ConfigEditorViewModel.needP2p = checked }
                    Switch { text: qsTr("按需 P2P"); checked: ConfigEditorViewModel.lazyP2p; onToggled: ConfigEditorViewModel.lazyP2p = checked }
                    Switch { text: qsTr("禁用 UDP 打洞"); checked: ConfigEditorViewModel.disableUdpHolePunching; onToggled: ConfigEditorViewModel.disableUdpHolePunching = checked }
                    Switch { text: qsTr("禁用 TCP 打洞"); checked: ConfigEditorViewModel.disableTcpHolePunching; onToggled: ConfigEditorViewModel.disableTcpHolePunching = checked }
                    Switch { text: qsTr("禁用 UPnP"); checked: ConfigEditorViewModel.disableUpnp; onToggled: ConfigEditorViewModel.disableUpnp = checked }
                    Switch { text: qsTr("禁用对称 NAT 打洞"); checked: ConfigEditorViewModel.disableSymHolePunching; onToggled: ConfigEditorViewModel.disableSymHolePunching = checked }
                    Switch { text: qsTr("转发 RPC 包"); checked: ConfigEditorViewModel.relayAllPeerRpc; onToggled: ConfigEditorViewModel.relayAllPeerRpc = checked }
                    Switch { text: qsTr("仅使用物理网卡"); checked: ConfigEditorViewModel.bindDevice; onToggled: ConfigEditorViewModel.bindDevice = checked }
                }

                // 性能与系统组
                Card {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    contentSpacing: 6

                    Label {
                        text: qsTr("性能与系统")
                        font.bold: true
                        color: palette.highlight
                        Layout.bottomMargin: 2
                    }

                    Switch { text: qsTr("启用多线程"); checked: ConfigEditorViewModel.multiThread; onToggled: ConfigEditorViewModel.multiThread = checked }
                    Switch { text: qsTr("使用用户态协议栈"); checked: ConfigEditorViewModel.useSmoltcp; onToggled: ConfigEditorViewModel.useSmoltcp = checked }
                    Switch { text: qsTr("不创建 TUN"); checked: ConfigEditorViewModel.noTun; onToggled: ConfigEditorViewModel.noTun = checked }
                    Switch { text: qsTr("启用 IPv6"); checked: ConfigEditorViewModel.enableIpv6; onToggled: ConfigEditorViewModel.enableIpv6 = checked }

                    // MTU 数值输入
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("MTU"); Layout.preferredWidth: 110 }
                        SpinBox {
                            Layout.fillWidth: true
                            value: ConfigEditorViewModel.mtu
                            from: 576; to: 1500
                            editable: true
                            onValueModified: ConfigEditorViewModel.mtu = value
                        }
                    }

                    // TUN 设备名称
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("TUN 设备名"); Layout.preferredWidth: 110 }
                        TextField {
                            Layout.fillWidth: true
                            text: ConfigEditorViewModel.devName
                            onTextEdited: ConfigEditorViewModel.devName = text
                        }
                    }
                }

                // 网络服务组：出口节点、转发、DNS、监听、代理、路由
                Card {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    contentSpacing: 6

                    Label {
                        text: qsTr("网络服务")
                        font.bold: true
                        color: palette.highlight
                        Layout.bottomMargin: 2
                    }

                    Switch { text: qsTr("启用出口节点"); checked: ConfigEditorViewModel.enableExitNode; onToggled: ConfigEditorViewModel.enableExitNode = checked }
                    Switch { text: qsTr("系统转发（子网代理禁用内置 NAT）"); checked: ConfigEditorViewModel.systemForwarding; onToggled: ConfigEditorViewModel.systemForwarding = checked }
                    Switch { text: qsTr("启用魔法 DNS"); checked: ConfigEditorViewModel.acceptDns; onToggled: ConfigEditorViewModel.acceptDns = checked }

                    // 白名单开关 + 输入框联动
                    Switch {
                        text: qsTr("启用网络白名单")
                        checked: ConfigEditorViewModel.enableForeignNetworkWhitelist
                        onToggled: ConfigEditorViewModel.enableForeignNetworkWhitelist = checked
                    }
                    TextField {
                        Layout.fillWidth: true
                        // 仅在白名单开启时可编辑
                        enabled: ConfigEditorViewModel.enableForeignNetworkWhitelist
                        text: ConfigEditorViewModel.foreignNetworkWhitelist
                        placeholderText: qsTr("多个网络用空格分开")
                        onTextEdited: ConfigEditorViewModel.foreignNetworkWhitelist = text
                    }

                    // 以下为可增删的动态列表，均通过 EditableList 组件渲染
                    Label { text: qsTr("监听地址"); font.bold: true; Layout.topMargin: 4 }
                    EditableList {
                        id: listenList
                        Layout.fillWidth: true
                        addDialogTitle: qsTr("添加监听地址")
                        defaultAddValue: "tcp://0.0.0.0:11010"
                        checkDuplicates: true
                        onChanged: commitListsToViewModel()
                        onDuplicateDetected: function(msg) { AppState.showError(msg) }
                    }

                    Label { text: qsTr("代理子网"); font.bold: true; Layout.topMargin: 4 }
                    ProxyNetworkListItem {
                        id: proxyList
                        Layout.fillWidth: true
                        onChanged: commitListsToViewModel()
                        onDuplicateDetected: function(msg) { AppState.showError(msg) }
                    }

                    Label { text: qsTr("自定义路由"); font.bold: true; Layout.topMargin: 4 }
                    EditableList {
                        id: routeList
                        Layout.fillWidth: true
                        addDialogTitle: qsTr("添加自定义路由规则")
                        defaultAddValue: "0.0.0.0/24"
                        checkDuplicates: true
                        onChanged: commitListsToViewModel()
                        onDuplicateDetected: function(msg) { AppState.showError(msg) }
                    }

                    Label { text: qsTr("出口节点列表"); font.bold: true; Layout.topMargin: 4 }
                    EditableList {
                        id: exitNodeList
                        Layout.fillWidth: true
                        addDialogTitle: qsTr("添加出口节点地址")
                        defaultAddValue: "10.126.126.1"
                        checkDuplicates: true
                        onChanged: commitListsToViewModel()
                        onDuplicateDetected: function(msg) { AppState.showError(msg) }
                    }
                }

                // 安全模式组
                Card {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    contentSpacing: 6

                    Label {
                        text: qsTr("安全模式")
                        font.bold: true
                        color: palette.highlight
                        Layout.bottomMargin: 2
                    }

                    Switch {
                        text: qsTr("启用安全模式")
                        checked: ConfigEditorViewModel.secureModeEnabled
                        onToggled: ConfigEditorViewModel.secureModeEnabled = checked
                    }

                    // 节点私钥（安全模式相关），采用密码输入形式，可点击眼睛图标切换明文
                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("节点私钥Base64")
                            Layout.preferredWidth: 110
                        }
                        Item {
                            Layout.fillWidth: true
                            implicitHeight: secureKeyField.implicitHeight

                            TextField {
                                id: secureKeyField
                                anchors.fill: parent
                                rightPadding: secureKeyToggle.implicitWidth + 8
                                text: ConfigEditorViewModel.localPrivateKey
                                placeholderText: qsTr("可选，留空使用随机密钥")
                                onTextEdited: ConfigEditorViewModel.localPrivateKey = text
                                echoMode: root.secureKeyVisible ? TextInput.Normal : TextInput.Password
                            }

                            IconToolButton {
                                id: secureKeyToggle
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                iconSource: root.secureKeyVisible ? "qrc:/icons/eye-slash.svg" : "qrc:/icons/eye.svg"
                                onClicked: root.secureKeyVisible = !root.secureKeyVisible
                            }
                        }
                    }

                    // 私钥操作按钮：随机生成密钥对 / 实时计算并展示公钥
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 4

                        Button {
                            text: qsTr("随机生成")
                            onClicked: {
                                if (!ConfigEditorViewModel.generateRandomPrivateKey())
                                    AppState.showError(qsTr("随机私钥生成失败"))
                            }
                        }

                        Button {
                            text: qsTr("显示公钥")
                            onClicked: {
                                var pub = ConfigEditorViewModel.derivePublicKey()
                                if (pub === "") {
                                    AppState.showError(qsTr("私钥为空或无效，请先输入或随机生成私钥"))
                                    return
                                }
                                publicKeyField.text = pub
                                showPublicKeyDialog.open()
                            }
                        }
                    }

                    // 临时密钥对文件（.json，仅记录路径交给 daemon 的 credential_file）
                    Label {
                        text: qsTr("临时密钥文件(.json)")
                        Layout.topMargin: 4
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        TextField {
                            Layout.fillWidth: true
                            text: ConfigEditorViewModel.credentialFile
                            placeholderText: qsTr("例如 /path/to/credential.json")
                            onTextEdited: ConfigEditorViewModel.credentialFile = text
                        }

                        IconToolButton {
                            iconSource: "qrc:/icons/edit.svg"
                            onClicked: credentialFileDialog.open()
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

        Button {
            text: qsTr("导出配置")
            // 无当前配置时禁用
            enabled: ConfigEditorViewModel.currentInstanceName !== ""
            onClicked: exportChoiceDialog.open()
        }

        // 弹性空间，将导出按钮推至左侧、清空按钮推至右侧
        Item { Layout.fillWidth: true }

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
    Dialog {
        id: resetConfirmDialog
        title: qsTr("清空配置")
        standardButtons: Dialog.Yes | Dialog.No
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(360, parent ? parent.width - 48 : 320)

        Label {
            text: qsTr("将把当前实例「%1」的全部网络设置恢复为默认值。此操作不可恢复！\n\n是否继续？")
                .arg(ConfigEditorViewModel.displayName)
            wrapMode: Text.WordWrap
            width: parent ? parent.width : 320
        }

        onAccepted: ConfigEditorViewModel.resetToDefaults()
    }

    // 导出方式选择对话框
    Dialog {
        id: exportChoiceDialog
        title: qsTr("导出配置")
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        standardButtons: Dialog.Cancel
        RowLayout {
            spacing: 12
            Button {
                text: qsTr("导出为文件")
                Layout.fillWidth: true
                onClicked: {
                    exportChoiceDialog.close()
                    exportFileDialog.open()
                }
            }
            Button {
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
    Dialog {
        id: exportUrlDialog
        title: qsTr("导出 URL")
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(520, parent ? parent.width - 48 : 480)
        ColumnLayout {
            anchors.fill: parent
            spacing: 8
            Label { text: qsTr("复制以下 URL 即可分享配置：") }
            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 100

                TextArea {
                    id: exportUrlField
                    readOnly: true
                    wrapMode: TextEdit.WrapAnywhere
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                Button {
                    text: qsTr("复制")
                    onClicked: {
                        exportUrlField.selectAll()
                        exportUrlField.copy()
                    }
                }
                Button {
                    text: qsTr("确定")
                    onClicked: exportUrlDialog.close()
                }
            }
        }
    }

    // 显示公钥对话框：由安全模式私钥实时计算（公钥不落库），支持一键复制
    Dialog {
        id: showPublicKeyDialog
        title: qsTr("本地公钥")
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(520, parent ? parent.width - 48 : 480)
        ColumnLayout {
            anchors.fill: parent
            spacing: 8
            Label { text: qsTr("复制以下公钥即可分享给其他节点：") }
            TextField {
                id: publicKeyField
                readOnly: true
                Layout.fillWidth: true
            }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                Button {
                    text: qsTr("复制")
                    onClicked: {
                        publicKeyField.selectAll()
                        publicKeyField.copy()
                    }
                }
                Button {
                    text: qsTr("确定")
                    onClicked: showPublicKeyDialog.close()
                }
            }
        }
    }

    // 选择临时密钥文件（FileDialog 选择到的必然是文件）
    FileDialog {
        id: credentialFileDialog
        title: qsTr("选择临时密钥文件")
        nameFilters: [qsTr("JSON 文件 (*.json)"), qsTr("所有文件 (*)")]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            var p = ConfigEditorViewModel.toLocalFilePath(selectedFile.toString())
            if (p !== "")
                ConfigEditorViewModel.credentialFile = p
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
