/* @brief 代理子网列表组件：管理 cidr + mappedCidr + allow 列表（allow 以逗号字符串角色存储），支持添加、编辑、删除和去重检测，Swb 控件迁移版 */
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
    Theme { id: appTheme }

    function allProtocols() {
        return ["tcp", "udp", "icmp"]
    }

    // allow 在 ListModel 中以逗号字符串存储（Qt6 ListModel 的数组角色 get/setProperty 读写不可靠），兼容数组入参
    function normalizedAllow(allow) {
        var list = Array.isArray(allow) ? allow : (allow ? String(allow).split(",") : [])

        var result = []
        for (var i = 0; i < list.length; i++) {
            var protocol = list[i]
            if ((protocol === "tcp" || protocol === "udp" || protocol === "icmp")
                    && result.indexOf(protocol) === -1) {
                result.push(protocol)
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
                    iconSource: "qrc:/icons/edit.svg"
                    flat: true
                    onClicked: addDialog.openForEdit(proxyRow.index)
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
        textColor: appTheme.statusGreen
        onClicked: addDialog.openForAdd()
    }

    // 添加/编辑代理子网对话框（editingIndex 区分模式：-1 添加，>=0 编辑）
    SwbDialog {
        id: addDialog
        title: editingIndex >= 0 ? qsTr("编辑代理子网") : qsTr("添加代理子网")
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(420, parent ? parent.width - 48 : 360)

        // 当前对话框模式：-1 添加，>=0 为正在编辑的列表项索引
        property int editingIndex: -1

        property alias cidrText: cidrField.text
        property alias mappedCidrText: mappedCidrField.text

        // 以添加模式打开：清空输入并勾选全部协议
        function openForAdd() {
            editingIndex = -1
            cidrText = ""
            mappedCidrText = ""
            tcpCheck.checked = true
            udpCheck.checked = true
            icmpCheck.checked = true
            open()
        }

        // 以编辑模式打开：预填当前项的网段与协议勾选
        function openForEdit(idx) {
            editingIndex = idx
            var item = listModel.get(idx)
            cidrText = item.cidr
            mappedCidrText = item.mappedCidr || ""
            var allow = root.normalizedAllow(item.allow)
            tcpCheck.checked = allow.indexOf("tcp") !== -1
            udpCheck.checked = allow.indexOf("udp") !== -1
            icmpCheck.checked = allow.indexOf("icmp") !== -1
            open()
        }

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
                if (i !== editingIndex && listModel.get(i).cidr === cidr) {
                    root.duplicateDetected(qsTr("子网代理 CIDR 已存在"))
                    return
                }
            }

            if (editingIndex >= 0) {
                listModel.set(editingIndex, {
                    cidr: cidr,
                    mappedCidr: mappedCidrField.text.trim(),
                    allow: allow.join(",")
                })
                editingIndex = -1
            } else {
                listModel.append({
                    cidr: cidr,
                    mappedCidr: mappedCidrField.text.trim(),
                    allow: allow.join(",")
                })
            }
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
                text: addDialog.editingIndex >= 0 ? qsTr("保存") : qsTr("添加")
                onClicked: addDialog.acceptProxyNetwork()
            }
        }

        onOpened: {
            cidrField.forceActiveFocus()
            cidrField.selectAll()
        }
    }
}
