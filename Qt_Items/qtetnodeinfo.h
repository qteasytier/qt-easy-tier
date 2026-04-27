#ifndef QTETNODEINFO_H
#define QTETNODEINFO_H

#include <QWidget>
#include <QColor>
#include <QString>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include <QContextMenuEvent>

enum class NodeConnType
{
    Direct,
    Relay,
    Server
};

struct NodeInfo
{
    QString hostname;
    QString virtualIp;
    NodeConnType connType = NodeConnType::Direct;
    int latencyMs = -1;
    QString protocol;
    QString connMethod;
    bool isLocalNode = false;

    NodeInfo() = default;
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

class QtETNodeInfo : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal borderOpacity READ borderOpacity WRITE setBorderOpacity)

public:
    explicit QtETNodeInfo(QWidget *parent = nullptr);
    explicit QtETNodeInfo(const NodeInfo &info, QWidget *parent = nullptr);
    ~QtETNodeInfo() override = default;

    [[nodiscard]] NodeInfo nodeInfo() const;
    void setNodeInfo(const NodeInfo &info);

    [[nodiscard]] qreal borderOpacity() const;
    void setBorderOpacity(qreal opacity);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void init();
    void updateColorScheme();
    [[nodiscard]] static QColor getConnTypeColor(NodeConnType type);
    [[nodiscard]] static QString getConnTypeText(NodeConnType type);
    [[nodiscard]] int calculateLabelsWidth() const;

private:
    NodeInfo m_nodeInfo;
    qreal m_borderOpacity = 0.0;
    QPropertyAnimation *m_borderAnimation = nullptr;

    QColor m_bgColor;
    QColor m_borderColor;
    QColor m_highlightBorderColor;
    QColor m_textColor;
    QColor m_detailColor;

    static constexpr int BORDER_RADIUS = 5;
    static constexpr int CONTENT_MARGIN = 10;
    static constexpr int LINE_SPACING = 6;
    static constexpr int WIDGET_HEIGHT = 58;
    static constexpr int LABEL_BORDER_RADIUS = 6;
    static constexpr int LABEL_PADDING_H = 6;
    static constexpr int LABEL_PADDING_V = 2;
    static constexpr int LABEL_FONT_SIZE = 9;
    static constexpr int LABEL_SPACING = 4;
};

#endif // QTETNODEINFO_H
