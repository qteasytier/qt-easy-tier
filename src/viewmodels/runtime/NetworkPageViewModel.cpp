/**
 * @file NetworkPageViewModel.cpp
 * @brief NetworkPageViewModel 实现
 *
 * 通过网络页面协调器，接收配置编辑器的实例变更和 VPN 运行服务的状态变更信号，
 * 自动维护 currentInstanceName、currentInstanceRunning 和页面切换状态。
 * 所有 VPN 操作均通过应用服务层 VpnRuntimeService 完成。
 */
#include "NetworkPageViewModel.h"

#include "app_service/runtime/VpnRuntimeService.h"
#include "viewmodels/ConfigEditorViewModel.h"
#include "viewmodels/ConfigListModel.h"

NetworkPageViewModel::NetworkPageViewModel(ConfigListModel *configListModel,
                                           ConfigEditorViewModel *configEditorViewModel,
                                           VpnRuntimeService *vpnRuntimeService,
                                           BackendStatusViewModel *backendStatusViewModel,
                                           QObject *parent)
    : QObject(parent)
    , m_configListModel(configListModel)
    , m_configEditorViewModel(configEditorViewModel)
    , m_vpnRuntimeService(vpnRuntimeService)
    , m_backendStatusViewModel(backendStatusViewModel)
{
    // 监听编辑器当前实例名称变化，同步更新本协调器的 currentInstanceName
    if (m_configEditorViewModel) {
        setCurrentInstanceName(m_configEditorViewModel->currentInstanceName());
        refreshSecureMode();
        connect(m_configEditorViewModel, &ConfigEditorViewModel::currentInstanceNameChanged,
                this, [this]() {
                    setCurrentInstanceName(m_configEditorViewModel->currentInstanceName());
                    refreshSecureMode();
                    refreshRunning();
                });
        // 编辑器内切换安全模式开关时同步（运行状态页凭据能力据此禁用/启用）
        connect(m_configEditorViewModel, &ConfigEditorViewModel::secureModeEnabledChanged,
                this, &NetworkPageViewModel::refreshSecureMode);
    }

    // 监听 VPN 运行服务发出的配置状态变更信号，自动刷新当前实例的运行状态
    if (m_vpnRuntimeService)
        connect(m_vpnRuntimeService, &VpnRuntimeService::configStateChanged,
                this, [this](const QString &, ConfigRunState) {
                    refreshRunning();
                });
}

QString NetworkPageViewModel::currentInstanceName() const
{
    return m_currentInstanceName;
}

bool NetworkPageViewModel::currentInstanceRunning() const
{
    return m_currentInstanceRunning;
}

bool NetworkPageViewModel::currentInstanceSecureMode() const
{
    return m_currentInstanceSecureMode;
}

void NetworkPageViewModel::refreshSecureMode()
{
    const bool secure = m_configEditorViewModel && m_configEditorViewModel->secureModeEnabled();
    if (m_currentInstanceSecureMode == secure)
        return;
    m_currentInstanceSecureMode = secure;
    emit currentInstanceSecureModeChanged();
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
    // 通过应用服务层查询当前实例运行状态
    const bool running = m_vpnRuntimeService && !m_currentInstanceName.isEmpty()
        && m_vpnRuntimeService->isRunning(m_currentInstanceName);
    setCurrentInstanceRunning(running);
    // 若正在运行，将当前实例名同步到 VPN 运行服务的 activeInstanceName 属性
    if (running && m_vpnRuntimeService)
        m_vpnRuntimeService->setActiveInstanceName(m_currentInstanceName);
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
    // 选中配置后调用 VPN 运行服务启动
    selectConfig(instanceName);
    if (m_vpnRuntimeService)
        m_vpnRuntimeService->startConfig(instanceName);
}

void NetworkPageViewModel::stopConfig(const QString &instanceName)
{
    // 选中配置后调用 VPN 运行服务停止
    selectConfig(instanceName);
    if (m_vpnRuntimeService)
        m_vpnRuntimeService->stopConfig(instanceName);
}

void NetworkPageViewModel::importConfigFile(const QString &filePath)
{
    if (m_configListModel)
        m_configListModel->importConfigFile(filePath);
}

void NetworkPageViewModel::importConfigUrl(const QString &url)
{
    if (m_configListModel)
        m_configListModel->importConfigUrl(url);
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
