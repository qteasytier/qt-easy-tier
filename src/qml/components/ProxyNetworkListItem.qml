/* @brief 代理子网列表组件：管理 cidr + mappedCidr + allow 列表，支持添加、删除和去重检测 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    property alias model: listModel

    signal changed()
    signal duplicateDetected(string msg)

    spacing: 4

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
        Layout.preferredHeight: listModel.count === 0 ? 0 : listModel.count * 44 + 4
        spacing: 2
        model: listModel
        interactive: false

        delegate: Rectangle {
            required property int index
            required property var model

            width: ListView.view.width
            height: 42
            color: index % 2 === 0 ? "transparent"
                                   : Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.04)
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 4
                spacing: 4

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Label {
                        Layout.fillWidth: true
                        text: model.mappedCidr ? model.cidr + " -> " + model.mappedCidr : model.cidr
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("协议: ") + root.allowText(model.allow)
                        color: palette.placeholderText
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                }

                ToolButton {
                    icon.source: "qrc:/icons/delete.svg"
                    icon.color: "transparent"
                    icon.width: 14
                    icon.height: 14
                    padding: 0
                    flat: true
                    implicitWidth: 28
                    implicitHeight: 28
                    onClicked: {
                        listModel.remove(index)
                        root.changed()
                    }
                }
            }
        }
    }

    Button {
        id: addButton
        Layout.fillWidth: true
        flat: true
        text: qsTr("添加代理子网")
        contentItem: Label {
            text: addButton.text
            color: palette.highlight
            renderType: Text.NativeRendering
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        onClicked: {
            addDialog.cidrText = ""
            addDialog.mappedCidrText = ""
            tcpCheck.checked = true
            udpCheck.checked = true
            icmpCheck.checked = true
            addDialog.open()
        }
    }

    Dialog {
        id: addDialog
        title: qsTr("添加代理子网")
        modal: true
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

            Label { text: qsTr("CIDR（必填）") }

            TextField {
                id: cidrField
                Layout.fillWidth: true
                placeholderText: "192.168.1.0/24"
                onAccepted: addDialog.acceptProxyNetwork()
            }

            Label { text: qsTr("映射网段（选填）") }

            TextField {
                id: mappedCidrField
                Layout.fillWidth: true
                placeholderText: "192.168.10.0/24"
                onAccepted: addDialog.acceptProxyNetwork()
            }

            Label { text: qsTr("允许协议") }

            RowLayout {
                Layout.fillWidth: true
                CheckBox { id: tcpCheck; text: "tcp"; checked: true }
                CheckBox { id: udpCheck; text: "udp"; checked: true }
                CheckBox { id: icmpCheck; text: "icmp"; checked: true }
            }
        }

        footer: DialogButtonBox {
            spacing: 8
            leftPadding: 16
            rightPadding: 16
            topPadding: 8
            bottomPadding: 12

            Button {
                text: qsTr("取消")
                onClicked: addDialog.close()
            }

            Button {
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
