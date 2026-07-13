/**
 * @file TrayMessageSink.h
 * @brief 系统托盘消息消费者接口
 *
 * 与日志系统的 LogSink 类似，所有能消费托盘消息的对象都实现此接口。
 * 当前实现只有 SystemTrayManager，后续如需测试或替换通知后端可直接扩展。
 */
#pragma once

#include "core/system_tray/TrayMessageTypes.h"

/** @brief 托盘消息输出槽抽象接口 */
class TrayMessageSink
{
public:
    virtual ~TrayMessageSink() = default;

    /** @brief 显示一条托盘消息 */
    virtual void showTrayMessage(const TrayMessage &message) = 0;
};
