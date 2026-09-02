/* @brief 自定义 Tab 控件：基于 SwbTabBar（line 变体）+ StackLayout 内容区，子组件通过 tabTitle 属性自动注册 */
import QtQuick
import QtQuick.Layouts
import SwbControls

// 自定义 Tab 控件：SwbTabBar 下划线变体 + StackLayout 内容区
// 子组件通过 tabTitle 属性自动注册为标签页
/* @brief Tab 控件根布局，包含标签栏、分隔线和内容区 */
ColumnLayout {
    id: root

    /* 当前激活的标签页索引 */
    property int currentIndex: 0
    /* 默认内容容器：子组件直接放入 StackLayout，通过 tabTitle 属性注册为标签页 */
    default property alias contentData: stack.data
    spacing: 0

    // 标签栏区域；与 root.currentIndex 双向同步（带防回环守卫）
    SwbTabBar {
        id: tabBar
        Layout.fillWidth: true
        variant: "line"

        onCurrentIndexChanged: {
            if (currentIndex !== root.currentIndex)
                root.currentIndex = currentIndex
        }

        Repeater {
            model: tabsModel

            delegate: SwbTabButton {
                text: model.title
                width: Math.max(72, implicitWidth)
            }
        }
    }

    onCurrentIndexChanged: {
        if (tabBar.currentIndex !== currentIndex)
            tabBar.currentIndex = currentIndex
    }

    // 标签栏底部分隔线
    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: SwbTheme.border
    }

    // 内容区：通过 StackLayout 按索引切换
    StackLayout {
        id: stack
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: root.currentIndex
    }

    // 标签标题模型
    ListModel {
        id: tabsModel
    }

    // 组件完成时，自动收集子组件的 tabTitle 属性注册为标签
    Component.onCompleted: {
        for (var i = 0; i < stack.children.length; i++) {
            var child = stack.children[i]
            if (child && child.tabTitle !== undefined)
                tabsModel.append({ title: child.tabTitle })
        }
    }
}
