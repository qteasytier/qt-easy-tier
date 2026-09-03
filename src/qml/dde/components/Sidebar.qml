/* @file Sidebar.qml (DDE)
 * @brief DDE 版侧边栏导航：DTK 图标 rail + 底部语言切换按钮
 *
 * 与共享版导航结构等价（itemClicked 信号切换页面）。DDE 主题由标题栏
 * ThemeMenu 接管（themeMode 无侧边栏入口）；语言按钮经 LanguageController
 * 持久化并即时重译。
 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

/* @brief 侧边栏根容器：导航按钮组、弹簧与语言切换按钮 */
Rectangle {
    id: root

    color: palette.window

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

    /* 导航项被点击时发出，外部按索引切换对应页面 */
    signal itemClicked(int index)

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 14
        anchors.bottomMargin: 14
        spacing: 10

        // 导航按钮组：纯图标 + 悬停提示，选中态由 ToolButton checked 背景表达
        Repeater {
            model: root.navItems

            delegate: IconToolButton {
                id: navButton

                /* 模型角色：图标资源与提示文字 */
                required property var modelData
                required property int index

                Layout.alignment: Qt.AlignHCenter
                buttonSize: 40
                iconSize: 22

                iconSource: navButton.modelData.iconSource
                checked: root.currentIndex === navButton.index
                onClicked: root.itemClicked(navButton.index)

                ToolTip {
                    parent: navButton
                    x: navButton.width + 8
                    y: (navButton.height - height) / 2
                    text: navButton.modelData.label
                    visible: navButton.hovered
                }
            }
        }

        // 弹簧占位，把语言切换按钮推到底部
        Item { Layout.fillHeight: true }

        // 语言切换：点击弹出菜单，四选一（跟随系统/简体/繁體/English）；
        // 写入经 LanguageController→SettingsViewModel 持久化，
        // 生效（换翻译器 + retranslate）由控制器监听设置变化统一完成。
        IconToolButton {
            id: languageButton

            Layout.alignment: Qt.AlignHCenter
            buttonSize: 40
            iconSize: 22
            iconSource: "qrc:/icons/language.svg"

            onClicked: {
                // 菜单以侧边栏为定位父项，弹出在按钮右侧
                var pos = languageButton.mapToItem(root, languageButton.width, 0)
                languageMenu.x = pos.x
                languageMenu.y = pos.y
                languageMenu.popup()
            }

            ToolTip {
                parent: languageButton
                x: languageButton.width + 8
                y: (languageButton.height - height) / 2
                text: qsTr("语言")
                visible: languageButton.hovered
            }
        }
    }

    // 语言选择菜单：条目来自 LanguageController.availableLanguages（label 随翻译更新），
    // 不用 MenuItem 的 checkable（交互会打断 checked 绑定），改以文本 ✓ 后缀标记当前项
    Menu {
        id: languageMenu
        width: 170

        Repeater {
            model: LanguageController.availableLanguages

            delegate: MenuItem {
                id: langItem

                required property var modelData

                text: langItem.modelData.label + (LanguageController.language === langItem.modelData.value ? "  ✓" : "")
                onTriggered: {
                    LanguageController.setLanguage(langItem.modelData.value)
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
        color: Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.15)
    }
}
