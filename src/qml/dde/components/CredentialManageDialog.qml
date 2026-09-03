/* @file CredentialManageDialog.qml (DDE)
 * @brief DDE 版管理临时节点密钥对话框：DTK DialogWindow 重写，对齐共享版新交互
 *        （查询/新增/编辑/撤销当前实例已签发的安全模式临时凭证，新增与编辑共用一套表单）
 *
 * 相对旧 DDE 版：独立的 GenerateCredentialDialog 已删除，签发流程并入本对话框
 * 的“新增”表单（submitEdit 统一走 generate/upsert）；嵌套的撤销确认对话框
 * 以命名属性挂载（DialogWindow 不能作另一 DialogWindow 的 content 子项）。
 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

/* @brief 临时凭证管理对话框，包含凭证列表、新增/编辑表单与结果/错误视图 */
DialogWindow {
    id: root

    modality: Qt.ApplicationModal

    title: ""
    width: 560

    /* 视图状态："list" 凭证列表 / "fetching" 获取原密钥中 / "edit" 新增或编辑表单 / "success" 操作成功 / "error" 操作失败 */
    property string viewState: "list"
    /* 表单是否处于新增模式（true=签发新凭证：ID 可填可空、不展示原密钥；false=编辑既有凭证） */
    property bool creating: false
    /* 列表查询中 */
    readonly property bool listing: CredentialViewModel.listing
    /* 变更操作中 */
    readonly property bool mutating: CredentialViewModel.mutating
    /* 编辑取钥进行中（自动获取原密钥，取到后才进入编辑表单） */
    readonly property bool fetchingSecret: CredentialViewModel.fetchingSecret
    /* 密钥输入框是否明文显示（默认隐藏，可点击眼睛切换） */
    property bool secretVisible: false

    /* 编辑表单预填数据（点击列表项"编辑"时暂存） */
    property string editCredentialId: ""
    property double editExpiryUnix: 0
    property string editGroupsText: ""
    property bool editAllowRelay: false
    property string editCidrsText: ""
    property bool editReusable: true

    /* 结果/错误视图文案 */
    property string resultTitle: ""
    property string resultText: ""
    /* 新增成功结果：密钥仅本次展示（关闭后无法再次查看） */
    property string resultCredentialId: ""
    property string resultSecret: ""
    property string resultExpiryText: ""

    /* 待撤销的凭证 ID */
    property string revokeId: ""

    /* 窗口关闭（含标题栏 X）时发出，兼容页面懒加载卸载逻辑 */
    signal closed()

    /* 真正打开过标记：仅在 open→close 结束才发 closed */
    property bool hadShown: false

    /* 撤销确认对话框（嵌套 DialogWindow 须以命名属性挂载，不能作 content 子项） */
    property ConfirmDialog revokeConfirmDialog: ConfirmDialog {
        headerTitle: qsTr("撤销凭证")
        danger: true
        confirmText: qsTr("撤销")
        message: root.revokeId !== ""
            ? qsTr("确定撤销凭证「%1」吗？撤销后使用该凭证的节点将无法加入网络。").arg(root.revokeId)
            : ""

        onAccepted: {
            CredentialViewModel.revokeCredential(
                VpnRuntimeService.activeInstanceName, root.revokeId)
        }
    }

    /* 每次打开重置为列表视图并刷新 */
    function open() {
        viewState = "list"
        creating = false
        resultTitle = ""
        resultText = ""
        resultCredentialId = ""
        resultSecret = ""
        resultExpiryText = ""
        hadShown = true
        visible = true
        requestActivate()
        refreshList()
    }

    function close() {
        visible = false
    }

    onVisibleChanged: {
        if (!visible && hadShown) {
            hadShown = false
            closed()
        }
    }

    /* 刷新当前实例的凭证列表 */
    function refreshList() {
        CredentialViewModel.listCredentials(VpnRuntimeService.activeInstanceName)
    }

    /* 将 Unix 秒级时间戳（过期时刻）格式化为固定格式的本地时间 */
    function formatExpiry(expiryUnix) {
        if (!expiryUnix || expiryUnix <= 0)
            return qsTr("未知")
        var local = Qt.formatDateTime(new Date(expiryUnix * 1000), "yyyy-MM-dd hh:mm:ss")
        return qsTr("%1（本地时间）").arg(local)
    }

    /* 拼接凭证授权约束摘要文本 */
    function formatConstraints(groups, allowRelay, reusable) {
        var parts = []
        if (groups && groups.length > 0)
            parts.push(qsTr("组：%1").arg(groups.join("、")))
        if (allowRelay)
            parts.push(qsTr("允许中继"))
        if (reusable)
            parts.push(qsTr("可复用"))
        return parts.join("  |  ")
    }

    /* 进入新增模式：表单重置为签发默认值，直接展示表单（无需取原密钥） */
    function beginCreate() {
        creating = true
        editCredentialId = ""
        editExpiryUnix = 0
        editGroupsText = ""
        editAllowRelay = false
        editCidrsText = ""
        editReusable = true
        editIdField.text = ""
        editTtlSpin.value = 3600
        editGroupsField.text = ""
        editCidrsField.text = ""
        editAllowRelayCheck.checked = false
        editReusableCheck.checked = true
        viewState = "edit"
    }

    /* 点击列表项"编辑"：暂存该凭证数据，先自动获取原密钥，取到后才进入编辑表单 */
    function beginEdit(credentialId, expiryUnix, groups, allowRelay, cidrs, reusable) {
        creating = false
        editCredentialId = credentialId || ""
        editExpiryUnix = expiryUnix || 0
        editGroupsText = (groups || []).join(", ")
        editAllowRelay = allowRelay === true
        editCidrsText = (cidrs || []).join(", ")
        editReusable = reusable !== false
        editIdField.text = editCredentialId
        secretField.text = ""
        editGroupsField.text = editGroupsText
        editCidrsField.text = editCidrsText
        editAllowRelayCheck.checked = editAllowRelay
        editReusableCheck.checked = editReusable
        viewState = "fetching"
        CredentialViewModel.prepareEdit(VpnRuntimeService.activeInstanceName, editCredentialId)
    }

    /* 点击列表项"撤销"：弹出确认对话框 */
    function beginRevoke(credentialId) {
        revokeId = credentialId || ""
        revokeConfirmDialog.open()
    }

    /* 提交表单：新增走 generateCredential（服务端生成新密钥），编辑走 upsertCredential（expiry = 当前时刻 + 有效期） */
    function submitEdit() {
        if (creating) {
            CredentialViewModel.generateCredential(
                VpnRuntimeService.activeInstanceName,
                editTtlSpin.value,
                editGroupsField.text,
                editAllowRelayCheck.checked,
                editCidrsField.text,
                editIdField.text,
                editReusableCheck.checked)
        } else {
            CredentialViewModel.upsertCredential(
                VpnRuntimeService.activeInstanceName,
                editCredentialId,
                secretField.text,
                editGroupsField.text,
                editAllowRelayCheck.checked,
                editCidrsField.text,
                Math.floor(Date.now() / 1000) + editTtlSpin.value,
                editReusableCheck.checked)
        }
    }

    header: DialogTitleBar {
        title: qsTr("管理临时节点密钥")
    }

    // DialogWindow 高度由内容自动决定，内容以 anchors 铺满并控制左右边距
    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        spacing: 12

        // 状态色（Theme 为 QtObject，须置于内容 Item 内而非 DialogWindow 顶层）
        Theme { id: appTheme }

        // 监听服务结果：刷新列表、切换结果/错误视图（Connections 为非 Item，
        // 须置于内容 Item 内而非 DialogWindow 顶层）
        Connections {
            target: CredentialViewModel

            // 取到原密钥后才进入编辑表单
            function onEditSecretReadyChanged() {
                if (root.viewState === "fetching" && CredentialViewModel.editSecretReady)
                    root.viewState = "edit"
            }

            // 签发成功 → 进入密钥结果视图（密钥仅本次展示）并后台刷新列表。
            // 注意：编辑前的 prepareEdit 取原密钥复用 generate 流程、同样发本信号，
            // fetching 状态下的成功属于编辑取钥，须忽略（等 editSecretReady 进入编辑表单）
            function onGenerateSucceeded(credentialId, credentialSecret, expiryUnix) {
                if (root.viewState === "fetching")
                    return
                if (root.viewState !== "edit" || !root.creating)
                    return
                root.resultCredentialId = credentialId || ""
                root.resultSecret = credentialSecret || ""
                root.resultExpiryText = root.formatExpiry(expiryUnix)
                root.viewState = "success"
                root.refreshList()
            }

            // 取钥失败（仅 admin 节点可自动获取）→ 返回列表并提示；新增提交失败 → 错误视图
            function onGenerateFailed(message) {
                if (root.viewState === "fetching") {
                    root.resultTitle = qsTr("获取原密钥失败")
                    root.resultText = message || qsTr("未知错误")
                    root.viewState = "error"
                } else if (root.viewState === "edit" && root.creating) {
                    root.resultTitle = qsTr("生成失败")
                    root.resultText = message || qsTr("未知错误")
                    root.viewState = "error"
                }
            }

            function onListFailed(message) {
                if (root.viewState !== "list")
                    return
                root.resultTitle = qsTr("查询失败")
                root.resultText = message || qsTr("未知错误")
                root.viewState = "error"
            }

            function onUpsertSucceeded(changed) {
                root.resultSecret = ""
                root.resultTitle = qsTr("更新成功")
                root.resultText = changed ? qsTr("凭证「%1」已更新。").arg(root.editCredentialId)
                                          : qsTr("凭证「%1」内容无变化。").arg(root.editCredentialId)
                root.viewState = "success"
            }

            function onUpsertFailed(message) {
                root.resultTitle = qsTr("更新失败")
                root.resultText = message || qsTr("未知错误")
                root.viewState = "error"
            }

            function onRevokedSucceeded(success) {
                root.resultSecret = ""
                root.resultTitle = success ? qsTr("撤销成功") : qsTr("凭证不存在")
                root.resultText = success
                    ? qsTr("凭证「%1」已撤销。").arg(root.revokeId)
                    : qsTr("凭证「%1」不存在，可能已被撤销。").arg(root.revokeId)
                root.viewState = "success"
            }

            function onRevokedFailed(message) {
                root.resultTitle = qsTr("撤销失败")
                root.resultText = message || qsTr("未知错误")
                root.viewState = "error"
            }
        }

        // ========== 视图 1：凭证列表 ==========
        ColumnLayout {
            visible: root.viewState === "list"
            Layout.fillWidth: true
            spacing: 8

            // 目标实例提示
            Label {
                Layout.fillWidth: true
                text: qsTr("当前实例「%1」已签发的临时凭证。").arg(VpnRuntimeService.activeInstanceName)
                font: FontHelper.smallFont
                color: palette.placeholderText
                wrapMode: Text.WordWrap
                visible: VpnRuntimeService.activeInstanceName !== ""
            }

            // 加载中提示
            Label {
                Layout.fillWidth: true
                visible: root.listing
                text: qsTr("加载中…")
                font: FontHelper.smallFont
                color: palette.placeholderText
            }

            // 空状态
            Label {
                visible: !root.listing && CredentialViewModel.credentialListModel.count === 0
                text: qsTr("暂无已签发的临时凭证")
                font.pixelSize: 18
                color: palette.placeholderText
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            // 凭证列表
            ListView {
                id: credentialListView
                visible: !root.listing && CredentialViewModel.credentialListModel.count > 0
                Layout.fillWidth: true
                Layout.preferredHeight: 260
                clip: true
                model: CredentialViewModel.credentialListModel
                spacing: 6

                ScrollBar.vertical: ScrollBar {}

                // 每个凭证用 Card 组件展示（同目录，隐式解析）
                delegate: Card {
                    width: credentialListView.width
                    contentSpacing: 4

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        // 第一行：凭证 ID + 过期时间
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Label {
                                text: credentialId || ""
                                font.bold: true
                                font.family: "monospace"
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }

                            Label {
                                text: qsTr("过期：%1").arg(root.formatExpiry(expiryUnix))
                                font: FontHelper.smallFont
                                color: palette.placeholderText
                            }
                        }

                        // 第二行：公钥指纹
                        Label {
                            visible: (publicKeyFingerprint || "") !== ""
                            text: qsTr("指纹：%1").arg(publicKeyFingerprint || "")
                            font.family: "monospace"
                            font.pixelSize: 11
                            color: palette.placeholderText
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }

                        // 第三行：授权约束摘要 + 操作按钮
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Label {
                                text: root.formatConstraints(groups, allowRelay, reusable)
                                font: FontHelper.smallFont
                                color: palette.placeholderText
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Button {
                                text: qsTr("编辑")
                                enabled: !root.mutating
                                onClicked: root.beginEdit(credentialId, expiryUnix,
                                                          groups, allowRelay,
                                                          allowedProxyCidrs, reusable)
                            }
                            WarningButton {
                                text: qsTr("撤销")
                                enabled: !root.mutating
                                onClicked: root.beginRevoke(credentialId)
                            }
                        }
                    }
                }
            }

            // 底部按钮：新增 / 刷新 / 关闭
            RowLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: 12
                spacing: 10

                RecommandButton {
                    text: qsTr("新增临时节点密钥")
                    enabled: !root.listing && !root.mutating
                    onClicked: root.beginCreate()
                }
                Button {
                    text: qsTr("刷新")
                    enabled: !root.listing && !root.mutating
                    onClicked: root.refreshList()
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: qsTr("关闭")
                    enabled: !root.mutating
                    onClicked: root.close()
                }
            }
        }

        // ========== 视图：获取原密钥（取到后才进入编辑表单） ==========
        ColumnLayout {
            visible: root.viewState === "fetching"
            Layout.fillWidth: true
            Layout.bottomMargin: 12
            spacing: 10

            Label {
                Layout.fillWidth: true
                Layout.preferredHeight: 160
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: qsTr("正在获取原密钥…")
                color: palette.placeholderText
            }
        }

        // ========== 视图 2：新增/编辑表单（共用一套控件，creating 区分模式） ==========
        ColumnLayout {
            visible: root.viewState === "edit"
            Layout.fillWidth: true
            Layout.bottomMargin: 12
            spacing: 10

            Label {
                Layout.fillWidth: true
                text: root.creating ? qsTr("新增临时节点密钥") : qsTr("编辑凭证")
                font.bold: true
            }

            // 凭证 ID：新增模式可编辑（留空由服务端自动生成），编辑模式只读
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Label {
                    text: qsTr("凭证 ID")
                    // 标签列三向钉死同一宽度：保证各行输入框左缘对齐（英文标签较长）
                    Layout.preferredWidth: 140
                    Layout.minimumWidth: 140
                    Layout.maximumWidth: 140
                    elide: Text.ElideRight
                }
                TextField {
                    id: editIdField
                    readOnly: !root.creating
                    selectByMouse: true
                    placeholderText: root.creating ? qsTr("留空则由服务端自动生成") : ""
                    Layout.fillWidth: true
                }
            }

            // 密钥内容（仅编辑模式：已自动取回原密钥，留空则使用原密钥；也可手动粘贴新密钥替换）
            ColumnLayout {
                visible: !root.creating
                Layout.fillWidth: true
                spacing: 4

                Label {
                    text: qsTr("密钥（credential_secret）")
                    color: palette.placeholderText
                }
                // 密码输入框：默认隐藏，可点击眼睛图标切换明文
                Item {
                    Layout.fillWidth: true
                    implicitHeight: secretField.implicitHeight

                    TextField {
                        id: secretField
                        anchors.fill: parent
                        rightPadding: secretToggle.implicitWidth + 8
                        placeholderText: CredentialViewModel.editSecretReady
                            ? qsTr("留空使用原密钥")
                            : qsTr("正在获取原密钥…")
                        selectByMouse: true
                        font.family: "monospace"
                        echoMode: root.secretVisible ? TextInput.Normal : TextInput.Password
                    }

                    IconToolButton {
                        id: secretToggle
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        iconSource: root.secretVisible ? "qrc:/icons/eye-slash.svg" : "qrc:/icons/eye.svg"
                        onClicked: root.secretVisible = !root.secretVisible
                    }
                }
            }

            // 有效期（秒）：预填剩余秒数，提交时 expiry = 当前时刻 + 该值
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Label {
                    text: qsTr("有效期（秒）")
                    Layout.preferredWidth: 140
                    Layout.minimumWidth: 140
                    Layout.maximumWidth: 140
                    elide: Text.ElideRight
                }
                SpinBox {
                    id: editTtlSpin
                    Layout.fillWidth: true
                    from: 1
                    to: 2592000
                    value: {
                        var remain = Math.floor((root.editExpiryUnix - Date.now() / 1000) / 60) * 60
                        return Math.max(1, Math.min(2592000, remain))
                    }
                }
            }

            // ACL 组
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Label {
                    text: qsTr("ACL 组")
                    Layout.preferredWidth: 140
                    Layout.minimumWidth: 140
                    Layout.maximumWidth: 140
                    elide: Text.ElideRight
                }
                TextField {
                    id: editGroupsField
                    Layout.fillWidth: true
                    placeholderText: qsTr("逗号分隔，可留空")
                    selectByMouse: true
                }
            }

            // 允许代理的 CIDR
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Label {
                    text: qsTr("允许代理 CIDR")
                    Layout.preferredWidth: 140
                    Layout.minimumWidth: 140
                    Layout.maximumWidth: 140
                    elide: Text.ElideRight
                }
                TextField {
                    id: editCidrsField
                    Layout.fillWidth: true
                    placeholderText: qsTr("逗号分隔，可留空")
                    selectByMouse: true
                }
            }

            // 允许中继 + 可复用：两个勾选框分行垂直排列（英文文案较长，横排会溢出）
            CheckBox {
                id: editAllowRelayCheck
                text: qsTr("允许通过该凭证节点中继数据")
            }

            CheckBox {
                id: editReusableCheck
                text: qsTr("允许多个节点并发复用该凭证")
            }

            // 底部按钮：返回 / 保存
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Button {
                    text: qsTr("返回")
                    enabled: !root.mutating
                    onClicked: root.viewState = "list"
                }
                Item { Layout.fillWidth: true }
                RecommandButton {
                    text: root.creating
                          ? (root.mutating ? qsTr("生成中…") : qsTr("生成"))
                          : (root.mutating ? qsTr("保存中…") : qsTr("保存"))
                    enabled: !root.mutating && !root.fetchingSecret
                              && (root.creating || root.editCredentialId !== "")
                    onClicked: root.submitEdit()
                }
            }
        }

        // ========== 视图 3：操作成功（新增签发展示一次性密钥；编辑/撤销展示结果文案） ==========
        ColumnLayout {
            visible: root.viewState === "success"
            Layout.fillWidth: true
            Layout.bottomMargin: 12
            spacing: 10

            // 新增签发结果：密钥仅本次展示，关闭后无法再次查看
            ColumnLayout {
                visible: root.resultSecret !== ""
                Layout.fillWidth: true
                spacing: 10

                Label {
                    Layout.fillWidth: true
                    text: qsTr("关闭本页面后密钥将不再显示，请妥善保管")
                    font.bold: true
                    color: appTheme.statusRed
                    wrapMode: Text.WordWrap
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("临时凭证生成成功，复制密钥分发给其他节点即可临时加入网络：")
                    wrapMode: Text.WordWrap
                }

                // 凭证 ID
                RowLayout {
                    Layout.fillWidth: true
                    visible: root.resultCredentialId !== ""

                    Label {
                        text: qsTr("凭证 ID")
                        color: palette.placeholderText
                    }
                    Label {
                        text: root.resultCredentialId
                        font.family: "monospace"
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                }

                // 密钥内容（只读，可选中复制）
                Label {
                    Layout.fillWidth: true
                    text: qsTr("密钥（credential_secret）")
                    color: palette.placeholderText
                }
                TextField {
                    id: secretResultArea
                    text: root.resultSecret
                    readOnly: true
                    selectByMouse: true
                    font.family: "monospace"
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Button {
                        text: qsTr("复制密钥")
                        onClicked: {
                            secretResultArea.selectAll()
                            secretResultArea.copy()
                            copyResultHint.visible = true
                            copyResultHintTimer.restart()
                        }
                    }
                    Label {
                        id: copyResultHint
                        visible: false
                        text: qsTr("已复制到剪贴板")
                        color: appTheme.statusGreen
                        font: FontHelper.smallFont
                    }
                    Timer {
                        id: copyResultHintTimer
                        interval: 2000
                        onTriggered: copyResultHint.visible = false
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: qsTr("过期时间：%1").arg(root.resultExpiryText)
                        font: FontHelper.smallFont
                        color: palette.placeholderText
                    }
                }
            }

            // 编辑/撤销结果文案
            Label {
                visible: root.resultSecret === ""
                Layout.fillWidth: true
                text: root.resultTitle
                font.bold: true
                color: appTheme.statusGreen
            }
            Label {
                visible: root.resultSecret === ""
                Layout.fillWidth: true
                text: root.resultText
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                RecommandButton {
                    text: qsTr("完成")
                    onClicked: {
                        root.close()
                    }
                }
            }
        }

        // ========== 视图 4：操作失败 ==========
        ColumnLayout {
            visible: root.viewState === "error"
            Layout.fillWidth: true
            Layout.bottomMargin: 12
            spacing: 10

            Label {
                Layout.fillWidth: true
                text: root.resultTitle
                font.bold: true
                color: appTheme.statusRed
            }
            Label {
                Layout.fillWidth: true
                text: root.resultText
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                Button {
                    text: qsTr("返回")
                    onClicked: root.viewState = "list"
                }
            }
        }
    }
}
