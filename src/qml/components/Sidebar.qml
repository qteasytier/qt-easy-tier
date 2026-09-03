/* @brief 侧边栏导航组件：参照 SWB-QML-UI 示例的图标 rail 风格，SwbToolButton 导航项 + 底部明暗主题切换 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import SwbControls

// 侧边栏导航：64px 纯图标 rail（示例项目模式）
// 选中态由 SwbToolButton 的 checked 背景块表达，悬停提示由 SwbToolTip 提供
// 通过 currentIndex 控制选中项，itemClicked 信号通知外部切换页面
/* @brief 侧边栏根容器：应用标识、导航按钮组、弹簧与主题切换按钮 */
Rectangle {
    id: root

    color: SwbTheme.background

    /* 当前选中项的索引，0=网络配置 1=节点收藏 2=日志 3=设置 */
    property int currentIndex: 0

    /*
      主题模式：auto=跟随系统（默认）/ light / dark。
      数据源为 SettingsViewModel.themeMode（持久化到 settings3.json，启动时自动恢复）；
      切换按钮只写 ViewModel，经 NOTIFY 回推本属性后统一在 onThemeModeChanged 生效。
      SwbTheme.darkMode 默认绑定 Application.styleHints.colorScheme，系统深浅色
      变化会实时生效；但手动赋值会打断该绑定，故仅显式选择时覆盖，
      切回 auto 时通过 applyThemeMode() 重建响应式绑定恢复跟随。
    */
    property string themeMode: SettingsViewModel.themeMode
    onThemeModeChanged: applyThemeMode()
    Component.onCompleted: applyThemeMode()

    /* 导航项被点击时发出，外部按索引切换对应页面 */
    signal itemClicked(int index)

    /* 图标边长：主题 token 基础上加 6，配合 lg 按钮尺寸 */
    readonly property int navIconSize: SwbTheme.iconSize + 6

    /* 应用当前主题模式：auto 时重建对系统 colorScheme 的绑定，否则固定覆盖 */
    function applyThemeMode() {
        if (themeMode === "auto") {
            SwbTheme.darkMode = Qt.binding(function() {
                return Application.styleHints.colorScheme === Qt.Dark
            })
        } else {
            SwbTheme.darkMode = (themeMode === "dark")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 14
        anchors.bottomMargin: 14
        spacing: 10

        // 导航按钮组：纯图标 + 悬停提示，选中项图标点亮
        Repeater {
            model: ListModel {
                ListElement { iconSource: "qrc:/icons/net-page.svg"; label: "网络配置" }
                ListElement { iconSource: "qrc:/icons/favorites.svg"; label: "节点收藏" }
                ListElement { iconSource: "qrc:/icons/log.svg"; label: "日志" }
                ListElement { iconSource: "qrc:/icons/settings.svg"; label: "设置" }
            }

            delegate: SwbToolButton {
                id: navButton

                /* 模型角色：图标资源与提示文字（role 不能叫 icon，会与按钮自身的 icon 组属性冲突） */
                required property string iconSource
                required property string label
                required property int index

                Layout.alignment: Qt.AlignHCenter
                size: "lg"

                icon.source: navButton.iconSource
                icon.width: root.navIconSize
                icon.height: root.navIconSize
                icon.color: navButton.checked ? SwbTheme.accentForeground : SwbTheme.mutedForeground

                checked: root.currentIndex === navButton.index
                onClicked: root.itemClicked(navButton.index)

                SwbToolTip {
                    text: navButton.label
                    visible: navButton.hovered
                }
            }
        }

        // 弹簧占位，把主题切换按钮推到底部
        Item { Layout.fillHeight: true }

        // 明暗主题切换：auto → light → dark 循环；auto 跟随系统深浅色并实时变化。
        // auto 模式以大写字母 A 作为图标（不使用 svg），light/dark 分别为太阳/月亮；
        // 图标源与文字互斥，SwbIconLabel 只渲染存在的一方，无须 display 切换。
        SwbToolButton {
            id: themeButton

            Layout.alignment: Qt.AlignHCenter
            size: "lg"

            icon.source: root.themeMode === "auto" ? ""
                         : (SwbTheme.darkMode ? "qrc:/swb/theme/moon.svg"
                                             : "qrc:/swb/theme/sun.svg")
            icon.width: root.navIconSize
            icon.height: root.navIconSize
            icon.color: SwbTheme.mutedForeground

            text: root.themeMode === "auto" ? "A" : ""
            textColor: SwbTheme.mutedForeground
            font.pixelSize: 18
            font.bold: true

            onClicked: {
                // 三态循环写入 ViewModel：持久化 + NOTIFY 回推 themeMode → applyThemeMode
                SettingsViewModel.themeMode = root.themeMode === "auto" ? "light"
                                           : root.themeMode === "light" ? "dark"
                                           : "auto"
            }

            SwbToolTip {
                text: root.themeMode === "auto" ? qsTr("主题：跟随系统")
                     : root.themeMode === "light" ? qsTr("主题：亮色")
                     : qsTr("主题：暗色")
                visible: themeButton.hovered
            }
        }
    }

    // 右侧分隔线
    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 1
        color: SwbTheme.border
    }
}
