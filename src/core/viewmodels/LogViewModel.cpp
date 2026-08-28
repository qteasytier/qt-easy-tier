/**
 * @file LogViewModel.cpp
 * @brief LogViewModel 实现
 *
 * 监听 LogRepository 的 logInserted / logsCleared 信号，
 * 自动刷新模型数据，保持 QML 视图与数据库同步。
 */
#include "LogViewModel.h"
#include <QDateTime>

LogViewModel::LogViewModel(LogRepository *repo, QObject *parent)
    : QAbstractListModel(parent)
    , m_repo(repo)
{
    // 监听仓库的新日志插入和清空事件，自动刷新视图
    connect(m_repo, &LogRepository::logInserted, this, &LogViewModel::loadLogs);
    connect(m_repo, &LogRepository::logsCleared, this, &LogViewModel::loadLogs);
    loadLogs();
}

int LogViewModel::rowCount(const QModelIndex &parent) const
{
    // 标准扁平列表模型：顶层项数 = 列表大小
    if (parent.isValid())
        return 0;
    return m_logs.size();
}

QVariant LogViewModel::data(const QModelIndex &index, int role) const
{
    // 越界检查
    if (!index.isValid() || index.row() >= m_logs.size())
        return {};

    const LogEntry &entry = m_logs.at(index.row());

    switch (role) {
    case TimestampRole:
        // 将毫秒时间戳转换为可读的日期时间字符串
        return QDateTime::fromMSecsSinceEpoch(entry.timestamp).toString("yyyy-MM-dd hh:mm:ss");
    case LevelRole:
        // 将枚举型日志级别映射为 QML 友好的字符串
        if (entry.level == LogLevel::Info)
            return QStringLiteral("info");
        if (entry.level == LogLevel::Warning)
            return QStringLiteral("warning");
        if (entry.level == LogLevel::Error)
            return QStringLiteral("error");
        return QStringLiteral("unknown");
    case MessageRole:
        // 有上下文时以 "[Context] message" 格式展示
        if (entry.context.isEmpty())
            return entry.message;
        return QStringLiteral("[%1] %2").arg(entry.context, entry.message);
    case ContextRole:
        return entry.context;
    case Qt::DisplayRole:
        return entry.message;
    default:
        return {};
    }
}

QHash<int, QByteArray> LogViewModel::roleNames() const
{
    // 将 C++ 角色枚举映射到 QML 属性名
    return {
        { TimestampRole, "timestamp" },
        { LevelRole, "level" },
        { MessageRole, "message" },
        { ContextRole, "context" }
    };
}

void LogViewModel::loadLogs()
{
    // 全量重置模型，从仓库加载最近 1000 条日志
    beginResetModel();
    m_logs = m_repo->loadRecent(1000);
    endResetModel();
    emit countChanged();
}

bool LogViewModel::clearLogs()
{
    // 代理到仓库层清空所有日志记录
    return m_repo->clearAll();
}

int LogViewModel::count() const
{
    return m_logs.size();
}
