/* @file AppAboutDialog.qml
 * @brief DDE 关于对话框：左图标+产品名、右版本/主页/描述/致谢，尺寸取官方默认
 *
 * 官方右信息列基于 Flickable，在本环境会触发递归布局终止，故用 ColumnLayout 复刻。
 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk
import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style 1.0 as DStyle

DialogWindow {
    id: root

    modality: Qt.ApplicationModal

    width: DStyle.Style.aboutDialog.width
    height: DStyle.Style.aboutDialog.height
    // 官方关于页无标题行：仅保留 DialogTitleBar（右上角关闭按钮）
    title: ""

    header: DialogTitleBar {
        id: dialogTitleBar
    }

    RowLayout {
        width: parent.width
        height: parent.height - dialogTitleBar.height
        spacing: 12

        // 左侧：应用图标 + 产品名
        ColumnLayout {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: DStyle.Style.aboutDialog.leftAreaWidth
            spacing: 10

            Image {
                source: "qrc:/icons/qtet.png"
                sourceSize: Qt.size(96, 96)
                Layout.preferredWidth: 96
                Layout.preferredHeight: 96
                Layout.alignment: Qt.AlignHCenter
                smooth: true
            }

            Label {
                text: qsTr("QtEasyTier")
                font: D.DTK.fontManager.t6
                Layout.alignment: Qt.AlignHCenter
            }
        }

        // 右侧：版本/主页/描述/致谢
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 16
            Layout.bottomMargin: 16
            spacing: 10

            ColumnLayout {
                spacing: 1
                Label {
                    text: qsTr("版本")
                    font: D.DTK.fontManager.t10
                }
                Label {
                    text: SettingsViewModel.frontendVersion
                    font: D.DTK.fontManager.t8
                }
            }

            ColumnLayout {
                spacing: 1
                Label {
                    text: qsTr("主页")
                    font: D.DTK.fontManager.t10
                }
                Label {
                    text: "<a href='https://qtet.cn' style='text-decoration: none; color: #004EE5;'>qtet.cn</a>"
                    textFormat: Text.RichText
                    font: D.DTK.fontManager.t8
                    onLinkActivated: function (link) {
                        Qt.openUrlExternally(link)
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.NoButton
                    }
                }
            }

            ColumnLayout {
                spacing: 1
                Label {
                    text: qsTr("描述")
                    font: D.DTK.fontManager.t10
                }
                Label {
                    text: qsTr("基于 EasyTier, 一款美观实用的联机组网工具!")
                    font: D.DTK.fontManager.t8
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            ColumnLayout {
                spacing: 1
                Label {
                    text: qsTr("致谢")
                    font: D.DTK.fontManager.t10
                }
                Label {
                    text: qsTr("致谢所使用的开源软件以及 deepin 开源社区")
                    font: D.DTK.fontManager.t8
                }
            }
        }
    }
}
