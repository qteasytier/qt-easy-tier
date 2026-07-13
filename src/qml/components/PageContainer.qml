/* @brief 页面容器组件：管理多个页面的生命周期与切换动画，通过统一下沉实现淡入淡出 + 纵向滑入滑出 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier

// 页面容器：管理多个页面的生命周期与切换动画
// 页面位置统一采用下沉方向：当前页归位，非当前页都停在下方等待，切换时形成统一的向下收束感
// 当前页面轻微放大归位，配合进入页的低强度回弹，让切换更灵动但不抢注意力
/* @brief 页面容器根节点，持有四个页面并通过 currentIndex 切换 */
Item {
    id: root

    /* 当前页面索引：0=网络 1=节点收藏 2=日志 3=设置 */
    property int currentIndex: 0
    readonly property int slideOffset: 36
    readonly property real inactiveScale: 0.985

    function pageOpacity(pageIndex) {
        return currentIndex === pageIndex ? 1 : 0
    }

    function pageY(pageIndex) {
        if (currentIndex === pageIndex)
            return 0
        return slideOffset
    }

    function pageScale(pageIndex) {
        return currentIndex === pageIndex ? 1 : inactiveScale
    }

    NetworkPage {
        id: networkPage

        anchors.fill: parent
        opacity: root.pageOpacity(0)
        scale: root.pageScale(0)
        transformOrigin: Item.Center
        transform: Translate {
            y: root.pageY(0)

            Behavior on y {
                enabled: root.currentIndex === 0 || networkPage.opacity > 0.01
                NumberAnimation {
                    duration: root.currentIndex === 0 ? 270 : 190
                    easing.type: root.currentIndex === 0 ? Easing.OutBack : Easing.InCubic
                    easing.overshoot: root.currentIndex === 0 ? 0.55 : 0
                }
            }
        }
        // 通过 opacity 控制可见性，避免不可见页面接收事件
        visible: opacity > 0.01

        Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutQuad } }
        Behavior on scale { NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }
    }

    FavoriteNodesPage {
        id: favoriteNodesPage

        anchors.fill: parent
        opacity: root.pageOpacity(1)
        scale: root.pageScale(1)
        transformOrigin: Item.Center
        transform: Translate {
            y: root.pageY(1)

            Behavior on y {
                enabled: root.currentIndex === 1 || favoriteNodesPage.opacity > 0.01
                NumberAnimation {
                    duration: root.currentIndex === 1 ? 270 : 190
                    easing.type: root.currentIndex === 1 ? Easing.OutBack : Easing.InCubic
                    easing.overshoot: root.currentIndex === 1 ? 0.55 : 0
                }
            }
        }
        visible: opacity > 0.01

        Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutQuad } }
        Behavior on scale { NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }
    }

    LogPage {
        id: logPage

        anchors.fill: parent
        opacity: root.pageOpacity(2)
        scale: root.pageScale(2)
        transformOrigin: Item.Center
        transform: Translate {
            y: root.pageY(2)

            Behavior on y {
                enabled: root.currentIndex === 2 || logPage.opacity > 0.01
                NumberAnimation {
                    duration: root.currentIndex === 2 ? 270 : 190
                    easing.type: root.currentIndex === 2 ? Easing.OutBack : Easing.InCubic
                    easing.overshoot: root.currentIndex === 2 ? 0.55 : 0
                }
            }
        }
        visible: opacity > 0.01

        Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutQuad } }
        Behavior on scale { NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }
    }

    SettingsPage {
        id: settingsPage

        anchors.fill: parent
        opacity: root.pageOpacity(3)
        scale: root.pageScale(3)
        transformOrigin: Item.Center
        transform: Translate {
            y: root.pageY(3)

            Behavior on y {
                enabled: root.currentIndex === 3 || settingsPage.opacity > 0.01
                NumberAnimation {
                    duration: root.currentIndex === 3 ? 270 : 190
                    easing.type: root.currentIndex === 3 ? Easing.OutBack : Easing.InCubic
                    easing.overshoot: root.currentIndex === 3 ? 0.55 : 0
                }
            }
        }
        visible: opacity > 0.01

        Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutQuad } }
        Behavior on scale { NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }
    }
}
