/* @brief 页面容器组件：通过懒加载管理页面生命周期，并在切换时仅保留当前页实例 */
import QtQuick
import QtEasyTier

// 页面容器：通过单个 Loader 控制当前页的创建与销毁
// 切换到别的页时立即释放旧页实例，当前页进入时保留轻量的位移与缩放动画
Item {
    id: root

    /* 当前页面索引：0=网络 1=节点收藏 2=日志 3=设置 */
    property int currentIndex: 0
    readonly property int slideOffset: 36
    readonly property real inactiveScale: 0.985

    function pageComponent(pageIndex) {
        switch (pageIndex) {
        case 0:
            return networkPageComponent
        case 1:
            return favoriteNodesPageComponent
        case 2:
            return logPageComponent
        case 3:
            return settingsPageComponent
        default:
            return null
        }
    }

    Component {
        id: networkPageComponent
        NetworkPage {
            anchors.fill: parent
        }
    }

    Component {
        id: favoriteNodesPageComponent
        FavoriteNodesPage {
            anchors.fill: parent
        }
    }

    Component {
        id: logPageComponent
        LogPage {
            anchors.fill: parent
        }
    }

    Component {
        id: settingsPageComponent
        SettingsPage {
            anchors.fill: parent
        }
    }

    Loader {
        id: pageLoader
        anchors.fill: parent
        sourceComponent: root.pageComponent(root.currentIndex)
        opacity: 0
        scale: root.inactiveScale
        transformOrigin: Item.Center
        transform: Translate {
            id: pageOffset
            y: root.slideOffset
        }

        onSourceComponentChanged: {
            opacity = 0
            scale = root.inactiveScale
            pageOffset.y = root.slideOffset
        }

        onLoaded: enterAnimation.restart()
    }

    SequentialAnimation {
        id: enterAnimation

        ScriptAction {
            script: {
                pageLoader.opacity = 0
                pageLoader.scale = root.inactiveScale
                pageOffset.y = root.slideOffset
            }
        }

        ParallelAnimation {
            NumberAnimation {
                target: pageLoader
                property: "opacity"
                to: 1
                duration: 160
                easing.type: Easing.OutQuad
            }

            NumberAnimation {
                target: pageLoader
                property: "scale"
                to: 1
                duration: 260
                easing.type: Easing.OutCubic
            }

            NumberAnimation {
                target: pageOffset
                property: "y"
                to: 0
                duration: 270
                easing.type: Easing.OutBack
                easing.overshoot: 0.55
            }
        }
    }
}
