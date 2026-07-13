/**
 * @file LogTypes.h
 * @brief 日志系统公共类型定义
 *
 * 定义日志级别枚举和日志条目结构体，
 * 供 LogDispatcher、LogSink 等组件共享使用。
 * LogEntry 为纯数据结构，值语义安全，无所有权问题。
 */
#pragma once

#include <QString>

/**
 * @brief 日志级别枚举
 *
 * 数值越大表示严重程度越高，None 用于禁用所有日志输出。
 */
enum class LogLevel {
    Info = 0,     ///< 信息级别日志
    Warning = 1,  ///< 警告级别日志
    Error = 2,    ///< 错误级别日志
    None = 3      ///< 关闭所有日志输出
};

/**
 * @brief 单条日志记录
 *
 * 包含日志的所有元数据：ID、时间戳、级别、上下文和消息内容。
 * id 字段由持久化层（SQLite repository）分配，未持久化时为 0。
 */
struct LogEntry {
    qint64 id = 0;             ///< 数据库主键，0 表示未持久化
    qint64 timestamp = 0;      ///< Unix 时间戳（毫秒）
    LogLevel level = LogLevel::Info;  ///< 日志级别
    QString context;           ///< 日志来源上下文（如模块名）
    QString message;           ///< 日志正文内容
};
