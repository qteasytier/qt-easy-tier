/* @brief 侧边栏导航组件：参照 SWB-QML-UI 示例的图标 rail 风格，SwbToolButton 导航项 + 底部语言/明暗主题切换 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import SwbControls

// 侧边栏导航：64px 纯图标 rail（示例项目模式）
// 选中态由 SwbToolButton 的 checked 背景块表达，悬停提示由 SwbToolTip 提供
// 通过 currentIndex 控制选中项，itemClicked 信号通知外部切换页面
/* @brief 侧边栏根容器：应用标识、导航按钮组、弹簧与语言/主题切换按钮 */
Rectangle {
    id: root

    color: SwbTheme.background

    /* 当前选中项的索引，0=网络配置 1=节点收藏 2=日志 3=设置 */
    property int currentIndex: 0

    /*
      导航项清单：label 用 qsTr 字面量（可被 lupdate 收割），
      engine.retranslate() 会让数组绑定重求值实现语言即时切换。
    */
    readonly property var navItems: [
        { iconSource: "qrc:/icons/net-page.svg", label: qsTr("网络配置") },
        { iconSource: "qrc:/icons/favorites.svg", label: qsTr("节点收藏") },
        { iconSource: "qrc:/icons/log.svg", label: qsTr("日志") },
        { iconSource: "qrc:/icons/settings.svg", label: qsTr("设置") }
    ]

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
            model: root.navItems

            delegate: SwbToolButton {
                id: navButton

                /* 模型角色：图标资源与提示文字 */
                required property var modelData
                required property int index

                Layout.alignment: Qt.AlignHCenter
                size: "lg"

                icon.source: modelData.iconSource
                icon.width: root.navIconSize
                icon.height: root.navIconSize
                icon.color: navButton.checked ? SwbTheme.accentForeground : SwbTheme.mutedForeground

                checked: root.currentIndex === navButton.index
                onClicked: root.itemClicked(navButton.index)

                SwbToolTip {
                    text: navButton.modelData.label
                    visible: navButton.hovered
                }
            }
        }

        // 弹簧占位，把语言/主题切换按钮推到底部
        Item { Layout.fillHeight: true }

        // 语言切换：点击弹出菜单，四选一（跟随系统/简体/繁體/English），
        // 当前语言以 "✓" 后缀标记；写入经 LanguageController→SettingsViewModel 持久化，
        // 生效（换翻译器 + retranslate）由控制器监听设置变化统一完成。
        SwbToolButton {
            id: languageButton

            Layout.alignment: Qt.AlignHCenter
            size: "lg"

            icon.source: "qrc:/icons/language.svg"
            icon.width: root.navIconSize
            icon.height: root.navIconSize
            icon.color: SwbTheme.mutedForeground

            onClicked: languageMenu.popup()

            SwbToolTip {
                text: qsTr("语言")
                visible: languageButton.hovered
            }
        }

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

    // 语言选择菜单：条目来自 LanguageController.availableLanguages（label 随翻译更新），
    // 不用 MenuItem 的 checkable（交互会打断 checked 绑定），改以文本 ✓ 后缀标记当前项
    SwbMenu {
        id: languageMenu
        width: 170

        Repeater {
            model: LanguageController.availableLanguages

            delegate: SwbMenuItem {
                id: langItem

                required property var modelData

                text: modelData.label + (LanguageController.language === modelData.value ? "  ✓" : "")
                onTriggered: {
                    LanguageController.setLanguage(modelData.value)
                    languageMenu.close()
                }
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
