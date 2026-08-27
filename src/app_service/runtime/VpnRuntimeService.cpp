/**
 * @file VpnRuntimeService.cpp
 * @brief VpnRuntimeService 实现
 *
 * 服务内部连线：
 * - VpnManager::configStateChanged / stopFailed / allStopped → 原样转发
 * - VpnManager::instanceInfoUpdated → 若实例为当前选中，刷新展示模型
 * - VpnManager::activeInstanceNameChanged → 刷新展示模型并转发
 */
#include "VpnRuntimeService.h"
#include "core/vpn_manager/VpnManager.h"

VpnRuntimeService::VpnRuntimeService(VpnManager *vpnManager, QObject *parent)
    : QObject(parent)
    , m_vpnManager(vpnManager)
{
    // 创建运行状态展示模型，本服务持有所有权（父对象为本服务）
    m_nodeInfoModel = new NodeInfoModel(this);
    m_runtimeLogModel = new RuntimeLogModel(this);

    if (!m_vpnManager)
        return;

    // 转发运行状态相关信号给 UI 层
    connect(m_vpnManager, &VpnManager::configStateChanged,
            this, &VpnRuntimeService::configStateChanged);
    connect(m_vpnManager, &VpnManager::stopFailed,
            this, &VpnRuntimeService::stopFailed);
    connect(m_vpnManager, &VpnManager::allStopped,
            this, &VpnRuntimeService::allStopped);

    // 运行状态信息更新（StatusMonitor 解析完成）→ 刷新展示模型
    connect(m_vpnManager, &VpnManager::instanceInfoUpdated,
            this, [this](const QString &instName,
                         const QVariantList &nodeInfos,
                         const QVariantList &logEntries) {
                // 仅当该实例是当前查看的实例时才刷新展示
                if (instName == m_vpnManager->activeInstanceName()) {
                    m_nodeInfoModel->setFromVariantList(nodeInfos);
                    m_runtimeLogModel->setFromVariantList(logEntries);
                }
            });

    // 选中实例变化（含删除当前实例导致的清空）→ 重新填充展示模型并转发
    connect(m_vpnManager, &VpnManager::activeInstanceNameChanged,
            this, [this]() {
                refreshModels();
                emit activeInstanceNameChanged();
            });

    // 初始填充：若装配时已有选中实例（通常为空），同步一次展示数据
    refreshModels();
}

void VpnRuntimeService::startConfig(const QString &instanceName)
{
    if (m_vpnManager)
        m_vpnManager->startConfig(instanceName);
}

void VpnRuntimeService::stopConfig(const QString &instanceName)
{
    if (m_vpnManager)
        m_vpnManager->stopConfig(instanceName);
}

void VpnRuntimeService::stopAll()
{
    if (m_vpnManager)
        m_vpnManager->stopAll();
}

bool VpnRuntimeService::isRunning(const QString &instanceName) const
{
    return m_vpnManager && m_vpnManager->isRunning(instanceName);
}

int VpnRuntimeService::configState(const QString &instanceName) const
{
    return m_vpnManager ? m_vpnManager->configState(instanceName)
                        : static_cast<int>(ConfigRunState::Stopped);
}

bool VpnRuntimeService::exportLog(const QString &filePath)
{
    return m_vpnManager && m_vpnManager->exportLog(filePath);
}

void VpnRuntimeService::setHideServerNodes(bool value)
{
    if (m_nodeInfoModel)
        m_nodeInfoModel->setHideServerNodes(value);
}

QString VpnRuntimeService::activeInstanceName() const
{
    return m_vpnManager ? m_vpnManager->activeInstanceName() : QString();
}

void VpnRuntimeService::setActiveInstanceName(const QString &name)
{
    // 委托 VpnManager 记录选中实例；展示刷新由 activeInstanceNameChanged 连线完成
    if (m_vpnManager)
        m_vpnManager->setActiveInstanceName(name);
}

NodeInfoModel *VpnRuntimeService::nodeInfoModel() const
{
    return m_nodeInfoModel;
}

RuntimeLogModel *VpnRuntimeService::runtimeLogModel() const
{
    return m_runtimeLogModel;
}

void VpnRuntimeService::cleanupController(const QString &instanceName)
{
    if (m_vpnManager)
        m_vpnManager->cleanupController(instanceName);
}

void VpnRuntimeService::ensureLocalController(const QString &instanceName)
{
    if (m_vpnManager)
        m_vpnManager->ensureLocalController(instanceName);
}

void VpnRuntimeService::refreshModels()
{
    if (!m_vpnManager)
        return;

    // 从 VpnManager 缓存中读取当前选中实例的节点与日志数据并填充模型
    const QString name = m_vpnManager->activeInstanceName();
    m_nodeInfoModel->setFromVariantList(m_vpnManager->nodeInfosFor(name));
    m_runtimeLogModel->setFromVariantList(m_vpnManager->logEntriesFor(name));
}
