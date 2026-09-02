/* @brief 添加临时节点密钥对话框：填写参数签发安全模式临时凭证，成功后展示密钥并支持一键复制，Swb 控件迁移版 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtEasyTier
import SwbControls

/* @brief 临时节点密钥生成对话框，包含完整参数表单与结果/错误展示 */
SwbDialog {
    id: root

    title: qsTr("添加临时节点密钥")
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(520, parent ? parent.width - 48 : 480)
    standardButtons: Dialog.None

    Theme { id: appTheme }

    /* 表单视图是否可见（false 时展示结果或错误视图） */
    property bool showForm: true
    /* 是否处于错误状态 */
    property bool hasError: false
    /* 生成中标志（禁用按钮） */
    readonly property bool generating: CredentialViewModel.generating

    /* 生成成功后暂存的凭证字段 */
    property string resultCredentialId: ""
    property string resultSecret: ""
    property string resultExpiryText: ""

    // 每次打开重置为表单视图
    onOpened: {
        showForm = true
        hasError = false
        resultCredentialId = ""
        resultSecret = ""
        resultExpiryText = ""
    }

    /* 将 Unix 秒级时间戳（过期时刻）格式化为固定格式的本地时间 */
    function formatExpiry(expiryUnix) {
        if (!expiryUnix || expiryUnix <= 0)
            return qsTr("未知")
        var local = Qt.formatDateTime(new Date(expiryUnix * 1000), "yyyy-MM-dd hh:mm:ss")
        return qsTr("%1（本地时间）").arg(local)
    }

    // 监听生成结果
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

    contentItem: ColumnLayout {
        spacing: 12

        // ========== 视图 1：参数表单 ==========
        ColumnLayout {
            visible: root.showForm
            spacing: 10

            // 有效期（秒）
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                SwbLabel {
                    text: qsTr("有效期（秒）")
                    Layout.preferredWidth: 130
                }
                SwbSpinBox {
                    id: ttlSpin
                    Layout.fillWidth: true
                    from: 1
                    to: 2592000
                    value: 3600
                }
            }

            // 自定义凭证 ID
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                SwbLabel {
                    text: qsTr("凭证 ID（可选）")
                    Layout.preferredWidth: 130
                }
                SwbTextField {
                    id: idField
                    Layout.fillWidth: true
                    placeholderText: qsTr("留空则由服务端自动生成")
                    selectByMouse: true
                }
            }

            // 允许中继
            SwbCheckBox {
                id: allowRelayCheck
                text: qsTr("允许通过该凭证节点中继数据")
            }

            // 可复用
            SwbCheckBox {
                id: reusableCheck
                text: qsTr("允许多个节点并发复用该凭证")
                checked: true
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

                SwbLabel {
                    text: qsTr("允许代理 CIDR")
                    Layout.preferredWidth: 130
                }
                SwbTextField {
                    id: cidrsField
                    Layout.fillWidth: true
                    placeholderText: qsTr("逗号分隔，可留空")
                    selectByMouse: true
                }
            }

            // 目标实例提示
            SwbLabel {
                Layout.fillWidth: true
                text: qsTr("将当前实例「%1」作为凭证签发目标。")
                    .arg(VpnRuntimeService.activeInstanceName)
                font: FontHelper.smallFont
                color: SwbTheme.mutedForeground
                wrapMode: Text.WordWrap
                visible: VpnRuntimeService.activeInstanceName !== ""
            }
        }

        // ========== 视图 2：生成结果 ==========
        ColumnLayout {
            visible: !root.showForm && !root.hasError
            spacing: 10

            // 密钥仅本次展示，关闭后无法再次查看
            SwbLabel {
                Layout.fillWidth: true
                text: qsTr("关闭本页面后密钥将不再显示，请妥善保管")
                font.bold: true
                color: appTheme.statusRed
                wrapMode: Text.WordWrap
            }

            SwbLabel {
                Layout.fillWidth: true
                text: qsTr("临时凭证生成成功，复制密钥分发给其他节点即可临时加入网络：")
                color: SwbTheme.foreground
                wrapMode: Text.WordWrap
            }

            // 凭证 ID
            RowLayout {
                Layout.fillWidth: true
                visible: root.resultCredentialId !== ""

                SwbLabel {
                    text: qsTr("凭证 ID")
                    color: SwbTheme.mutedForeground
                }
                SwbLabel {
                    text: root.resultCredentialId
                    font.family: "monospace"
                    color: SwbTheme.foreground
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }
            }

            // 密钥内容（只读，可选中复制）
            SwbLabel {
                Layout.fillWidth: true
                text: qsTr("密钥（credential_secret）")
                color: SwbTheme.mutedForeground
            }
            SwbTextField {
                id: secretArea
                text: root.resultSecret
                readOnly: true
                selectByMouse: true
                font.family: "monospace"
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                SwbButton {
                    text: qsTr("复制密钥")
                    onClicked: {
                        secretArea.selectAll()
                        secretArea.copy()
                        copyHint.visible = true
                        copyHintTimer.restart()
                    }
                }
                SwbLabel {
                    id: copyHint
                    visible: false
                    text: qsTr("已复制到剪贴板")
                    color: appTheme.statusGreen
                    font: FontHelper.smallFont
                }
                Timer {
                    id: copyHintTimer
                    interval: 2000
                    onTriggered: copyHint.visible = false
                }
                Item { Layout.fillWidth: true }
                SwbLabel {
                    text: qsTr("过期时间：%1").arg(root.resultExpiryText)
                    font: FontHelper.smallFont
                    color: SwbTheme.mutedForeground
                }
            }
        }

        // ========== 视图 3：错误 ==========
        ColumnLayout {
            visible: !root.showForm && root.hasError
            spacing: 10

            SwbLabel {
                Layout.fillWidth: true
                text: qsTr("生成失败")
                font.bold: true
                color: appTheme.statusRed
            }
            SwbLabel {
                id: errorText
                Layout.fillWidth: true
                color: SwbTheme.foreground
                wrapMode: Text.WordWrap
            }
        }

        // ========== 底部按钮 ==========
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Item { Layout.fillWidth: true }

            // 表单视图：取消 / 生成
            SwbButton {
                visible: root.showForm
                variant: "outline"
                text: qsTr("取消")
                enabled: !root.generating
                onClicked: root.close()
            }
            SwbButton {
                visible: root.showForm
                text: root.generating ? qsTr("生成中…") : qsTr("生成")
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
            SwbButton {
                visible: !root.showForm
                text: qsTr("完成")
                onClicked: root.close()
            }
        }
    }
}
