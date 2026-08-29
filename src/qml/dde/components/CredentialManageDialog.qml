/* @file CredentialManageDialog.qml (DDE)
 * @brief DDE 版临时节点密钥管理对话框：DTK DialogWindow（独立顶层窗），接口与共享版一致
 *
 * 参照 ErrorDialog：标题栏用 DialogTitleBar；关闭与标题栏 X 均发出 closed（兼容共享版 onClosed）。
 */
import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk 1.0 as D

D.DialogWindow {
    id: root

    title: ""
    width: 640

    // 视图状态：list 列表 / fetching 取钥中 / edit 编辑 / success 成功 / error 失败
    property string viewState: "list"
    // 列表查询中
    readonly property bool listing: CredentialViewModel.listing
    // 变更操作中
    readonly property bool mutating: CredentialViewModel.mutating
    // 编辑取钥进行中
    readonly property bool fetchingSecret: CredentialViewModel.fetchingSecret
    // 密钥输入框是否明文显示
    property bool secretVisible: false

    // 编辑表单预填数据（点击列表项"编辑"时暂存）
    property string editCredentialId: ""
    property double editExpiryUnix: 0
    property string editGroupsText: ""
    property bool editAllowRelay: false
    property string editCidrsText: ""
    property bool editReusable: true

    // 结果/错误视图文案
    property string resultTitle: ""
    property string resultText: ""

    // 窗口关闭（含标题栏 X）时发出，兼容共享版 QQC.Dialog.onClosed
    signal closed()

    // 真正打开过标记：仅在 open→close 结束才发 closed，忽略初始 hidden 与重复隐藏
    property bool hadShown: false

    function open() {
        viewState = "list"
        resultTitle = ""
        resultText = ""
        refreshList()
        hadShown = true
        visible = true
        requestActivate()
    }

    function close() {
        visible = false
    }

    // DialogWindow 关闭走 hide()，标题栏 X 关闭时也同步 closed
    onVisibleChanged: {
        if (!visible && hadShown) {
            hadShown = false
            closed()
        }
    }

    header: D.DialogTitleBar {
        title: qsTr("管理临时节点密钥")
    }

    // DialogWindow 的 content 只接受 Item，故以命名属性声明 Theme（QtObject）
    property Theme theme: Theme { }

    // 刷新当前实例的凭证列表
    function refreshList() {
        CredentialViewModel.listCredentials(VpnRuntimeService.activeInstanceName)
    }

    // 格式化过期时间戳为本地时间
    function formatExpiry(expiryUnix) {
        if (!expiryUnix || expiryUnix <= 0)
            return qsTr("未知")
        var local = Qt.formatDateTime(new Date(expiryUnix * 1000), "yyyy-MM-dd hh:mm:ss")
        return qsTr("%1（本地时间）").arg(local)
    }

    // 拼接凭证授权约束摘要文本
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

    // 点击"编辑"：先自动获取原密钥，取到后才进入编辑表单
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

    // 点击"撤销"：弹出确认对话框
    function beginRevoke(credentialId) {
        revokeId = credentialId || ""
        revokeConfirmDialog.open()
    }

    // 提交编辑：expiry = 当前时刻 + 有效期（秒）
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

    // 撤销确认对话框：Window 非 Item，不能作 DialogWindow 的 content 子项，故以命名属性挂载
    property var revokeConfirmDialog: D.DialogWindow {
        id: revokeConfirmDialog
        title: ""
        width: Math.min(420, root.width - 48)

        function open() {
            visible = true
            requestActivate()
        }

        function close() {
            visible = false
        }

        header: D.DialogTitleBar {
            title: qsTr("撤销凭证")
        }

        ColumnLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            spacing: 12

            D.Label {
                Layout.fillWidth: true
                Layout.topMargin: 8
                wrapMode: Text.WordWrap
                text: qsTr("确定撤销凭证「%1」吗？撤销后使用该凭证的节点将无法加入网络。").arg(root.revokeId)
                color: palette.windowText
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16
                spacing: 10

                Item { Layout.fillWidth: true }

                D.Button {
                    text: qsTr("取消")
                    onClicked: revokeConfirmDialog.close()
                }
                D.Button {
                    text: qsTr("撤销")
                    highlighted: true
                    onClicked: {
                        CredentialViewModel.revokeCredential(
                            VpnRuntimeService.activeInstanceName, root.revokeId)
                        revokeConfirmDialog.close()
                    }
                }
            }

            // 底部留白占位
            Item {
                Layout.preferredHeight: 8
            }
        }
    }

    // 新增临时节点密钥对话框：Window 非 Item，以命名属性挂载
    property var generateCredentialDialog: GenerateCredentialDialog {
        id: generateCredentialDialog
    }

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        spacing: 12

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

        // 视图 1：凭证列表
        ColumnLayout {
            visible: root.viewState === "list"
            spacing: 8
            Layout.topMargin: 24

            // 目标实例提示
            D.Label {
                Layout.fillWidth: true
                text: qsTr("当前实例「%1」已签发的临时凭证。").arg(VpnRuntimeService.activeInstanceName)
                font: FontHelper.smallFont
                color: palette.placeholderText
                wrapMode: Text.WordWrap
                visible: VpnRuntimeService.activeInstanceName !== ""
            }

            // 加载中提示
            D.Label {
                Layout.fillWidth: true
                visible: root.listing
                text: qsTr("加载中…")
                font: FontHelper.smallFont
                color: palette.placeholderText
            }

            // 空状态
            D.Label {
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
            QQC.ListView {
                visible: !root.listing && CredentialViewModel.credentialListModel.count > 0
                Layout.fillWidth: true
                Layout.preferredHeight: 320
                clip: true
                model: CredentialViewModel.credentialListModel
                spacing: 6

                delegate: Card {
                    width: ListView.view ? ListView.view.width : 0
                    contentSpacing: 4

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        // 凭证 ID + 过期时间
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            D.Label {
                                text: credentialId || ""
                                font.bold: true
                                font.family: "monospace"
                                color: palette.highlight
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }

                            D.Label {
                                text: qsTr("过期：%1").arg(root.formatExpiry(expiryUnix))
                                font: FontHelper.smallFont
                                color: palette.placeholderText
                            }
                        }

                        // 公钥指纹
                        D.Label {
                            visible: (publicKeyFingerprint || "") !== ""
                            text: qsTr("指纹：%1").arg(publicKeyFingerprint || "")
                            font.family: "monospace"
                            font.pixelSize: 11
                            color: palette.placeholderText
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }

                        // 授权约束摘要 + 操作按钮
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            D.Label {
                                text: root.formatConstraints(groups, allowRelay, reusable)
                                font: FontHelper.smallFont
                                color: palette.placeholderText
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            D.Button {
                                text: qsTr("编辑")
                                enabled: !root.mutating
                                onClicked: root.beginEdit(credentialId, expiryUnix,
                                                          groups, allowRelay,
                                                          allowedProxyCidrs, reusable)
                            }
                            D.Button {
                                text: qsTr("撤销")
                                enabled: !root.mutating
                                onClicked: root.beginRevoke(credentialId)
                            }
                        }
                    }
                }
            }

            // 底部按钮：刷新 / 关闭
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16
                spacing: 10

                Item { Layout.fillWidth: true }

                D.Button {
                    text: qsTr("新增临时节点密钥")
                    enabled: !root.listing && !root.mutating
                    onClicked: generateCredentialDialog.open()
                }
                D.Button {
                    text: qsTr("刷新")
                    enabled: !root.listing && !root.mutating
                    onClicked: root.refreshList()
                }
                D.Button {
                    text: qsTr("关闭")
                    enabled: !root.mutating
                    onClicked: root.close()
                }
            }
        }

        // 视图：获取原密钥
        ColumnLayout {
            visible: root.viewState === "fetching"
            spacing: 12

            D.Label {
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: qsTr("正在获取原密钥…")
                color: palette.placeholderText
            }
        }

        // 视图 2：编辑表单
        ColumnLayout {
            visible: root.viewState === "edit"
            spacing: 14

            D.Label {
                Layout.fillWidth: true
                text: qsTr("编辑凭证")
                font.bold: true
                color: palette.windowText
            }

            // 凭证 ID（只读）
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                D.Label {
                    text: qsTr("凭证 ID")
                    Layout.preferredWidth: 150
                    color: palette.windowText
                }
                D.TextField {
                    text: root.editCredentialId
                    readOnly: true
                    Layout.fillWidth: true
                }
            }

            // 密钥内容（留空用原密钥，或手动粘贴新密钥）
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                D.Label {
                    text: qsTr("密钥（credential_secret）")
                    color: palette.placeholderText
                }
                // 密码输入框：默认隐藏，可点击眼睛图标切换明文
                Item {
                    Layout.fillWidth: true
                    implicitHeight: secretField.implicitHeight

                    D.TextField {
                        id: secretField
                        anchors.fill: parent
                        rightPadding: secretToggle.implicitWidth + 8
                        placeholderText: CredentialViewModel.editSecretReady
                            ? qsTr("留空使用原密钥")
                            : qsTr("正在获取原密钥…")
                        selectByMouse: true
                        font.family: "monospace"
                        color: palette.windowText
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

                D.Label {
                    text: qsTr("有效期（秒）")
                    Layout.preferredWidth: 150
                    color: palette.windowText
                }
                D.SpinBox {
                    id: editTtlSpin
                    Layout.fillWidth: true
                    from: 1
                    to: 2592000
                    editable: true
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

                D.Label {
                    text: qsTr("ACL 组")
                    Layout.preferredWidth: 150
                    color: palette.windowText
                }
                D.TextField {
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

                D.Label {
                    text: qsTr("允许代理 CIDR")
                    Layout.preferredWidth: 150
                    color: palette.windowText
                }
                D.TextField {
                    id: editCidrsField
                    text: root.editCidrsText
                    Layout.fillWidth: true
                    placeholderText: qsTr("逗号分隔，可留空")
                    selectByMouse: true
                }
            }

            // 允许中继 + 可复用（两个勾选框放在一起）
            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                D.CheckBox {
                    id: editAllowRelayCheck
                    text: qsTr("允许通过该凭证节点中继数据")
                    checked: root.editAllowRelay
                }

                D.CheckBox {
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

                D.Button {
                    text: qsTr("返回")
                    enabled: !root.mutating
                    onClicked: root.viewState = "list"
                }
                D.Button {
                    text: root.mutating ? qsTr("保存中…") : qsTr("保存")
                    highlighted: true
                    enabled: !root.mutating && !root.fetchingSecret && root.editCredentialId !== ""
                    onClicked: root.submitEdit()
                }
            }
        }

        // 视图 3：操作成功
        ColumnLayout {
            visible: root.viewState === "success"
            spacing: 10

            D.Label {
                Layout.fillWidth: true
                text: root.resultTitle
                font.bold: true
                color: theme.statusGreen
            }
            D.Label {
                Layout.fillWidth: true
                text: root.resultText
                color: palette.windowText
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                D.Button {
                    text: qsTr("完成")
                    highlighted: true
                    onClicked: {
                        root.close()
                    }
                }
            }
        }

        // 视图 4：操作失败
        ColumnLayout {
            visible: root.viewState === "error"
            spacing: 10

            D.Label {
                Layout.fillWidth: true
                text: root.resultTitle
                font.bold: true
                color: theme.statusRed
            }
            D.Label {
                Layout.fillWidth: true
                text: root.resultText
                color: palette.windowText
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Item { Layout.fillWidth: true }

                D.Button {
                    text: qsTr("返回")
                    onClicked: root.viewState = "list"
                }
            }
        }

        // 底部留白占位
        Item {
            Layout.preferredHeight: 16
        }
    }
}
