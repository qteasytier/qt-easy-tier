/**
 * @file RepositoryLogSink.h
 * @brief 仓库日志槽，将日志条目写入 LogRepository，支持最大条数限制
 */
#pragma once

#include "core/log/LogSink.h"

#include <QObject>

class LogRepository;

/** @brief 将日志持久化到 LogRepository 的日志槽，可配置最大保留条数 */
class RepositoryLogSink final : public QObject, public LogSink
{
    Q_OBJECT

public:
    /** @brief 构造函数 @param repository 日志仓库 @param parent 父对象 */
    explicit RepositoryLogSink(LogRepository *repository, QObject *parent = nullptr);

    /** @brief 写入一条日志条目到仓库 @param entry 日志条目 */
    void write(const LogEntry &entry) override;

    /** @brief 获取最大保留日志条数 @return 当前最大条数 */
    int maxEntries() const;
    /** @brief 设置最大保留日志条数 @param maxEntries 最大条数 */
    void setMaxEntries(int maxEntries);

private:
    LogRepository *m_repository = nullptr;
    int m_maxEntries = 100;
};
