/**
 * @file TrayMessageTypes.h
 * @brief 系统托盘消息公共类型
 *
 * 托盘消息接口只描述“要显示什么”，不直接依赖 QSystemTrayIcon。
 * 这样业务代码可以通过统一 helper 发送消息，而无需知道托盘对象的生命周期。
 */
#pragma once

#include <QString>

/** @brief 托盘消息级别，用于映射到系统托盘的提示图标 */
enum class TrayMessageLevel {
    Info = 0,    ///< 普通提示
    Warning = 1, ///< 警告提示
    Error = 2    ///< 错误提示
};

/** @brief 一条托盘通知消息 */
struct TrayMessage {
    TrayMessageLevel level = TrayMessageLevel::Info; ///< 消息级别
    QString title;                                   ///< 通知标题
    QString message;                                 ///< 通知正文
    int durationMs = 3000;                           ///< 建议显示时长，单位毫秒
};
