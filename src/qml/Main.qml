/* @brief 应用主窗口：QML 入口，负责窗口属性、侧边栏导航、页面容器切换和底部状态栏的整体布局 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtEasyTier

// 主窗口：整个应用的 QML 入口
// 负责窗口属性、侧边栏、页面容器和底部状态栏的基本布局组合
/* @brief 根窗口：管理全局布局、页面切换和错误提示 */
Window {
    id: root

    // 初始不显示，由 main.cpp 根据普通启动或 --autostart 决定是否展示窗口。
    visible: false
    width: 700
    height: 480
    minimumWidth: 500
    minimumHeight: 300
    title: qsTr("QtEasyTier")

    color: palette.window

    Theme { id: theme }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // 左侧导航侧边栏
            Sidebar {
                id: sidebar

                Layout.preferredWidth: 64
                Layout.fillHeight: true
                currentIndex: 0

                // 侧边栏项点击 → 同步切换页面容器
                onItemClicked: function (index) {
                    sidebar.currentIndex = index;
                    pageContainer.currentIndex = index;
                }
            }

            // 右侧页面容器：封装页面切换动画
            PageContainer {
                id: pageContainer

                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: 0
            }
        }

        // 分隔线
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.15)
        }

        // 底部状态栏：显示后端连接状态
        Rectangle {
            Layout.fillWidth: true
            height: 24
            color: palette.alternateBase

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 6

                // 状态指示灯
                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    // 根据连接状态显示不同颜色
                    color: BackendStatusViewModel.connected ? theme.statusGreen
                         : BackendStatusViewModel.connecting ? theme.statusOrange
                         : theme.statusRed
                }

                Label {
                    text: BackendStatusViewModel.statusText
                    font.pixelSize: 11
                    color: palette.windowText
                }

                // 弹簧占位，把后续元素推到右侧
                Item { Layout.fillWidth: true }
            }
        }
    }

    // 全局错误弹窗：由 AppState 的 errorOccurred 信号驱动
    MessageDialog {
        id: errorDialog
        title: qsTr("错误")
        buttons: MessageDialog.Ok
        text: ""
    }

    Connections {
        target: AppState
        function onErrorOccurred(message) {
            errorDialog.text = message
            errorDialog.open()
        }
    }
}
