/* @brief 侧边栏导航组件：垂直排列的导航项列表，通过 currentIndex 控制选中项，右侧有分隔线 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 侧边栏导航：垂直排列的导航项列表，右侧有分隔线
// 通过 currentIndex 控制选中项，itemClicked 信号通知外部切换页面
/* @brief 侧边栏根容器，包含导航项列表和右侧分隔线 */
Rectangle {
    id: root

    color: palette.alternateBase

    /* 当前选中项的索引，0=网络配置 1=节点收藏 2=日志 3=设置 */
    property int currentIndex: 0

    /* 导航项被点击时发出，外部按索引切换对应页面 */
    signal itemClicked(int index)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 2

        Repeater {
            id: repeater
            model: ListModel {
                ListElement { icon: "qrc:/icons/net-page.svg"; label: "网络配置"; page: "network" }
                ListElement { icon: "qrc:/icons/favorites.svg"; label: "节点收藏"; page: "favoriteNodes" }
                ListElement { icon: "qrc:/icons/log.svg"; label: "日志"; page: "logs" }
                ListElement { icon: "qrc:/icons/settings.svg"; label: "设置"; page: "settings" }
            }

            delegate: SidebarItem {
                Layout.fillWidth: true
                icon: model.icon
                label: model.label
                selected: root.currentIndex === index
                onClicked: root.itemClicked(index)
            }
        }

        // 弹簧占位，把导航项推到顶部
        Item {
            Layout.fillHeight: true
        }
    }

    // 右侧分隔线
    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 1
        color: Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.12)
    }
}
