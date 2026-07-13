/**
 * @file RuntimeLogModel.h
 * @brief VPN 运行时日志列表 Model（QAbstractListModel 子类）
 *
 * 展示 VPN 运行过程中产生的实时日志条目，包括时间戳、级别、消息正文。
 * 支持 QML 列表展示和 plainText 纯文本导出。数据由 VpnManager 通过
 * setItems/setFromVariantList 注入。
 */
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QVariantList>

/**
 * @brief 运行日志条目，描述单条 VPN 运行日志
 */
struct RuntimeLogItem {
    QString rawTimestamp;  ///< 原始时间戳（毫秒级）
    QString timestamp;     ///< 格式化后的时间戳字符串
    QString level;         ///< 日志级别标识
    QString levelText;     ///< 日志级别显示文本（大写）
    QString message;       ///< 日志消息正文
};

/**
 * @brief VPN 运行时日志列表 Model，供 QML 展示和纯文本导出
 */
class RuntimeLogModel : public QAbstractListModel
{
    Q_OBJECT
    /// 当前日志条目数量
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    /// 全部日志的纯文本格式（换行分隔），用于复制/导出
    Q_PROPERTY(QString plainText READ plainText NOTIFY plainTextChanged FINAL)

public:
    /// QML 可访问的数据角色枚举
    enum Roles {
        TimestampRole = Qt::UserRole + 1,  ///< 格式化时间戳
        RawTimestampRole,                   ///< 原始时间戳
        LevelRole,                          ///< 日志级别标识
        LevelTextRole,                      ///< 日志级别显示文本
        MessageRole,                        ///< 日志消息正文
        DisplayTextRole,                    ///< 组合显示文本（"[时间戳] 消息"）
    };
    Q_ENUM(Roles)

    explicit RuntimeLogModel(QObject *parent = nullptr);

    // ---- QAbstractListModel 核心接口 ----
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const;
    /// 获取全部日志的纯文本（每行格式：[timestamp] message）
    QString plainText() const;

    /// 直接设置日志列表（从结构体列表）
    void setItems(const QList<RuntimeLogItem> &items);
    /// 从 QVariantList 反序列化并设置日志列表（由 VpnManager 传入）
    void setFromVariantList(const QVariantList &items);

signals:
    /// 日志数量变化时发射
    void countChanged();
    /// 纯文本内容变化时发射
    void plainTextChanged();

private:
    QList<RuntimeLogItem> m_items; ///< 日志条目缓存
};
