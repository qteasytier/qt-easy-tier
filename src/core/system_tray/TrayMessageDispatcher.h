/**
 * @file TrayMessageDispatcher.h
 * @brief 系统托盘消息分发器
 *
 * 参考 LogDispatcher 的模式提供全局分发入口：业务层提交消息，dispatcher
 * 再分发给已注册的 TrayMessageSink，避免业务层直接依赖 QSystemTrayIcon。
 */
#pragma once

#include "core/system_tray/TrayMessageSink.h"

#include <QList>

/** @brief 托盘消息分发器，负责维护 sink 列表并广播消息 */
class TrayMessageDispatcher
{
public:
    /** @brief 获取全局唯一分发器实例 */
    static TrayMessageDispatcher *instance();

    /** @brief 注册托盘消息 sink，空指针或重复注册会被忽略 */
    void addSink(TrayMessageSink *sink);
    /** @brief 移除托盘消息 sink */
    void removeSink(TrayMessageSink *sink);
    /** @brief 清空所有 sink，主要用于测试隔离 */
    void clearSinks();
    /** @brief 将消息分发给当前所有 sink */
    void showMessage(const TrayMessage &message);

private:
    QList<TrayMessageSink *> m_sinks; ///< 已注册的消息消费者
};
