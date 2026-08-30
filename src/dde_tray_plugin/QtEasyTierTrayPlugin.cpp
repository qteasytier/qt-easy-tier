/** @file QtEasyTierTrayPlugin.cpp @brief DDE 托盘插件接口实现占位 */
#include "QtEasyTierTrayPlugin.h"

#include "TrayStatusService.h"
#include "TrayStatusWidget.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QPixmap>
#include <QProcess>

QtEasyTierTrayPlugin::QtEasyTierTrayPlugin(QObject *parent)
    : QObject(parent)
    , m_icon(new QLabel)
    , m_tips(new QLabel)
{
    m_icon->setFixedSize(16, 16);
    m_icon->setScaledContents(true);
}

QtEasyTierTrayPlugin::~QtEasyTierTrayPlugin()
{
    delete m_popup;
    delete m_tips;
    delete m_icon;
}

const QString QtEasyTierTrayPlugin::pluginName() const
{
    return QStringLiteral("qteasytier-network-status");
}

const QString QtEasyTierTrayPlugin::pluginDisplayName() const
{
    return tr("QtEasyTier 网络状态");
}

void QtEasyTierTrayPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;
    if (m_service)
        return;
    m_service = new TrayStatusService(QString(), this);
    m_popup = new TrayStatusWidget(m_service, nullptr);
    m_popup->setAttribute(Qt::WA_DeleteOnClose, false);
    connect(m_service, &TrayStatusService::snapshotChanged,
            this, &QtEasyTierTrayPlugin::updateIcon);
    updateIcon();
    m_proxyInter->itemAdded(this, QString::fromLatin1(kItemKey));
}

QWidget *QtEasyTierTrayPlugin::itemWidget(const QString &itemKey)
{
    return itemKey == QLatin1String(kItemKey) ? m_icon : nullptr;
}

QWidget *QtEasyTierTrayPlugin::itemTipsWidget(const QString &itemKey)
{
    if (itemKey != QLatin1String(kItemKey))
        return nullptr;
    const auto snapshot = m_service ? m_service->snapshot() : TrayStatusSnapshot{};
    int running = 0;
    int nodes = 0;
    for (const auto &instance : snapshot.instances) {
        if (instance.state == ConfigRunState::Running)
            ++running;
        if (instance.nodeCount)
            nodes += *instance.nodeCount;
    }
    m_tips->setText(tr("后端：%1，运行实例：%2 个，节点连接：%3 个")
                        .arg(snapshot.daemonState == DaemonClient::ConnectionState::Connected
                                 ? tr("已连接") : tr("未连接"))
                        .arg(running).arg(nodes));
    return m_tips;
}

QWidget *QtEasyTierTrayPlugin::itemPopupApplet(const QString &itemKey)
{
    return itemKey == QLatin1String(kItemKey) ? m_popup : nullptr;
}

const QString QtEasyTierTrayPlugin::itemContextMenu(const QString &itemKey)
{
    if (itemKey != QLatin1String(kItemKey))
        return {};
    const QVariantMap item{{QStringLiteral("itemId"), QString::fromLatin1(kOpenMainMenuId)},
                           {QStringLiteral("itemText"), tr("打开 QtEasyTier")},
                           {QStringLiteral("isCheckable"), false},
                           {QStringLiteral("isActive"), true},
                           {QStringLiteral("checked"), false}};
    return QString::fromUtf8(QJsonDocument(QJsonObject{
        {QStringLiteral("items"), QJsonArray{QJsonObject::fromVariantMap(item)}},
        {QStringLiteral("checkableMenu"), false},
        {QStringLiteral("singleCheck"), false}
    }).toJson());
}

void QtEasyTierTrayPlugin::invokedMenuItem(const QString &, const QString &menuId, bool)
{
    if (menuId == QLatin1String(kOpenMainMenuId))
        QProcess::startDetached(QStringLiteral("dde-am"), {QStringLiteral("qteasytier")});
}

Dock::PluginFlags QtEasyTierTrayPlugin::flags() const
{
    return Dock::PluginFlag::Type_Tray | Dock::PluginFlag::Attribute_CanSetting;
}

QIcon QtEasyTierTrayPlugin::icon(Dock::IconType, Dock::ThemeType) const
{
    return QIcon(QStringLiteral(":/dde-tray/qtet.png"));
}

void QtEasyTierTrayPlugin::refreshIcon(const QString &itemKey)
{
    if (itemKey == QLatin1String(kItemKey))
        updateIcon();
}

QString QtEasyTierTrayPlugin::message(const QString &message)
{
    if (message.isEmpty())
        return QStringLiteral("{}");
    return QStringLiteral("{}");
}

void QtEasyTierTrayPlugin::updateIcon()
{
    if (!m_service)
        return;
    const auto snapshot = m_service->snapshot();
    QString path = QStringLiteral(":/dde-tray/qtet.png");
    if (snapshot.daemonState != DaemonClient::ConnectionState::Connected)
        path = QStringLiteral(":/dde-tray/qtet-red.png");
    else {
        for (const auto &instance : snapshot.instances) {
            if (instance.state == ConfigRunState::Running) {
                path = QStringLiteral(":/dde-tray/qtet-green.png");
                break;
            }
        }
    }
    m_icon->setPixmap(QPixmap(path));
    if (m_proxyInter)
        m_proxyInter->itemUpdate(this, QString::fromLatin1(kItemKey));
}
