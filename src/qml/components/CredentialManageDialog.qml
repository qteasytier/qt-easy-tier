/* @brief 管理临时节点密钥对话框：查询/编辑/撤销当前实例已签发的安全模式临时凭证，Swb 控件迁移版 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtEasyTier
import SwbControls

/* @brief 临时凭证管理对话框，包含凭证列表、编辑表单与结果/错误视图 */
SwbDialog {
    id: root

    title: qsTr("管理临时节点密钥")
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(560, parent ? parent.width - 48 : 520)
    standardButtons: Dialog.None

    Theme { id: appTheme }

    /* 视图状态："list" 凭证列表 / "fetching" 获取原密钥中 / "edit" 编辑表单 / "success" 操作成功 / "error" 操作失败 */
    property string viewState: "list"
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

    // 每次打开重置为列表视图并刷新
    onOpened: {
        viewState = "list"
        resultTitle = ""
        resultText = ""
        refreshList()
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

    /* 点击列表项"编辑"：暂存该凭证数据，先自动获取原密钥，取到后才进入编辑表单 */
    function beginEdit(credentialId, expiryUnix, groups, allowRelay, cidrs, reusable) {
        editCredentialId = credentialId || ""
        editExpiryUnix = expiryUnix || 0
        editGroupsText = (groups || []).join(", ")
        editAllowRelay = allowRelay === true
        editCidrsText = (cidrs || []).join(", ")
        editReusable = reusable !== false
        viewState = "fetching"
        CredentialViewModel.prepareEdit(VpnRuntimeService.activeInstanceName, editCredentialId)
    }

    /* 点击列表项"撤销"：弹出确认对话框 */
    function beginRevoke(credentialId) {
        revokeId = credentialId || ""
        revokeConfirmDialog.open()
    }

    /* 提交编辑：expiry 由当前时刻 + 有效期（秒）计算 */
    function submitEdit() {
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

    property string revokeId: ""

    /* 撤销确认对话框 */
    SwbDialog {
        id: revokeConfirmDialog
        title: qsTr("撤销凭证")
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(420, root.width - 48)
        standardButtons: Dialog.Yes | Dialog.No

        SwbLabel {
            width: parent.width
            wrapMode: Text.WordWrap
            text: qsTr("确定撤销凭证「%1」吗？撤销后使用该凭证的节点将无法加入网络。").arg(root.revokeId)
            color: SwbTheme.foreground
        }

        onAccepted: {
            CredentialViewModel.revokeCredential(
                VpnRuntimeService.activeInstanceName, root.revokeId)
        }
    }

    // 新增临时节点密钥对话框（签发成功后回到列表并刷新）
    GenerateCredentialDialog {
        id: generateCredentialDialog
    }

    // 监听服务结果：刷新列表、切换结果/错误视图
    Connections {
        target: CredentialViewModel

        // 取到原密钥后才进入编辑表单
        function onEditSecretReadyChanged() {
            if (root.viewState === "fetching" && CredentialViewModel.editSecretReady)
                root.viewState = "edit"
        }

        // 新增临时凭证成功（生成对话框）→ 回到列表并刷新
        function onGenerateSucceeded() {
            if (root.viewState === "list")
                root.refreshList()
        }

        // 取钥失败（仅 admin 节点可自动获取）→ 返回列表并提示
        function onGenerateFailed(message) {
            if (root.viewState !== "fetching")
                return
            root.resultTitle = qsTr("获取原密钥失败")
            root.resultText = message || qsTr("未知错误")
            root.viewState = "error"
        }

        function onListFailed(message) {
            if (root.viewState !== "list")
                return
            root.resultTitle = qsTr("查询失败")
            root.resultText = message || qsTr("未知错误")
            root.viewState = "error"
        }

        function onUpsertSucceeded(changed) {
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

    contentItem: ColumnLayout {
        spacing: 12

        // ========== 视图 1：凭证列表 ==========
        ColumnLayout {
            visible: root.viewState === "list"
            spacing: 8

            // 目标实例提示
            SwbLabel {
                Layout.fillWidth: true
                text: qsTr("当前实例「%1」已签发的临时凭证。").arg(VpnRuntimeService.activeInstanceName)
                font: FontHelper.smallFont
                color: SwbTheme.mutedForeground
                wrapMode: Text.WordWrap
                visible: VpnRuntimeService.activeInstanceName !== ""
            }

            // 加载中提示
            SwbLabel {
                Layout.fillWidth: true
                visible: root.listing
                text: qsTr("加载中…")
                font: FontHelper.smallFont
                color: SwbTheme.mutedForeground
            }

            // 空状态
            SwbLabel {
                visible: !root.listing && CredentialViewModel.credentialListModel.count === 0
                text: qsTr("暂无已签发的临时凭证")
                font.pixelSize: 18
                color: SwbTheme.mutedForeground
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            // 凭证列表
            SwbScrollView {
                visible: !root.listing && CredentialViewModel.credentialListModel.count > 0
                Layout.fillWidth: true
                Layout.preferredHeight: 260
                clip: true

                ListView {
                    id: credentialListView
                    model: CredentialViewModel.credentialListModel
                    spacing: 6

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

                                SwbLabel {
                                    text: credentialId || ""
                                    font.bold: true
                                    font.family: "monospace"
                                    color: SwbTheme.foreground
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }

                                SwbLabel {
                                    text: qsTr("过期：%1").arg(root.formatExpiry(expiryUnix))
                                    font: FontHelper.smallFont
                                    color: SwbTheme.mutedForeground
                                }
                            }

                            // 第二行：公钥指纹
                            SwbLabel {
                                visible: (publicKeyFingerprint || "") !== ""
                                text: qsTr("指纹：%1").arg(publicKeyFingerprint || "")
                                font.family: "monospace"
                                font.pixelSize: 11
                                color: SwbTheme.mutedForeground
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }

                            // 第三行：授权约束摘要 + 操作按钮
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                SwbLabel {
                                    text: root.formatConstraints(groups, allowRelay, reusable)
                                    font: FontHelper.smallFont
                                    color: SwbTheme.mutedForeground
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                SwbButton {
                                    size: "sm"
                                    variant: "outline"
                                    text: qsTr("编辑")
                                    enabled: !root.mutating
                                    onClicked: root.beginEdit(credentialId, expiryUnix,
                                                              groups, allowRelay,
                                                              allowedProxyCidrs, reusable)
                                }
                                SwbButton {
                                    size: "sm"
                                    variant: "destructive"
                                    text: qsTr("撤销")
                                    enabled: !root.mutating
                                    onClicked: root.beginRevoke(credentialId)
                                }
                            }
                        }
                    }
                }
            }

            // 底部按钮：刷新 / 关闭
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                SwbButton {
                    text: qsTr("新增临时节点密钥")
                    enabled: !root.listing && !root.mutating
                    onClicked: generateCredentialDialog.open()
                }
                SwbButton {
                    variant: "outline"
                    text: qsTr("刷新")
                    enabled: !root.listing && !root.mutating
                    onClicked: root.refreshList()
                }
                SwbButton {
                    variant: "outline"
                    text: qsTr("关闭")
                    enabled: !root.mutating
                    onClicked: root.close()
                }
            }
        }

        // ========== 视图：获取原密钥（取到后才进入编辑表单） ==========
        ColumnLayout {
            visible: root.viewState === "fetching"
            spacing: 10

            SwbLabel {
                Layout.fillWidth: true
                Layout.preferredHeight: 160
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: qsTr("正在获取原密钥…")
                color: SwbTheme.mutedForeground
            }
        }

        // ========== 视图 2：编辑表单 ==========
        ColumnLayout {
            visible: root.viewState === "edit"
            spacing: 10

            SwbLabel {
                Layout.fillWidth: true
                text: qsTr("编辑凭证")
                font.bold: true
                color: SwbTheme.foreground
            }

            // 凭证 ID（只读）
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                SwbLabel {
                    text: qsTr("凭证 ID")
                    Layout.preferredWidth: 130
                }
                SwbTextField {
                    text: root.editCredentialId
                    readOnly: true
                    Layout.fillWidth: true
                }
            }

            // 密钥内容（可选：已自动取回原密钥，留空则使用原密钥；也可手动粘贴新密钥替换）
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                SwbLabel {
                    text: qsTr("密钥（credential_secret）")
                    color: SwbTheme.mutedForeground
                }
                // 密码输入框：默认隐藏，可点击眼睛图标切换明文
                Item {
                    Layout.fillWidth: true
                    implicitHeight: secretField.implicitHeight

                    SwbTextField {
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

                SwbLabel {
                    text: qsTr("有效期（秒）")
                    Layout.preferredWidth: 130
                }
                SwbSpinBox {
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

                SwbLabel {
                    text: qsTr("ACL 组")
                    Layout.preferredWidth: 130
                }
                SwbTextField {
                    id: editGroupsField
                    text: root.editGroupsText
                    Layout.fillWidth: true
                    placeholderText: qsTr("逗号分隔，可留空")
                    selectByMouse: true
                }
            }

            // 允许代理的 CIDR
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                SwbLabel {
                    text: qsTr("允许代理 CIDR")
                    Layout.preferredWidth: 130
                }
                SwbTextField {
                    id: editCidrsField
                    text: root.editCidrsText
                    Layout.fillWidth: true
                    placeholderText: qsTr("逗号分隔，可留空")
                    selectByMouse: true
                }
            }

            // 允许中继 + 可复用：两个勾选框并排一行，置于表单最下方
            RowLayout {
                Layout.fillWidth: true
                spacing: 24

                SwbCheckBox {
                    id: editAllowRelayCheck
                    text: qsTr("允许通过该凭证节点中继数据")
                    checked: root.editAllowRelay
                }

                SwbCheckBox {
                    id: editReusableCheck
                    text: qsTr("允许多个节点并发复用该凭证")
                    checked: root.editReusable
                }
            }

            // 底部按钮：返回 / 保存
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                SwbButton {
                    variant: "outline"
                    text: qsTr("返回")
                    enabled: !root.mutating
                    onClicked: root.viewState = "list"
                }
                SwbButton {
                    text: root.mutating ? qsTr("保存中…") : qsTr("保存")
                    enabled: !root.mutating && !root.fetchingSecret && root.editCredentialId !== ""
                    onClicked: root.submitEdit()
                }
            }
        }

        // ========== 视图 3：操作成功 ==========
        ColumnLayout {
            visible: root.viewState === "success"
            spacing: 10

            SwbLabel {
                Layout.fillWidth: true
                text: root.resultTitle
                font.bold: true
                color: appTheme.statusGreen
            }
            SwbLabel {
                Layout.fillWidth: true
                text: root.resultText
                color: SwbTheme.foreground
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                SwbButton {
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
            spacing: 10

            SwbLabel {
                Layout.fillWidth: true
                text: root.resultTitle
                font.bold: true
                color: appTheme.statusRed
            }
            SwbLabel {
                Layout.fillWidth: true
                text: root.resultText
                color: SwbTheme.foreground
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                SwbButton {
                    variant: "outline"
                    text: qsTr("返回")
                    onClicked: root.viewState = "list"
                }
            }
        }
    }
}
