#include "qtetnodeinfo.h"

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleHints>
#include <QFontMetrics>

QtETNodeInfo::QtETNodeInfo(QWidget *parent)
    : QWidget(parent)
    , m_borderOpacity(0.0)
    , m_borderAnimation(nullptr)
{
    init();
}

QtETNodeInfo::QtETNodeInfo(const NodeInfo &info, QWidget *parent)
    : QWidget(parent)
    , m_nodeInfo(info)
    , m_borderOpacity(0.0)
    , m_borderAnimation(nullptr)
{
    init();
}

void QtETNodeInfo::init()
{
    // 设置固定高度
    setFixedHeight(WIDGET_HEIGHT);
    setMinimumWidth(250);

    // 初始化边框动画
    m_borderAnimation = new QPropertyAnimation(this, "borderOpacity", this);
    m_borderAnimation->setDuration(200);
    m_borderAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // 更新颜色方案
    updateColorScheme();

    // 监听系统主题变化
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        updateColorScheme();
        update();
    });
}

void QtETNodeInfo::updateColorScheme()
{
    const bool isDark = (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark);

    if (isDark) {
        m_bgColor = QColor(45, 45, 45);
        m_borderColor = QColor(70, 70, 70);
        m_highlightBorderColor = QColor("#66ccff");
        m_textColor = QColor(220, 220, 220);
        m_detailColor = QColor("#66ccff");
    } else {
        m_bgColor = QColor(240, 240, 240);
        m_borderColor = QColor(180, 180, 180);
        m_highlightBorderColor = QColor("#66ccff");
        m_textColor = QColor(30, 30, 30);
        m_detailColor = QColor("#66ccff");
    }

    update();
}

QColor QtETNodeInfo::getConnTypeColor() const
{
    switch (m_nodeInfo.connType) {
    case NodeConnType::Direct:
        return QColor("#4CAF50");   // 绿色
    case NodeConnType::Relay:
        return QColor("#FF9800");   // 橙色
    case NodeConnType::Server:
        return QColor("#66ccff");   // 蓝色
    default:
        return QColor("#4CAF50");
    }
}

QString QtETNodeInfo::getConnTypeText() const
{
    switch (m_nodeInfo.connType) {
    case NodeConnType::Direct:
        return tr("直连");
    case NodeConnType::Relay:
        return tr("中转");
    case NodeConnType::Server:
        return tr("服务器");
    default:
        return tr("直连");
    }
}

NodeInfo QtETNodeInfo::nodeInfo() const
{
    return m_nodeInfo;
}

void QtETNodeInfo::setNodeInfo(const NodeInfo &info)
{
    m_nodeInfo = info;
    update();
}

qreal QtETNodeInfo::borderOpacity() const
{
    return m_borderOpacity;
}

void QtETNodeInfo::setBorderOpacity(qreal opacity)
{
    if (!qFuzzyCompare(m_borderOpacity, opacity)) {
        m_borderOpacity = qBound(0.0, opacity, 1.0);
        update();
    }
}

QSize QtETNodeInfo::sizeHint() const
{
    return QSize(300, WIDGET_HEIGHT);
}

QSize QtETNodeInfo::minimumSizeHint() const
{
    return QSize(250, WIDGET_HEIGHT);
}

