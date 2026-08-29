/* @file DdeMain.qml
 * @brief DDE 模式应用主窗口（BUILD_WITH_DDE=ON 时由 main.cpp 加载）：DTK ApplicationWindow + TitleBar
 *
 * 相对 Main.qml：根窗口与标题栏走 DTK；页面容器、侧边栏、状态栏、错误弹窗逻辑等价。
 */
import QtQuick
// QtQuick.Controls 仅以 QQC 别名限量使用（避免与 dtk 的 ApplicationWindow/Label 同名歧义）
import QtQuick.Controls as QQC
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk
import org.deepin.dtk 1.0 as D

ApplicationWindow {
    id: root

    // 初始不显示，由 main.cpp 根据普通启动或 --autostart 决定是否展示窗口。
    visible: false
    width: 700
    height: 480
    minimumWidth: 500
    minimumHeight: 300
    title: qsTr("QtEasyTier")

    color: palette.window

    // dtk WindowButtonGroup 按 Window.flags 对应位决定窗口按钮可见性，需显式声明
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint

    // 启用 DTK 客户端侧装饰（否则窗口管理器提供系统标题栏，与 dtk TitleBar 叠加）
    DWindow.enabled: true
    // 圆角随系统主题（< 0 时取 dtk 默认 12）
    DWindow.windowRadius: D.DTK.platformTheme.windowRadius < 0 ? 12 : D.DTK.platformTheme.windowRadius
    DWindow.shadowColor: Qt.rgba(0, 0, 0, 0.15)

    Theme { id: theme }

    header: Item {
        id: headerArea
        height: titleBar.height + 1

        TitleBar {
            id: titleBar
            title: root.title

            // 标题栏抽屉菜单：主题/帮助/关于/退出
            menu: Menu {
                ThemeMenu { }

                MenuSeparator { }

                MenuItem {
                    text: qsTr("帮助")
                    onTriggered: Qt.openUrlExternally("https://qtet.cn/docs-home")
                }

                AboutAction {
                    text: qsTr("关于")
                    aboutDialog: AppAboutDialog { }
                }

                MenuSeparator { }

                QuitAction { }
            }

            // TitleBar.icon 为 DciIcon（按主题图标名查找），系统主题无 QtEasyTier 图标，
            // 故用 leftContent 展示应用内嵌图标
            leftContent: Image {
                source: "qrc:/icons/qtet.png"
                sourceSize: Qt.size(24, 24)
                fillMode: Image.PreserveAspectFit
                smooth: true
            }
        }

        // 额头与内容之间的分割线（TitleBar 内置分隔线为透明色，颜色规则同 Main.qml）
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.15)
        }
    }

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
                    color: BackendStatusViewModel.connected ? theme.statusGreen
                         : BackendStatusViewModel.connecting ? theme.statusOrange
                         : theme.statusRed
                }

                QQC.Label {
                    text: BackendStatusViewModel.statusText
                    font.pixelSize: 11
                    color: palette.windowText
                }

                Item { Layout.fillWidth: true }
            }
        }
    }

    // 全局错误弹窗：由 AppState 的 errorOccurred 信号驱动，逻辑与 Main.qml 等价
    QQC.Dialog {
        id: errorDialog
        title: qsTr("错误")
        modal: true
        parent: QQC.Overlay.overlay
        anchors.centerIn: parent
        standardButtons: QQC.Dialog.Ok
        width: Math.min(420, parent ? parent.width - 48 : 360)

        property string text: ""

        QQC.Label {
            text: errorDialog.text
            wrapMode: Text.WordWrap
            width: parent ? parent.width : 360
        }
    }

    Connections {
        target: AppState
        function onErrorOccurred(message) {
            errorDialog.text = message
            errorDialog.open()
        }
    }
}
