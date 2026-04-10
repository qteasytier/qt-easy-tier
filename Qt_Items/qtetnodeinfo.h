#ifndef QTETNODEINFO_H
#define QTETNODEINFO_H

#include <QWidget>
#include <QColor>
#include <QString>
#include <QPropertyAnimation>

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

/// @brief 节点信息显示控件
/// 完全自定义绘制，不使用任何 QSS 或 QLabel
/// 显示节点的连接状态、延迟、协议等信息
class QtETNodeInfo : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal borderOpacity READ borderOpacity WRITE setBorderOpacity)

public:
    explicit QtETNodeInfo(QWidget *parent = nullptr);
    explicit QtETNodeInfo(const NodeInfo &info, QWidget *parent = nullptr);
    ~QtETNodeInfo() override = default;

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
    /// @brief 更新颜色方案（根据深色/浅色模式）
    void updateColorScheme();
    /// @brief 获取连接类型颜色
    [[nodiscard]] QColor getConnTypeColor() const;
    /// @brief 获取连接类型文字
    [[nodiscard]] QString getConnTypeText() const;

private:
    NodeInfo m_nodeInfo;                ///< 节点信息
    qreal m_borderOpacity;              ///< 边框高亮不透明度 (0.0 ~ 1.0)
    QPropertyAnimation *m_borderAnimation; ///< 边框动画

    // 颜色方案
    QColor m_bgColor;                   ///< 背景颜色
    QColor m_borderColor;               ///< 边框颜色
    QColor m_highlightBorderColor;      ///< 高亮边框颜色
    QColor m_textColor;                 ///< 主文字颜色
    QColor m_detailColor;               ///< 详细信息文字颜色（蓝色 #66ccff）

    // 尺寸常量
    static constexpr int BORDER_RADIUS = 5;         ///< 边框圆角
    static constexpr int CONTENT_MARGIN = 10;       ///< 内容边距
    static constexpr int LINE_SPACING = 6;          ///< 行间距
    static constexpr int WIDGET_HEIGHT = 58;        ///< 控件固定高度
    static constexpr int LABEL_BORDER_RADIUS = 6;   ///< 标签圆角
    static constexpr int LABEL_PADDING_H = 6;       ///< 标签水平内边距
    static constexpr int LABEL_PADDING_V = 2;       ///< 标签垂直内边距
    static constexpr int LABEL_FONT_SIZE = 9;       ///< 标签字体大小
    static constexpr int LABEL_SPACING = 4;         ///< 标签间距
};

#endif // QTETNODEINFO_H