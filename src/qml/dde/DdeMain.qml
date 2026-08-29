/* @file DdeMain.qml
 * @brief DDE（Deepin 桌面环境）模式应用主窗口：DTK ApplicationWindow + TitleBar，页面与组件复用 Main.qml 现有层
 *
 * 仅当 CMake BUILD_WITH_DDE=ON（QTET_WITH_DDE 宏）时由 main.cpp 加载。
 * 相对 Main.qml 的变化：
 *  - 根窗口改用 org.deepin.dtk.ApplicationWindow（DTK 标题栏、DTK 调色板、跟随 DDE 亮/暗主题）；
 *  - 页面容器、侧边栏、底部状态栏、错误弹窗逻辑与 Main.qml 逐条等价。
 */
import QtQuick
// QtQuick.Controls 以 QQC 别名引入，仅用 QQC.Label / QQC.Dialog / QQC.Overlay；
// 避免与 org.deepin.dtk 的 ApplicationWindow 产生同名类型歧义，
// 同时避免裸 Label 隐式依赖 dtk 的导出（无 dtk 的静态检查环境不可解析）。
import QtQuick.Controls as QQC
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

// 主窗口：DTK ApplicationWindow，自带标题栏与主题感知调色板
/* @brief 根窗口：管理全局布局、页面切换和错误提示 */
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

    // 窗口 flags：必须显式声明标题/最小化/最大化/关闭按钮 hints。
    // dtk WindowButtonGroup 以 Window.window.flags 的对应位作为各按钮可见性依据，
    // 而 QML Window 的默认 flags 不含 Min/Max/Close hints（会隐藏全部窗口按钮）。
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint

    // DTK 窗口样式：显式启用客户端侧装饰（DWindow 附加属性）。
    // 未启用时窗口仍由窗口管理器提供系统标题栏（黑条），与 dtk TitleBar 叠加，
    // 必须 enabled: true 才能接管装饰并呈现 DDE 风格圆角/阴影效果好窗口。
    DWindow.enabled: true
    // 圆角跟随系统主题规则（与 dtk 内部 Style/FlowStyle 判断一致：platformTheme.windowRadius < 0 时回退 12）
    DWindow.windowRadius: D.DTK.platformTheme.windowRadius < 0 ? 12 : D.DTK.platformTheme.windowRadius
    DWindow.shadowColor: Qt.rgba(0, 0, 0, 0.15)

    Theme { id: theme }

    // DDE 自绘标题栏：图标 + 标题 + 主题菜单 + 窗口按钮组（按钮按 flags 位自动显示）
    // 窗口图标：TitleBar.icon 为 DciIcon（按系统主题图标名查找），系统主题暂无 QtEasyTier
    // 图标，故通过 leftContent 自定义区展示应用内嵌图标（qrc:/icons/qtet.png）。
    header: Item {
        id: headerArea
        height: titleBar.height + 1

        TitleBar {
            id: titleBar
            title: root.title

            leftContent: Image {
                source: "qrc:/icons/qtet.png"
                sourceSize: Qt.size(16, 16)
                fillMode: Image.PreserveAspectFit
                smooth: true
            }
        }

        // 额头与主体内容之间的分割线（qtet：TitleBar 内置分隔线为透明色，此处补可见分割线；
        // 颜色与 Main.qml 的分隔线规则一致，随亮/暗主题自适应）
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

                QQC.Label {
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
    // Chameleon 风格未提供 Dialog 映射时回退 Qt 默认样式，功能与 Main.qml 等价。
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