void QtETNodeInfo::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制控件边框和背景
    QRect borderRect = rect().adjusted(1, 1, -1, -1);

    // 根据边框不透明度混合边框颜色
    QColor finalBorderColor = QColor::fromRgbF(
        m_borderColor.redF() * (1 - m_borderOpacity) + m_highlightBorderColor.redF() * m_borderOpacity,
        m_borderColor.greenF() * (1 - m_borderOpacity) + m_highlightBorderColor.greenF() * m_borderOpacity,
        m_borderColor.blueF() * (1 - m_borderOpacity) + m_highlightBorderColor.blueF() * m_borderOpacity
    );

    // 绘制背景
    QPainterPath bgPath;
    bgPath.addRoundedRect(borderRect, BORDER_RADIUS, BORDER_RADIUS);
    painter.fillPath(bgPath, m_bgColor);

    // 绘制边框
    painter.setPen(QPen(finalBorderColor, 1));
    painter.drawPath(bgPath);

    // ========== 绘制第一行内容 ==========
    int contentLeft = CONTENT_MARGIN;
    int contentTop = CONTENT_MARGIN;
    int contentWidth = width() - 2 * CONTENT_MARGIN;

    // 第一行高度
    QFont ipHostFont = font();
    ipHostFont.setPointSize(11);
    ipHostFont.setWeight(QFont::Medium);
    QFontMetrics ipHostFm(ipHostFont);
    int firstLineHeight = ipHostFm.height();

    // 绘制 IP | hostname
    QString ipHostText = QString("%1 | %2").arg(m_nodeInfo.virtualIp, m_nodeInfo.hostname);
    painter.setFont(ipHostFont);
    painter.setPen(m_textColor);
    int ipHostY = contentTop + ipHostFm.ascent();
    painter.drawText(contentLeft, ipHostY, ipHostText);

    // 计算标签位置
    int labelX = contentLeft + contentWidth;
    int labelY = contentTop + (firstLineHeight - (LABEL_PADDING_V * 2 + LABEL_FONT_SIZE + 2)) / 2;

    // 绘制本机标签（如果有）
    QFont labelFont = font();
    labelFont.setPointSize(LABEL_FONT_SIZE);
    labelFont.setWeight(QFont::Bold);
    QFontMetrics labelFm(labelFont);

    if (m_nodeInfo.isLocalNode) {
        QString localText = tr("本机");
        int localWidth = labelFm.horizontalAdvance(localText) + 2 * LABEL_PADDING_H;
        int localHeight = labelFm.height() + 2 * LABEL_PADDING_V;

        labelX -= localWidth + LABEL_SPACING;

        QPainterPath localPath;
        localPath.addRoundedRect(labelX, labelY, localWidth, localHeight,
                                  LABEL_BORDER_RADIUS, LABEL_BORDER_RADIUS);
        painter.fillPath(localPath, QColor("#9C27B0"));  // 紫色

        painter.setFont(labelFont);
        painter.setPen(Qt::white);
        int textX = labelX + (localWidth - labelFm.horizontalAdvance(localText)) / 2;
        int textY = labelY + LABEL_PADDING_V + labelFm.ascent();
        painter.drawText(textX, textY, localText);
    }

    // 绘制连接类型标签
    QString connText = getConnTypeText();
    int connWidth = labelFm.horizontalAdvance(connText) + 2 * LABEL_PADDING_H;
    int connHeight = labelFm.height() + 2 * LABEL_PADDING_V;

    labelX -= connWidth;

    QPainterPath connPath;
    connPath.addRoundedRect(labelX, labelY, connWidth, connHeight,
                             LABEL_BORDER_RADIUS, LABEL_BORDER_RADIUS);
    painter.fillPath(connPath, getConnTypeColor());

    painter.setFont(labelFont);
    painter.setPen(Qt::white);
    int connTextX = labelX + (connWidth - labelFm.horizontalAdvance(connText)) / 2;
    int connTextY = labelY + LABEL_PADDING_V + labelFm.ascent();
    painter.drawText(connTextX, connTextY, connText);

    // ========== 绘制第二行内容 ==========
    // 构建详细信息字符串
    QStringList detailParts;

    if (m_nodeInfo.latencyMs >= 0) {
        detailParts << QString("%1ms").arg(m_nodeInfo.latencyMs);
    } else {
        detailParts << QString("--ms");
    }

    if (!m_nodeInfo.protocol.isEmpty()) {
        detailParts << m_nodeInfo.protocol;
    }

    if (!m_nodeInfo.connMethod.isEmpty()) {
        detailParts << m_nodeInfo.connMethod;
    }

    QString detailText = detailParts.join(" | ");

    QFont detailFont = font();
    detailFont.setPointSize(10);
    QFontMetrics detailFm(detailFont);

    int detailY = contentTop + firstLineHeight + LINE_SPACING + detailFm.ascent();
    painter.setFont(detailFont);
    painter.setPen(m_detailColor);
    painter.drawText(contentLeft, detailY, detailText);
}

void QtETNodeInfo::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);

    // 启动边框高亮动画（渐显）
    m_borderAnimation->stop();
    m_borderAnimation->setStartValue(m_borderOpacity);
    m_borderAnimation->setEndValue(1.0);
    m_borderAnimation->start();
}

void QtETNodeInfo::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);

    // 启动边框淡出动画（渐隐）
    m_borderAnimation->stop();
    m_borderAnimation->setStartValue(m_borderOpacity);
    m_borderAnimation->setEndValue(0.0);
    m_borderAnimation->start();
}
