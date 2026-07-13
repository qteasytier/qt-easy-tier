/** @file ConfigRunState.h @brief 配置运行状态枚举，定义 Stopped/Starting/Running/Stopping/Error 五种状态及状态判断函数 */
#pragma once

#include <QMetaType>

/** @brief 配置运行状态枚举 */
enum class ConfigRunState {
    Stopped = 0,  // 已停止
    Starting = 1, // 启动中
    Running = 2,  // 运行中
    Stopping = 3, // 停止中
    Error = 4,    // 错误状态
};

/** @brief 判断是否处于忙碌状态（启动中或停止中） @param state 运行状态 @return 是否忙碌 */
constexpr bool configRunStateIsBusy(ConfigRunState state)
{
    return state == ConfigRunState::Starting || state == ConfigRunState::Stopping;
}

/** @brief 判断是否允许删除（已停止或出错） @param state 运行状态 @return 是否可删除 */
constexpr bool configRunStateCanDelete(ConfigRunState state)
{
    return state == ConfigRunState::Stopped || state == ConfigRunState::Error;
}

/** @brief 判断是否正在运行 @param state 运行状态 @return 是否运行中 */
constexpr bool configRunStateIsRunning(ConfigRunState state)
{
    return state == ConfigRunState::Running;
}

Q_DECLARE_METATYPE(ConfigRunState)
