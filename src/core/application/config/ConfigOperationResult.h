/** @file ConfigOperationResult.h @brief 配置操作结果类型，统一封装操作的成败、消息和实例名 */
#pragma once

#include <QString>

/** @brief 配置操作结果，承载 success/message/instanceName 三元组，提供静态工厂方法 */
struct ConfigOperationResult {
    bool success = false;
    QString message;
    QString instanceName;

    /** @brief 创建成功结果 @param instanceName 配置实例名 @param message 附加消息 @return 成功结果 */
    static ConfigOperationResult ok(const QString &instanceName = {}, const QString &message = {})
    {
        return ConfigOperationResult{true, message, instanceName};
    }

    /** @brief 创建失败结果 @param message 错误消息 @return 失败结果 */
    static ConfigOperationResult fail(const QString &message)
    {
        return ConfigOperationResult{false, message, {}};
    }
};
