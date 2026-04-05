#ifndef QTETNODEINFO_H
#define QTETNODEINFO_H

#include <QWidget>
#include <QColor>
#include <QString>
#include <QLabel>

/// @brief 节点连接类型枚举
enum class NodeConnType
{
    Direct,     ///< 直连（绿色）
    Relay,      ///< 中转（橙色）
    Server      ///< 服务器（蓝色 #66ccff）
};

/// @brief 节点信息结构体
struct NodeInfo
{
    QString hostname;           ///< 用户名/主机名
    QString virtualIp;          ///< 虚拟IP地址
    NodeConnType connType;      ///< 连接类型
    int latencyMs;              ///< 延迟（毫秒），-1表示未知
    QString protocol;           ///< 协议（如 TCP、UDP、KCP、QUIC）
    QString connMethod;         ///< 连接方式（如 P2P、Relay等）
    bool isLocalNode;           ///< 是否为本机节点

    /// @brief 默认构造函数
    NodeInfo()
        : connType(NodeConnType::Direct)
        , latencyMs(-1)
        , isLocalNode(false)
    {}

    /// @brief 完整构造函数
    NodeInfo(const QString &host, const QString &ip, NodeConnType type,
             int latency, const QString &proto, const QString &method, bool local = false)
        : hostname(host)
        , virtualIp(ip)
        , connType(type)
        , latencyMs(latency)
        , protocol(proto)
        , connMethod(method)
        , isLocalNode(local)
    {}
};

/// @brief 连接类型标签控件（圆角彩色背景）
class QtETConnTypeLabel : public QWidget
{
    Q_OBJECT

public:
    explicit QtETConnTypeLabel(NodeConnType type, QWidget *parent = nullptr);
    void setConnType(NodeConnType type);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    NodeConnType m_connType;
    QColor m_connTypeColor;
    QString m_connTypeText;
    static constexpr int BORDER_RADIUS = 6;
    static constexpr int PADDING_H = 6;
    static constexpr int PADDING_V = 2;
    static constexpr int FONT_SIZE = 9;

    QColor getConnTypeColor() const;
    QString getConnTypeText() const;
};

/// @brief 本机标签控件（紫色圆角背景）
class QtETLocalLabel : public QWidget
{
    Q_OBJECT

public:
    explicit QtETLocalLabel(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    static constexpr int BORDER_RADIUS = 6;
    static constexpr int PADDING_H = 6;
    static constexpr int PADDING_V = 2;
    static constexpr int FONT_SIZE = 9;
};

/// @brief 节点信息显示控件
/// 显示节点的连接状态、延迟、协议等信息
/// 边框、背景与高亮仿照 QtETCheckBtn 绘制
class QtETNodeInfo : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal borderOpacity READ borderOpacity WRITE setBorderOpacity)

public:
    explicit QtETNodeInfo(QWidget *parent = nullptr);
    explicit QtETNodeInfo(const NodeInfo &info, QWidget *parent = nullptr);
    ~QtETNodeInfo() override;

    /// @brief 获取节点信息
    [[nodiscard]] NodeInfo nodeInfo() const;
    /// @brief 设置节点信息
    void setNodeInfo(const NodeInfo &info);

    /// @brief 获取边框不透明度（用于动画）
    [[nodiscard]] qreal borderOpacity() const;
    /// @brief 设置边框不透明度（用于动画）
    void setBorderOpacity(qreal opacity);

    /// @brief 获取推荐尺寸
    [[nodiscard]] QSize sizeHint() const override;
    /// @brief 获取最小尺寸
    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    /// @brief 初始化控件
    void init();
    /// @brief 创建内容布局
    void setupContent();
    /// @brief 更新颜色方案（根据深色/浅色模式）
    void updateColorScheme();
    /// @brief 更新内容显示
    void updateContent();

private:
    NodeInfo m_nodeInfo;                ///< 节点信息
    qreal m_borderOpacity;              ///< 边框高亮不透明度 (0.0 ~ 1.0)

    // 内容控件
    QLabel *m_ipHostLabel;              ///< IP和主机名标签（可选中复制）
    QtETConnTypeLabel *m_connTypeLabel; ///< 连接类型标签
    QtETLocalLabel *m_localLabel;       ///< 本机标签
    QLabel *m_detailLabel;              ///< 详细信息标签

    // 颜色方案
    QColor m_bgColor;                   ///< 背景颜色
    QColor m_borderColor;               ///< 边框颜色
    QColor m_highlightBorderColor;      ///< 高亮边框颜色
    QColor m_textColor;                 ///< 主文字颜色
    QColor m_detailColor;               ///< 详细信息文字颜色（蓝色 #66ccff）

    // 尺寸常量
    static constexpr int BORDER_RADIUS = 5;         ///< 边框圆角
    static constexpr int CONTENT_MARGIN = 10;       ///< 内容边距
    static constexpr int LINE_SPACING = 4;          ///< 行间距
    static constexpr int WIDGET_HEIGHT = 58;        ///< 控件固定高度
};

#endif // QTETNODEINFO_H
