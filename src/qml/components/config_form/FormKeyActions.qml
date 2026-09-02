/* @brief 密钥操作动作行渲染器：随机生成私钥 / 实时显示公钥，公钥展示对话框（可复制）内嵌于此 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtEasyTier
import SwbControls

/* @brief 密钥操作渲染器根布局 */
ColumnLayout {
    id: root

    /* 字段元数据（keyActions 无数据绑定，仅为分发占位） */
    required property var field

    width: parent ? parent.width : 0

    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 4

        SwbButton {
            text: qsTr("随机生成")
            onClicked: {
                if (!ConfigEditorViewModel.generateRandomPrivateKey())
                    AppState.showError(qsTr("随机私钥生成失败"))
            }
        }

        SwbButton {
            variant: "outline"
            text: qsTr("显示公钥")
            onClicked: {
                var pub = ConfigEditorViewModel.derivePublicKey()
                if (pub === "") {
                    AppState.showError(qsTr("私钥为空或无效，请先输入或随机生成私钥"))
                    return
                }
                publicKeyField.text = pub
                publicKeyDialog.open()
            }
        }
    }

    // 显示公钥对话框：由安全模式私钥实时计算（公钥不落库），支持一键复制
    SwbDialog {
        id: publicKeyDialog
        title: qsTr("本地公钥")
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(520, parent ? parent.width - 48 : 480)

        ColumnLayout {
            width: parent ? parent.width : 480
            spacing: 8

            SwbLabel { text: qsTr("复制以下公钥即可分享给其他节点：") }

            SwbTextField {
                id: publicKeyField
                readOnly: true
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true

                Item { Layout.fillWidth: true }

                SwbButton {
                    text: qsTr("复制")
                    onClicked: {
                        publicKeyField.selectAll()
                        publicKeyField.copy()
                    }
                }

                SwbButton {
                    variant: "outline"
                    text: qsTr("确定")
                    onClicked: publicKeyDialog.close()
                }
            }
        }
    }
}
