import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Basic

// [QtEasyTier 补丁] 移除 QtQuick.VectorImage(.Helpers) 依赖与 CurveRenderer 渲染路径：
// 原实现仅在 Windows 且图标未着色时使用 VectorImage.CurveRenderer，其余平台本就走
// IconImage/IconLabel。QtQuick.VectorImage 为 addon 模块（Qt 6.8 tech preview），
// Debian 拆包为 qml6-module-qtquick-vectorimage(-helpers)，且 deepin v25 仓库仅提供
// libqt6quickvectorimage6 库包、未打包 QML 模块，运行时 import 必然失败。
// 裁剪后 SVG 一律经 IconImage 按 DPR 栅格化渲染，对常规图标尺寸无可感知差异。
// IconLabel-compatible content item that keeps text native while rendering
// SVG icons at native pixel density.
Item {
    id: root

    property var icon
    property string text: ""
    property font font
    property color color: "black"
    property int display: AbstractButton.TextBesideIcon
    property int alignment: Qt.AlignCenter
    property bool mirrored: false
    property real spacing: 0

    readonly property string iconUrl: icon && icon.source ? icon.source.toString() : ""
    readonly property bool hasIconSource: iconUrl.length > 0
    readonly property bool isSvgSource: {
        const normalizedUrl = iconUrl.toLowerCase()
        return normalizedUrl.endsWith(".svg") || normalizedUrl.endsWith(".svgz")
            || normalizedUrl.indexOf(".svg?") >= 0 || normalizedUrl.indexOf(".svg#") >= 0
            || normalizedUrl.indexOf(".svgz?") >= 0 || normalizedUrl.indexOf(".svgz#") >= 0
    }
    readonly property bool hasAbsoluteIconSource: iconUrl.startsWith("/")
                                                   || iconUrl.startsWith("\\")
                                                   || iconUrl.startsWith(":/")
                                                   || /^[a-z][a-z0-9+.-]*:/i.test(iconUrl)
    readonly property bool canRenderSvgDirectly: isSvgSource && hasAbsoluteIconSource
    readonly property bool hasIconName: !!(icon && icon.name && icon.name.length > 0)
    readonly property bool hasIcon: hasIconSource || hasIconName
    readonly property bool showIcon: hasIcon && display !== AbstractButton.TextOnly
    readonly property bool showText: text.length > 0 && display !== AbstractButton.IconOnly
    readonly property real iconWidth: showIcon && icon ? Math.max(0, icon.width) : 0
    readonly property real iconHeight: showIcon && icon ? Math.max(0, icon.height) : 0
    readonly property real textWidth: showText ? textLabel.implicitWidth : 0
    readonly property real textHeight: showText ? textLabel.implicitHeight : 0
    readonly property real groupWidth: {
        if (display === AbstractButton.TextBesideIcon)
            return iconWidth + textWidth + (showIcon && showText ? spacing : 0)
        if (display === AbstractButton.TextUnderIcon)
            return Math.max(iconWidth, textWidth)
        return showIcon ? iconWidth : textWidth
    }
    readonly property real groupHeight: {
        if (display === AbstractButton.TextUnderIcon)
            return iconHeight + textHeight + (showIcon && showText ? spacing : 0)
        return Math.max(iconHeight, textHeight)
    }
    // [QtEasyTier 补丁] hasExplicitIconColor/useCurveRenderer 随 CurveRenderer 路径一并移除
    readonly property real devicePixelRatio: Qt.application.screens.length > 0
                                             ? Qt.application.screens[0].devicePixelRatio
                                             : 1

    implicitWidth: groupWidth
    implicitHeight: groupHeight
    clip: true

    Item {
        id: group
        width: root.groupWidth
        height: root.groupHeight

        x: {
            if (root.alignment & Qt.AlignLeft)
                return 0
            if (root.alignment & Qt.AlignRight)
                return root.width - width
            return (root.width - width) / 2
        }
        y: {
            if (root.alignment & Qt.AlignTop)
                return 0
            if (root.alignment & Qt.AlignBottom)
                return root.height - height
            return (root.height - height) / 2
        }

        Item {
            id: iconItem
            width: root.iconWidth
            height: root.iconHeight
            x: {
                if (root.display === AbstractButton.TextBesideIcon)
                    return root.mirrored ? group.width - width : 0
                if (root.display === AbstractButton.TextUnderIcon)
                    return (group.width - width) / 2
                return 0
            }
            y: root.display === AbstractButton.TextUnderIcon
               ? 0
               : (group.height - height) / 2
            visible: root.showIcon

            // [QtEasyTier 补丁] 仅保留 IconImage 栅格化与 IconLabel 回退两条路径
            Loader {
                anchors.fill: parent
                active: root.showIcon
                sourceComponent: root.canRenderSvgDirectly
                                   ? rasterIconComponent
                                   : fallbackIconComponent
            }

            Component {
                id: rasterIconComponent

                IconImage {
                    source: root.icon ? root.icon.source : ""
                    color: root.icon ? root.icon.color : root.color
                    sourceSize.width: Math.ceil(width * root.devicePixelRatio)
                    sourceSize.height: Math.ceil(height * root.devicePixelRatio)
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: false
                }
            }

            Component {
                id: fallbackIconComponent

                // Preserve named icons and non-SVG sources exactly as before.
                IconLabel {
                    display: IconLabel.IconOnly
                    icon: root.icon
                    color: root.color
                }
            }
        }

        IconLabel {
            id: textLabel
            width: root.textWidth
            height: root.textHeight
            x: {
                if (root.display === AbstractButton.TextBesideIcon)
                    return root.mirrored ? 0 : root.iconWidth + (root.showIcon ? root.spacing : 0)
                if (root.display === AbstractButton.TextUnderIcon)
                    return (group.width - width) / 2
                return 0
            }
            y: root.display === AbstractButton.TextUnderIcon
               ? root.iconHeight + (root.showIcon ? root.spacing : 0)
               : (group.height - height) / 2
            display: IconLabel.TextOnly
            text: root.text
            font: root.font
            color: root.color
            visible: root.showText
            Behavior on color { ColorAnimation { duration: 120 } }
        }
    }
}
