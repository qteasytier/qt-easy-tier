/**
 * @file DaemonLogModel.h
 * @brief daemon 事件日志列表 Model（QAbstractListModel 子类）
 *
 * 展示 qtet-daemon 返回的运行实例事件日志（时间戳 + 消息正文），供运行状态页
 * "运行日志"页签展示与导出。数据由 VpnRuntimeService 通过
 * setItems/setFromVariantList 注入。
 *
 * 注意与 LogViewModel 区分：本模型装的是**后端运行日志**（daemon/FFI 侧事件），
 * 内存条数上限由 VpnController 的 kMaxRuntimeLogEntries 单独定死；设置页的
 * "最大日志保存条数"管的是**应用日志**（app_logs 表），与本模型无关。
 */
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QVariantList>

/**
 * @brief daemon 事件日志条目，描述单条后端运行日志
 */
struct DaemonLogItem {
    QString rawTimestamp;  ///< 原始时间戳（毫秒级）
    QString timestamp;     ///< 格式化后的时间戳字符串
    QString message;       ///< 日志消息正文

    /// 值比较：数据未变化时用于跳过整表重置
    bool operator==(const DaemonLogItem &other) const = default;
};

/**
 * @brief daemon 事件日志列表 Model，供 QML 展示和纯文本导出
 */
class DaemonLogModel : public QAbstractListModel
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
        MessageRole,                        ///< 日志消息正文
        DisplayTextRole,                    ///< 组合显示文本（"[时间戳] 消息"）
    };
    Q_ENUM(Roles)

    explicit DaemonLogModel(QObject *parent = nullptr);

    // ---- QAbstractListModel 核心接口 ----
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const;
    /// 获取全部日志的纯文本（每行格式：[timestamp] message）
    QString plainText() const;

    /// 直接设置日志列表（从结构体列表）
    void setItems(const QList<DaemonLogItem> &items);
    /// 从 QVariantList 反序列化并设置日志列表（由 VpnRuntimeService 传入）
    void setFromVariantList(const QVariantList &items);

signals:
    /// 日志数量变化时发射
    void countChanged();
    /// 纯文本内容变化时发射
    void plainTextChanged();

private:
    QList<DaemonLogItem> m_items; ///< 日志条目缓存
    /// plainText 缓存：QML 绑定求值频繁，避免每次都重新拼接整段文本
    mutable QString m_plainText;
    mutable bool m_plainTextDirty = true; ///< 缓存是否失效
};
