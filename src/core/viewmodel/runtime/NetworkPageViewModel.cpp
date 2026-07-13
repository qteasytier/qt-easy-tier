/**
 * @file NetworkPageViewModel.cpp
 * @brief NetworkPageViewModel 实现
 *
 * 通过网络页面协调器，接收配置编辑器的实例变更和 VPN 管理器的状态变更信号，
 * 自动维护 currentInstanceName、currentInstanceRunning 和页面切换状态。
 * 所有对 VpnManager 的调用通过 QMetaObject::invokeMethod 实现弱耦合。
 */
#include "NetworkPageViewModel.h"

#include <QMetaObject>

#include "core/viewmodel/ConfigEditorViewModel.h"
#include "core/viewmodel/ConfigListModel.h"

NetworkPageViewModel::NetworkPageViewModel(ConfigListModel *configListModel,
                                           ConfigEditorViewModel *configEditorViewModel,
                                           QObject *vpnManager,
                                           BackendStatusViewModel *backendStatusViewModel,
                                           QObject *parent)
    : QObject(parent)
    , m_configListModel(configListModel)
    , m_configEditorViewModel(configEditorViewModel)
    , m_vpnManager(vpnManager)
    , m_backendStatusViewModel(backendStatusViewModel)
{
    // 监听编辑器当前实例名称变化，同步更新本协调器的 currentInstanceName
    if (m_configEditorViewModel) {
        setCurrentInstanceName(m_configEditorViewModel->currentInstanceName());
        connect(m_configEditorViewModel, &ConfigEditorViewModel::currentInstanceNameChanged,
                this, [this]() {
                    setCurrentInstanceName(m_configEditorViewModel->currentInstanceName());
                    refreshRunning();
                });
    }

    // 监听 VPN 管理器发出的配置状态变更信号，自动刷新当前实例的运行状态
    if (m_vpnManager)
        connect(m_vpnManager, SIGNAL(configStateChanged(QString,ConfigRunState)),
                this, SLOT(refreshRunning()));
}

QString NetworkPageViewModel::currentInstanceName() const
{
    return m_currentInstanceName;
}

bool NetworkPageViewModel::currentInstanceRunning() const
{
    return m_currentInstanceRunning;
}

bool NetworkPageViewModel::showEditor() const
{
    // 无选中实例 或 选中实例未运行 → 展示编辑器
    return m_currentInstanceName.isEmpty() || !m_currentInstanceRunning;
}

bool NetworkPageViewModel::showRuntimeStatus() const
{
    // 有选中实例且正在运行 → 展示运行时状态
    return !m_currentInstanceName.isEmpty() && m_currentInstanceRunning;
}

void NetworkPageViewModel::refreshRunning()
{
    bool running = false;
    // 通过 invokeMethod 调用 VpnManager::isRunning 查询当前实例运行状态
    if (m_vpnManager && !m_currentInstanceName.isEmpty()) {
        QMetaObject::invokeMethod(m_vpnManager, "isRunning",
                                  Q_RETURN_ARG(bool, running),
                                  Q_ARG(QString, m_currentInstanceName));
    }
    setCurrentInstanceRunning(running);
    // 若正在运行，将当前实例名同步到 VpnManager 的 activeInstanceName 属性
    if (running && m_vpnManager)
        m_vpnManager->setProperty("activeInstanceName", m_currentInstanceName);
}

void NetworkPageViewModel::selectConfig(const QString &instanceName)
{
    // 选中配置：加载到编辑器 → 更新当前实例名 → 刷新运行状态
    if (m_configEditorViewModel)
        m_configEditorViewModel->loadConfig(instanceName);
    setCurrentInstanceName(instanceName);
    refreshRunning();
}

QString NetworkPageViewModel::createConfig()
{
    // 创建新配置并自动选中
    if (!m_configListModel)
        return {};
    const QString instanceName = m_configListModel->createNewConfig();
    if (!instanceName.isEmpty())
        selectConfig(instanceName);
    return instanceName;
}

void NetworkPageViewModel::deleteConfig(const QString &instanceName)
{
    if (!m_configListModel)
        return;

    const bool wasCurrent = instanceName == m_currentInstanceName;
    // 代理到配置列表模型执行删除
    if (!m_configListModel->deleteConfig(instanceName))
        return;

    // 若删除的是当前选中的配置，清空编辑器和运行状态
    if (wasCurrent) {
        if (m_configEditorViewModel)
            m_configEditorViewModel->clear();
        setCurrentInstanceName({});
        setCurrentInstanceRunning(false);
    }
}

void NetworkPageViewModel::renameConfig(const QString &instanceName, const QString &newDisplayName)
{
    // 代理到配置列表模型执行重命名
    if (m_configListModel)
        m_configListModel->renameConfig(instanceName, newDisplayName);
}

void NetworkPageViewModel::startConfig(const QString &instanceName)
{
    // 选中配置后调用 VpnManager 启动
    selectConfig(instanceName);
    if (m_vpnManager)
        QMetaObject::invokeMethod(m_vpnManager, "startConfig", Q_ARG(QString, instanceName));
}

void NetworkPageViewModel::stopConfig(const QString &instanceName)
{
    // 选中配置后调用 VpnManager 停止
    selectConfig(instanceName);
    if (m_vpnManager)
        QMetaObject::invokeMethod(m_vpnManager, "stopConfig", Q_ARG(QString, instanceName));
}

void NetworkPageViewModel::importConfig(const QString &filePath)
{
    // 代理到配置列表模型执行导入
    if (m_configListModel)
        m_configListModel->importConfig(filePath);
}

void NetworkPageViewModel::setCurrentInstanceName(const QString &instanceName)
{
    // 防抖：值未变化则跳过
    if (m_currentInstanceName == instanceName)
        return;
    m_currentInstanceName = instanceName;
    emit currentInstanceNameChanged();
    emit pageStateChanged();
}

void NetworkPageViewModel::setCurrentInstanceRunning(bool running)
{
    // 防抖：状态未变化则跳过
    if (m_currentInstanceRunning == running)
        return;
    m_currentInstanceRunning = running;
    emit currentInstanceRunningChanged();
    emit pageStateChanged();
}
