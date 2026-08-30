/** @file QtEasyTierTrayPlugin.h @brief DDE 托盘插件接口声明 */
#pragma once

#include <QObject>
#include <QPointer>

#include "pluginsiteminterface_v2.h"

class QLabel;
class TrayStatusService;
class TrayStatusWidget;

class QtEasyTierTrayPlugin final : public QObject, public PluginsItemInterfaceV2 {
    Q_OBJECT
    Q_INTERFACES(PluginsItemInterfaceV2)
    Q_PLUGIN_METADATA(IID ModuleInterface_iid_V2 FILE "tray-plugin.json")

public:
    explicit QtEasyTierTrayPlugin(QObject *parent = nullptr);
    ~QtEasyTierTrayPlugin() override;

    const QString pluginName() const override;
    const QString pluginDisplayName() const override;
    void init(PluginProxyInterface *proxyInter) override;
    QWidget *itemWidget(const QString &itemKey) override;
    QWidget *itemTipsWidget(const QString &itemKey) override;
    QWidget *itemPopupApplet(const QString &itemKey) override;
    const QString itemContextMenu(const QString &itemKey) override;
    void invokedMenuItem(const QString &itemKey, const QString &menuId, bool checked) override;
    Dock::PluginFlags flags() const override;
    QIcon icon(Dock::IconType dockPart, Dock::ThemeType themeType) const override;
    void refreshIcon(const QString &itemKey) override;
    QString message(const QString &message) override;

private:
    void updateIcon();
    static constexpr auto kItemKey = "qteasytier-network-status";
    static constexpr auto kOpenMainMenuId = "open-main";
    QPointer<QLabel> m_icon;
    QPointer<QLabel> m_tips;
    TrayStatusService *m_service = nullptr;
    QPointer<TrayStatusWidget> m_popup;
};
