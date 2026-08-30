/* @file GenerateCredentialDialog.qml (DDE)
 * @brief DDE 版临时节点密钥生成对话框：DTK DialogWindow（独立顶层窗），接口与共享版一致
 *
 * 参照 ErrorDialog：标题栏用 DialogTitleBar，底部按钮自建取消/生成/完成。
 */
import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk 1.0 as D

D.DialogWindow {
    id: root

    modality: Qt.ApplicationModal

    title: ""
    width: 600

    // 表单视图是否可见（false 时展示结果或错误视图）
    property bool showForm: true
    // 是否处于错误状态
    property bool hasError: false
    // 生成中标志
    readonly property bool generating: CredentialViewModel.generating

    // 生成成功后暂存的凭证字段
    property string resultCredentialId: ""
    property string resultSecret: ""
    property string resultExpiryText: ""

    function open() {
        showForm = true
        hasError = false
        resultCredentialId = ""
        resultSecret = ""
        resultExpiryText = ""
        visible = true
        requestActivate()
    }

    function close() {
        visible = false
    }

    // DialogWindow 的 content 只接受 Item，故以命名属性声明 Theme（QtObject）
    property Theme theme: Theme { }

    header: D.DialogTitleBar {
        title: qsTr("添加临时节点密钥")
    }

    // 格式化过期时间戳为本地时间
    function formatExpiry(expiryUnix) {
        if (!expiryUnix || expiryUnix <= 0)
            return qsTr("未知")
        var local = Qt.formatDateTime(new Date(expiryUnix * 1000), "yyyy-MM-dd hh:mm:ss")
        return qsTr("%1（本地时间）").arg(local)
    }

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        spacing: 12

        // 监听生成结果（Connections/Timer 为 QtObject，仅可作内容 Item 的 data 属性）
        Connections {
            target: CredentialViewModel

            function onGenerateSucceeded(credentialId, credentialSecret, expiryUnix) {
                resultCredentialId = credentialId || ""
                resultSecret = credentialSecret || ""
                resultExpiryText = root.formatExpiry(expiryUnix)
                hasError = false
                showForm = false
            }

            function onGenerateFailed(message) {
                errorText.text = message || qsTr("未知错误")
                hasError = true
                showForm = false
            }
        }

        // 视图 1：参数表单
        ColumnLayout {
            visible: root.showForm
            spacing: 14
            Layout.topMargin: 24

            // 有效期（秒）
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                D.Label {
                    text: qsTr("有效期（秒）")
                    Layout.preferredWidth: 140
                    color: palette.windowText
                }
                D.SpinBox {
                    id: ttlSpin
                    Layout.fillWidth: true
                    from: 1
                    to: 2592000
                    value: 3600
                    editable: true
                }
            }

            // 自定义凭证 ID
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                D.Label {
                    text: qsTr("凭证 ID（可选）")
                    Layout.preferredWidth: 140
                    color: palette.windowText
                }
                D.TextField {
                    id: idField
                    Layout.fillWidth: true
                    placeholderText: qsTr("留空则由服务端自动生成")
                    selectByMouse: true
                }
            }

            // 允许中继
            D.CheckBox {
                id: allowRelayCheck
                text: qsTr("允许通过该凭证节点中继数据")
            }

            // 可复用
            D.CheckBox {
                id: reusableCheck
                text: qsTr("允许多个节点并发复用该凭证")
                checked: true
            }

            // ACL 组
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                D.Label {
                    text: qsTr("ACL 组")
                    Layout.preferredWidth: 140
                    color: palette.windowText
                }
                D.TextField {
                    id: groupsField
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
                    Layout.preferredWidth: 140
                    color: palette.windowText
                }
                D.TextField {
                    id: cidrsField
                    Layout.fillWidth: true
                    placeholderText: qsTr("逗号分隔，可留空")
                    selectByMouse: true
                }
            }

            // 目标实例提示
            D.Label {
                Layout.fillWidth: true
                text: qsTr("将当前实例「%1」作为凭证签发目标。")
                    .arg(VpnRuntimeService.activeInstanceName)
                font: FontHelper.smallFont
                color: palette.placeholderText
                wrapMode: Text.WordWrap
                visible: VpnRuntimeService.activeInstanceName !== ""
            }
        }

        // 视图 2：生成结果
        ColumnLayout {
            visible: !root.showForm && !root.hasError
            spacing: 10

            // 密钥仅本次展示，关闭后无法再次查看
            D.Label {
                Layout.fillWidth: true
                text: qsTr("关闭本页面后密钥将不再显示，请妥善保管")
                font.bold: true
                color: theme.statusRed
                wrapMode: Text.WordWrap
            }

            D.Label {
                Layout.fillWidth: true
                text: qsTr("临时凭证生成成功，复制密钥分发给其他节点即可临时加入网络：")
                color: palette.windowText
                wrapMode: Text.WordWrap
            }

            // 凭证 ID
            RowLayout {
                Layout.fillWidth: true
                visible: root.resultCredentialId !== ""

                D.Label {
                    text: qsTr("凭证 ID")
                    color: palette.placeholderText
                }
                D.Label {
                    text: root.resultCredentialId
                    font.family: "monospace"
                    color: palette.highlight
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }
            }

            // 密钥内容（只读，可选中复制）
            D.Label {
                Layout.fillWidth: true
                text: qsTr("密钥（credential_secret）")
                color: palette.placeholderText
            }
            D.TextField {
                id: secretArea
                text: root.resultSecret
                readOnly: true
                selectByMouse: true
                font.family: "monospace"
                color: palette.windowText
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                D.Button {
                    text: qsTr("复制密钥")
                    highlighted: true
                    onClicked: {
                        secretArea.selectAll()
                        secretArea.copy()
                        copyHint.visible = true
                        copyHintTimer.restart()
                    }
                }
                D.Label {
                    id: copyHint
                    visible: false
                    text: qsTr("已复制到剪贴板")
                    color: theme.statusGreen
                    font: FontHelper.smallFont
                }
                Timer {
                    id: copyHintTimer
                    interval: 2000
                    onTriggered: copyHint.visible = false
                }
                Item { Layout.fillWidth: true }
                D.Label {
                    text: qsTr("过期时间：%1").arg(root.resultExpiryText)
                    font: FontHelper.smallFont
                    color: palette.placeholderText
                }
            }
        }

        // 视图 3：错误
        ColumnLayout {
            visible: !root.showForm && root.hasError
            spacing: 10

            D.Label {
                Layout.fillWidth: true
                text: qsTr("生成失败")
                font.bold: true
                color: theme.statusRed
            }
            D.Label {
                id: errorText
                Layout.fillWidth: true
                color: palette.windowText
                wrapMode: Text.WordWrap
            }
        }

        // 底部按钮
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 16
            spacing: 10

            Item { Layout.fillWidth: true }

            // 表单视图：取消 / 生成
            D.Button {
                visible: root.showForm
                text: qsTr("取消")
                enabled: !root.generating
                onClicked: root.close()
            }
            D.Button {
                visible: root.showForm
                text: root.generating ? qsTr("生成中…") : qsTr("生成")
                highlighted: true
                enabled: !root.generating && VpnRuntimeService.activeInstanceName !== ""
                onClicked: {
                    CredentialViewModel.generateCredential(
                        VpnRuntimeService.activeInstanceName,
                        ttlSpin.value,
                        groupsField.text,
                        allowRelayCheck.checked,
                        cidrsField.text,
                        idField.text,
                        reusableCheck.checked)
                }
            }

            // 结果 / 错误视图：完成
            D.Button {
                visible: !root.showForm
                text: qsTr("完成")
                highlighted: !root.hasError
                onClicked: root.close()
            }
        }

        // 底部留白占位
        Item {
            Layout.preferredHeight: 8
        }
    }
}
