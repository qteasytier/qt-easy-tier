/* @brief 代理子网列表组件：管理 cidr + mappedCidr + allow 列表，支持添加、删除和去重检测，Swb 控件迁移版 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtEasyTier
import SwbControls

ColumnLayout {
    id: root

    property alias model: listModel

    signal changed()
    signal duplicateDetected(string msg)

    spacing: 4

    // 状态色(添加按钮文字统一用 statusGreen)
    Theme { id: theme }

    function allProtocols() {
        return ["tcp", "udp", "icmp"]
    }

    function normalizedAllow(allow) {
        if (!allow || allow.length === 0)
            return allProtocols()

        var result = []
        for (var i = 0; i < allow.length; i++) {
            if ((allow[i] === "tcp" || allow[i] === "udp" || allow[i] === "icmp")
                    && result.indexOf(allow[i]) === -1) {
                result.push(allow[i])
            }
        }
        return result.length > 0 ? result : allProtocols()
    }

    function allowText(allow) {
        return normalizedAllow(allow).join(", ")
    }

    ListModel { id: listModel }

    ListView {
        id: listView
        Layout.fillWidth: true
        // 动态计算高度（行高 42，分隔线风格无行距）
        Layout.preferredHeight: listModel.count === 0 ? 0 : listModel.count * 42
        spacing: 0
        model: listModel
        interactive: false

        delegate: Rectangle {
            id: proxyRow

            required property int index
            required property var model

            width: ListView.view.width
            height: 42
            color: rowHover.hovered ? SwbTheme.withAlpha(SwbTheme.foreground, 0.04)
                 : "transparent"
            radius: SwbTheme.radiusSm

            Behavior on color {
                ColorAnimation { duration: SwbTheme.animationDuration }
            }

            HoverHandler {
                id: rowHover
            }

            // 行底分隔线（最后一行不显示）
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: SwbTheme.border
                visible: proxyRow.index < listView.count - 1
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 4
                spacing: 4

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    SwbLabel {
                        Layout.fillWidth: true
                        text: model.mappedCidr ? model.cidr + " -> " + model.mappedCidr : model.cidr
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    SwbLabel {
                        Layout.fillWidth: true
                        text: qsTr("协议: ") + root.allowText(model.allow)
                        color: SwbTheme.mutedForeground
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                }

                IconToolButton {
                    iconSource: "qrc:/icons/delete.svg"
                    flat: true
                    onClicked: {
                        listModel.remove(index)
                        root.changed()
                    }
                }
            }
        }
    }

    SwbButton {
        id: addButton
        Layout.fillWidth: true
        variant: "ghost"
        size: "sm"
        text: qsTr("添加代理子网")
        textColor: theme.statusGreen
        onClicked: {
            addDialog.cidrText = ""
            addDialog.mappedCidrText = ""
            tcpCheck.checked = true
            udpCheck.checked = true
            icmpCheck.checked = true
            addDialog.open()
        }
    }

    SwbDialog {
        id: addDialog
        title: qsTr("添加代理子网")
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(420, parent ? parent.width - 48 : 360)

        property alias cidrText: cidrField.text
        property alias mappedCidrText: mappedCidrField.text

        function acceptProxyNetwork() {
            var cidr = cidrField.text.trim()
            if (!cidr)
                return

            var allow = []
            if (tcpCheck.checked) allow.push("tcp")
            if (udpCheck.checked) allow.push("udp")
            if (icmpCheck.checked) allow.push("icmp")

            if (allow.length === 0) {
                root.duplicateDetected(qsTr("代理协议至少选择一项"))
                return
            }

            for (var i = 0; i < listModel.count; i++) {
                if (listModel.get(i).cidr === cidr) {
                    root.duplicateDetected(qsTr("子网代理 CIDR 已存在"))
                    return
                }
            }

            listModel.append({
                cidr: cidr,
                mappedCidr: mappedCidrField.text.trim(),
                allow: allow
            })
            root.changed()
            close()
        }

        ColumnLayout {
            width: parent ? parent.width : 360
            spacing: 8

            SwbLabel { text: qsTr("CIDR（必填）") }

            SwbTextField {
                id: cidrField
                Layout.fillWidth: true
                placeholderText: "192.168.1.0/24"
                onAccepted: addDialog.acceptProxyNetwork()
            }

            SwbLabel { text: qsTr("映射网段（选填）") }

            SwbTextField {
                id: mappedCidrField
                Layout.fillWidth: true
                placeholderText: "192.168.10.0/24"
                onAccepted: addDialog.acceptProxyNetwork()
            }

            SwbLabel { text: qsTr("允许协议") }

            RowLayout {
                Layout.fillWidth: true
                SwbCheckBox { id: tcpCheck; text: "tcp"; checked: true }
                SwbCheckBox { id: udpCheck; text: "udp"; checked: true }
                SwbCheckBox { id: icmpCheck; text: "icmp"; checked: true }
            }
        }

        footer: DialogButtonBox {
            spacing: 8
            leftPadding: 16
            rightPadding: 16
            topPadding: 8
            bottomPadding: 12

            SwbButton {
                variant: "outline"
                text: qsTr("取消")
                onClicked: addDialog.close()
            }

            SwbButton {
                text: qsTr("添加")
                onClicked: addDialog.acceptProxyNetwork()
            }
        }

        onOpened: {
            cidrField.forceActiveFocus()
            cidrField.selectAll()
        }
    }
}
