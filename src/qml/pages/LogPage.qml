/* @brief 运行日志页面：展示应用运行日志，支持按级别筛选（全部/信息/警告/错误）和清空操作 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtEasyTier

/* @brief 日志页面根容器，包含筛选栏和日志列表 */
Rectangle {
    id: root

    color: palette.window
    /* 当前日志级别筛选条件：all/info/warning/error */
    property string levelFilter: "all"

    /* 分隔线颜色 */
    readonly property color dividerColor: Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.12)
    /* 未选中筛选按钮的柔和背景色 */
    readonly property color mutedPanelColor: Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.045)

    /* 根据日志级别字符串返回对应的主题颜色 */
    function levelColor(level) {
        if (level === "error")
            return theme.statusRed
        if (level === "warning")
            return theme.statusOrange
        return theme.statusGreen
    }

    /* 根据日志级别字符串返回对应的中文显示文本 */
    function levelText(level) {
        if (level === "error")
            return qsTr("错误")
        if (level === "warning")
            return qsTr("警告")
        return qsTr("信息")
    }

    Theme { id: theme }

    // 页面加载时从数据库读取日志
    Component.onCompleted: LogViewModel.loadLogs()

    // 主布局
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        // 页面标题
        Label {
            Layout.fillWidth: true
            text: qsTr("运行日志")
            font.bold: true
            font.pixelSize: 24
            color: palette.highlight
        }

        // 日志筛选工具栏
        Card {
            Layout.fillWidth: true

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                // 级别筛选按钮组：全部/信息/警告/错误
                Repeater {
                    model: [
                        { label: qsTr("全部"), level: "all", color: palette.highlight },
                        { label: qsTr("信息"), level: "info", color: theme.statusGreen },
                        { label: qsTr("警告"), level: "warning", color: theme.statusOrange },
                        { label: qsTr("错误"), level: "error", color: theme.statusRed }
                    ]

                    delegate: Rectangle {
                        id: filterChip

                        property bool selected: root.levelFilter === modelData.level

                        Layout.preferredWidth: chipText.implicitWidth + 26
                        Layout.preferredHeight: chipText.implicitHeight + 10
                        radius: height / 2
                        color: selected ? Qt.rgba(modelData.color.r, modelData.color.g, modelData.color.b, 0.2) : root.mutedPanelColor
                        border.color: selected ? modelData.color : root.dividerColor
                        border.width: 1

                        Label {
                            id: chipText
                            anchors.centerIn: parent
                            text: modelData.label
                            font.bold: filterChip.selected
                            color: filterChip.selected ? modelData.color : palette.windowText
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.levelFilter = modelData.level
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                Label {
                    text: qsTr("已保存 %1 / %2").arg(LogViewModel.count).arg(SettingsViewModel.maxLogEntries)
                    font.pixelSize: FontHelper.smallFont.pixelSize
                    font.bold: true
                    color: Qt.rgba(palette.highlight.r, palette.highlight.g, palette.highlight.b, 1.0)
                }

                Button {
                    text: qsTr("清空日志")
                    flat: true
                    enabled: LogViewModel.count > 0
                    onClicked: LogViewModel.clearLogs()
                }
            }
        }

        // 日志内容区
        Card {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentSpacing: 0

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // 无日志时的空状态提示
                ColumnLayout {
                    visible: LogViewModel.count === 0
                    anchors.centerIn: parent
                    width: Math.min(parent.width, 360)
                    spacing: 10

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("没有可显示的日志")
                        font.bold: true
                        color: palette.windowText
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("当应用产生达到当前输出等级的日志时，会自动出现在这里。")
                        font: FontHelper.smallFont
                        color: palette.placeholderText
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }
                }

                // 日志列表视图
                ListView {
                    id: logListView

                    visible: LogViewModel.count > 0
                    anchors.fill: parent
                    clip: true
                    model: LogViewModel

                    // 单条日志项
                    delegate: ColumnLayout {
                        id: logEntry

                        required property string timestamp
                        required property string level
                        required property string message

                        readonly property bool matchesFilter: root.levelFilter === "all" || root.levelFilter === level

                        width: logListView.width
                        height: matchesFilter ? implicitHeight : 0
                        visible: matchesFilter
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Rectangle {
                                Layout.preferredWidth: entryLevelText.implicitWidth + 26
                                Layout.preferredHeight: entryLevelText.implicitHeight + 10
                                radius: height / 2
                                color: Qt.rgba(root.levelColor(level).r, root.levelColor(level).g, root.levelColor(level).b, 0.16)
                                border.color: root.levelColor(level)
                                border.width: 1

                                Label {
                                    id: entryLevelText
                                    anchors.centerIn: parent
                                    text: root.levelText(level)
                                    font.pixelSize: FontHelper.smallFont.pixelSize
                                    font.bold: true
                                    color: root.levelColor(level)
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                text: "[" + timestamp + "]"
                                font.pixelSize: FontHelper.smallFont.pixelSize
                                font.bold: true
                                color: root.levelColor(level)
                                elide: Text.ElideRight
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: message
                            color: palette.windowText
                            horizontalAlignment: Text.AlignLeft
                            wrapMode: Text.WrapAnywhere
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: root.dividerColor
                        }

                        Item { Layout.preferredHeight: 4 }
                    }
                }
            }
        }
    }
}
