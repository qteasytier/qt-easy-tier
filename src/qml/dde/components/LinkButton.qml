/* @file LinkButton.qml (DDE)
 * @brief DDE 链接样式按钮：背景/边框透明、文字用高亮色；鼠标悬停时文字变淡并平滑过渡
 *
 * 用于列表项的"添加/导入"等弱化入口，视觉上区别于普通实底按钮。
 */
import QtQuick
import org.deepin.dtk

Button {
    id: root

    /* 文字颜色：默认高亮色；危险操作可覆写为红色（如 appTheme.statusRed） */
    property color textColor: palette.highlight

    /* 背景与边框透明（含悬停/按下态均无底色） */
    background: null

    HoverHandler { id: hover }

    contentItem: Label {
        text: root.text
        color: root.textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        /* 悬停反馈：文字淡至 70% 不透明度，离开时恢复 */
        opacity: hover.hovered ? 0.7 : 1.0
        Behavior on opacity {
            NumberAnimation { duration: 120 }
        }
    }
}
