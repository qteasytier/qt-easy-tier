/**
 * @file NetworkPageViewModel.cpp
 * @brief NetworkPageViewModel 实现
 *
 * 通过网络页面协调器，接收配置编辑器的实例变更和 VPN 运行服务的状态变更信号，
 * 自动维护 currentInstanceName、currentInstanceRunning 和页面切换状态。
 * 所有 VPN 操作均通过应用核心层 VpnRuntimeService 完成。
 */
#include "NetworkPageViewModel.h"

#include "core/runtime/VpnRuntimeService.h"
#include "core/viewmodels/ConfigEditorViewModel.h"
#include "core/viewmodels/ConfigListModel.h"

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
    // 注意：configStateChanged → refreshRunning 的连接统一由 AppServices 装配，
    // 且保证 ConfigListModel 先更新状态缓存、本 ViewModel 再刷新（避免读到旧状态）。
}

QString NetworkPageViewModel::currentInstanceName() const
{
    return m_currentInstanceName;
}

bool NetworkPageViewModel::currentInstanceRunning() const
{
    return m_currentInstanceRunning;
}

int NetworkPageViewModel::currentInstanceRunState() const
{
    return static_cast<int>(m_currentInstanceRunState);
}

bool NetworkPageViewModel::currentInstanceBusy() const
{
    // 纯派生属性：启动/停止过渡期间禁止编辑与重复启停
    return configRunStateIsBusy(m_currentInstanceRunState);
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
    // 从配置列表模型读取当前实例的完整运行状态（该缓存由 configStateChanged 信号维护）
    const ConfigRunState state = m_configListModel
        ? m_configListModel->instanceState(m_currentInstanceName)
        : ConfigRunState::Stopped;

    // 更新完整状态缓存并发射变化信号
    if (m_currentInstanceRunState != state) {
        m_currentInstanceRunState = state;
        emit currentInstanceRunStateChanged();
    }

    setCurrentInstanceRunning(state == ConfigRunState::Running);
    // 若正在运行，将当前实例名同步到 VPN 运行服务的 activeInstanceName 属性
    if (state == ConfigRunState::Running && m_vpnRuntimeService)
        m_vpnRuntimeService->setActiveInstanceName(m_currentInstanceName);
}

void NetworkPageViewModel::selectConfig(const QString &instanceName)
{
    // 外部实例（daemon 中存在但本地配置列表中没有）没有本地配置：
    // 跳过加载到编辑器，避免 loadConfig 因仓库中无此配置而报错并重置编辑器
    const bool external = m_configListModel && m_configListModel->isExternal(instanceName);
    if (!external && m_configEditorViewModel)
        m_configEditorViewModel->loadConfig(instanceName);
    setCurrentInstanceName(instanceName);
    refreshRunning();
}

void NetworkPageViewModel::clearSelection()
{
    // 清空当前选中实例（模拟删除配置时对当前选中实例的处理）
    setCurrentInstanceName({});
    setCurrentInstanceRunning(false);
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

    // 仅转发删除请求：实际删除完成的清空由 handleConfigDeleted(configDeleted 信号)处理。
    // 运行中配置会先请求停止、待真正删除后才清空，避免删除尚未完成就清空页面选择。
    m_configListModel->deleteConfig(instanceName);
}

void NetworkPageViewModel::handleConfigDeleted(const QString &instanceName)
{
    if (instanceName != m_currentInstanceName)
        return;

    // 配置已真正从仓库删除：丢弃编辑快照（不刷写待保存修改，避免"复活"配置），再清空选择
    if (m_configEditorViewModel)
        m_configEditorViewModel->discardAndClear();
    setCurrentInstanceName({});
    setCurrentInstanceRunning(false);
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
