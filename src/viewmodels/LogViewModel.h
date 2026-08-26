/**
 * @file LogViewModel.h
 * @brief 系统日志列表 ViewModel（QAbstractListModel 子类）
 *
 * 展示系统日志条目（时间戳、级别、消息、上下文），
 * 作为 QML ListView 的数据源。数据来自 LogRepository（SQLite）。
 */
#pragma once
#include <QAbstractListModel>
#include <QList>
#include "core/repository/LogRepository.h"

class LogRepository;

/**
 * @brief 系统日志列表 ViewModel，从 LogRepository 加载日志并暴露给 QML 视图
 */
class LogViewModel : public QAbstractListModel {
    Q_OBJECT
    /// 当前日志条目数量
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

public:
    /// QML 可访问的数据角色枚举
    enum Roles {
        TimestampRole = Qt::UserRole + 1, ///< 时间戳（格式化的日期时间字符串）
        LevelRole,                         ///< 日志级别（"info" / "warning" / "error"）
        MessageRole,                       ///< 日志消息正文（含上下文前缀）
        ContextRole                        ///< 日志上下文标识
    };

    /**
     * @brief 构造日志 ViewModel
     * @param repo 日志持久化仓库（非所有权，生命周期由外部管理）
     * @param parent 父 QObject
     */
    explicit LogViewModel(LogRepository *repo, QObject *parent = nullptr);

    // ---- QAbstractListModel 核心接口 ----
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// 从仓库加载最近日志（最近 1000 条）
    Q_INVOKABLE void loadLogs();
    /// 清空所有日志
    Q_INVOKABLE bool clearLogs();

    /// 获取当前日志条目数量
    int count() const;

signals:
    /// 日志数量变化时发射
    void countChanged();

private:
    LogRepository *m_repo = nullptr; ///< 日志仓库（非所有权）
    QList<LogEntry> m_logs;          ///< 内存中的日志条目缓存
};
