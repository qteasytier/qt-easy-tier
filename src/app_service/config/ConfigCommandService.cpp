/** @file ConfigCommandService.cpp @brief ConfigCommandService 实现 */
#include "ConfigCommandService.h"

#include "core/config/NetworkConf.h"
#include "core/sqlite_repository/NetworkConfigRepository.h"

ConfigCommandService::ConfigCommandService(NetworkConfigRepository *repository, QObject *parent)
    : QObject(parent)
    , m_repository(repository)
{
}

std::optional<NetworkConf> ConfigCommandService::load(const QString &instanceName) const
{
    // 仓库层负责字段级反序列化，服务层直接透传读取结果
    return m_repository ? m_repository->load(instanceName) : std::nullopt;
}

QList<NetworkConf> ConfigCommandService::loadAll() const
{
    // 仓库不可用时返回空列表，调用方按空列表处理
    return m_repository ? m_repository->loadAll() : QList<NetworkConf>{};
}

bool ConfigCommandService::save(const NetworkConf &conf) const
{
    // 透传仓库保存，供配置编辑器等调用方统一落盘
    return m_repository && m_repository->save(conf);
}

ConfigOperationResult ConfigCommandService::createNewConfig()
{
    // 仓库不可用时直接失败，避免空指针解引用
    if (!m_repository)
        return ConfigOperationResult::fail(QStringLiteral("配置仓库不可用"));

    // 生成实例名并构建默认配置对象
    const QString instanceName = m_repository->generateUniqueInstanceName();
    NetworkConf cfg(instanceName);
    cfg.displayName = generateDisplayName();

    // 尝试写入仓库，失败则返回错误
    if (!m_repository->save(cfg))
        return ConfigOperationResult::fail(QStringLiteral("创建配置失败"));

    return ConfigOperationResult::ok(instanceName);
}

ConfigOperationResult ConfigCommandService::deleteConfig(const QString &instanceName)
{
    // 从仓库移除指定实例名的配置
    if (!m_repository || !m_repository->remove(instanceName))
        return ConfigOperationResult::fail(QStringLiteral("删除配置失败: %1").arg(instanceName));

    return ConfigOperationResult::ok(instanceName);
}

ConfigOperationResult ConfigCommandService::renameConfig(const QString &instanceName, const QString &newDisplayName)
{
    // 校验新名称非空
    const QString trimmed = newDisplayName.trimmed();
    if (trimmed.isEmpty())
        return ConfigOperationResult::fail(QStringLiteral("配置名称不能为空"));

    if (!m_repository)
        return ConfigOperationResult::fail(QStringLiteral("配置仓库不可用"));

    // 加载现有配置，修改显示名后写回
    auto loaded = m_repository->load(instanceName);
    if (!loaded.has_value())
        return ConfigOperationResult::fail(QStringLiteral("配置不存在: %1").arg(instanceName));

    auto cfg = loaded.value();
    cfg.displayName = trimmed;
    if (!m_repository->save(cfg))
        return ConfigOperationResult::fail(QStringLiteral("重命名失败: %1").arg(instanceName));

    return ConfigOperationResult::ok(instanceName);
}

QString ConfigCommandService::generateDisplayName() const
{
    // 生成 "新配置 N" 格式的显示名，递增编号直至不重复
    int counter = 1;
    QString displayName;
    bool taken;
    const auto configs = m_repository ? m_repository->loadAll() : QList<NetworkConf>{};
    do {
        displayName = QStringLiteral("新配置 %1").arg(counter++);
        taken = false;
        for (const auto &config : configs) {
            if (config.displayName == displayName) {
                taken = true;
                break;
            }
        }
    } while (taken);
    return displayName;
}
