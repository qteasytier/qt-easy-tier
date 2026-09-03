/* @file FormKeyActions.qml (DDE)
 * @brief DDE 版密钥操作动作行渲染器：随机生成私钥 / 实时显示公钥（模态对话框，可复制）
 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

/* @brief 密钥操作渲染器根布局 */
ColumnLayout {
    id: root

    /* 字段元数据（keyActions 无数据绑定，仅为分发占位） */
    required property var field

    width: parent ? parent.width : 0

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
                publicKeyDialog.open()
            }
        }
    }

    // 显示公钥对话框（模态独立窗）：由安全模式私钥实时计算（公钥不落库），支持一键复制
    DialogWindow {
        id: publicKeyDialog

        modality: Qt.ApplicationModal

        title: ""
        width: 520

        function open() {
            visible = true
            requestActivate()
        }

        function close() {
            visible = false
        }

        header: DialogTitleBar {
            title: qsTr("本地公钥")
        }

        ColumnLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            spacing: 16

            Label {
                Layout.fillWidth: true
                Layout.topMargin: 12
                text: qsTr("复制以下公钥即可分享给其他节点：")
            }

            TextField {
                id: publicKeyField
                Layout.fillWidth: true
                readOnly: true
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: 12
                spacing: 10

                Item { Layout.fillWidth: true }

                Button {
                    text: qsTr("复制")
                    onClicked: {
                        publicKeyField.selectAll()
                        publicKeyField.copy()
                    }
                }

                RecommandButton {
                    text: qsTr("确定")
                    onClicked: publicKeyDialog.close()
                }
            }
        }
    }
}
