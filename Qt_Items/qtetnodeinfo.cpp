#include "qtetnodeinfo.h"

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleHints>
#include <QFontMetrics>
#include <QPropertyAnimation>
#include <QHBoxLayout>
#include <QVBoxLayout>

// ==================== QtETConnTypeLabel ====================

QtETConnTypeLabel::QtETConnTypeLabel(NodeConnType type, QWidget *parent)
    : QWidget(parent)
    , m_connType(type)
{
    m_connTypeColor = getConnTypeColor();
    m_connTypeText = getConnTypeText();

    // 计算所需大小
    QFont font;
    font.setPointSize(FONT_SIZE);
    font.setWeight(QFont::Bold);
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(m_connTypeText);
    int textHeight = fm.height();

    setFixedSize(textWidth + 2 * PADDING_H, textHeight + 2 * PADDING_V);
}

void QtETConnTypeLabel::setConnType(NodeConnType type)
{
    if (m_connType != type) {
        m_connType = type;
        m_connTypeColor = getConnTypeColor();
        m_connTypeText = getConnTypeText();
        update();
    }
}

QColor QtETConnTypeLabel::getConnTypeColor() const
{
    switch (m_connType) {
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

QString QtETConnTypeLabel::getConnTypeText() const
{
    switch (m_connType) {
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

void QtETConnTypeLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制圆角背景
    QPainterPath path;
    path.addRoundedRect(rect(), BORDER_RADIUS, BORDER_RADIUS);
    painter.fillPath(path, m_connTypeColor);

    // 绘制文字
    QFont font;
    font.setPointSize(FONT_SIZE);
    font.setWeight(QFont::Bold);
    painter.setFont(font);
    painter.setPen(Qt::white);

    QFontMetrics fm(font);
    int textX = (width() - fm.horizontalAdvance(m_connTypeText)) / 2;
    int textY = (height() - fm.height()) / 2 + fm.ascent();
    painter.drawText(textX, textY, m_connTypeText);
}

// ==================== QtETNodeInfo ====================

QtETNodeInfo::QtETNodeInfo(QWidget *parent)
    : QWidget(parent)
    , m_borderOpacity(0.0)
    , m_ipHostLabel(nullptr)
    , m_connTypeLabel(nullptr)
    , m_detailLabel(nullptr)
{
    init();
}

QtETNodeInfo::QtETNodeInfo(const NodeInfo &info, QWidget *parent)
    : QWidget(parent)
    , m_nodeInfo(info)
    , m_borderOpacity(0.0)
    , m_ipHostLabel(nullptr)
    , m_connTypeLabel(nullptr)
    , m_detailLabel(nullptr)
{
    init();
}

QtETNodeInfo::~QtETNodeInfo()
{
}

void QtETNodeInfo::init()
{
    // 设置固定高度
    setFixedHeight(WIDGET_HEIGHT);
    setMinimumWidth(250);

    // 更新颜色方案
    updateColorScheme();

    // 创建内容布局
    setupContent();

    // 监听系统主题变化
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        updateColorScheme();
        update();
    });
}

void QtETNodeInfo::setupContent()
{
    // 创建内容容器（带边距）
    QWidget *contentWidget = new QWidget(this);
    contentWidget->setContentsMargins(CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN);

    QVBoxLayout *mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(LINE_SPACING);

    // 第一行：IP | hostname 和 连接类型标签
    QHBoxLayout *firstLineLayout = new QHBoxLayout();
    firstLineLayout->setContentsMargins(0, 0, 0, 0);
    firstLineLayout->setSpacing(8);

    // IP和主机名标签（可选中复制）
    m_ipHostLabel = new QLabel(contentWidget);
    m_ipHostLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_ipHostLabel->setFocusPolicy(Qt::NoFocus);
    m_ipHostLabel->setCursor(Qt::ArrowCursor);
    QFont ipHostFont = m_ipHostLabel->font();
    ipHostFont.setPointSize(11);
    ipHostFont.setWeight(QFont::Medium);
    m_ipHostLabel->setFont(ipHostFont);

    // 连接类型标签
    m_connTypeLabel = new QtETConnTypeLabel(m_nodeInfo.connType, contentWidget);

    firstLineLayout->addWidget(m_ipHostLabel, 1);
    firstLineLayout->addWidget(m_connTypeLabel);

    mainLayout->addLayout(firstLineLayout);

    // 第二行：详细信息（蓝色小字）
    m_detailLabel = new QLabel(contentWidget);
    QFont detailFont = m_detailLabel->font();
    detailFont.setPointSize(10);
    m_detailLabel->setFont(detailFont);

    mainLayout->addWidget(m_detailLabel);

    // 设置内容控件的布局
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(contentWidget);

    // 更新内容显示
    updateContent();
}

void QtETNodeInfo::updateColorScheme()
{
    const bool isDark = (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark);

    if (isDark) {
        // 深色模式
        m_bgColor = QColor(45, 45, 45);
        m_borderColor = QColor(70, 70, 70);
        m_highlightBorderColor = palette().color(QPalette::Highlight);
        m_textColor = QColor(220, 220, 220);
        m_detailColor = QColor("#66ccff");
    } else {
        // 浅色模式
        m_bgColor = palette().color(QPalette::Button);
        m_borderColor = palette().color(QPalette::Mid);
        // 如果边框颜色太浅，使用更深的颜色
        if (m_borderColor.lightnessF() > 0.9) {
            m_borderColor = palette().color(QPalette::Dark);
        }
        m_highlightBorderColor = palette().color(QPalette::Highlight);
        m_textColor = palette().color(QPalette::Text);
        m_detailColor = QColor("#66ccff");
    }

    // 更新控件颜色
    if (m_ipHostLabel) {
        m_ipHostLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(m_textColor.name()));
    }
    if (m_detailLabel) {
        m_detailLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(m_detailColor.name()));
    }

    update();
}

void QtETNodeInfo::updateContent()
{
    if (!m_ipHostLabel || !m_connTypeLabel || !m_detailLabel) {
        return;
    }

    // 更新 IP | hostname 显示
    QString ipHostText = QString("%1 | %2").arg(m_nodeInfo.virtualIp, m_nodeInfo.hostname);
    m_ipHostLabel->setText(ipHostText);

    // 更新连接类型
    m_connTypeLabel->setConnType(m_nodeInfo.connType);

    // 构建详细信息字符串
    QStringList detailParts;

    // 延迟
    if (m_nodeInfo.latencyMs >= 0) {
        detailParts << QString("%1ms").arg(m_nodeInfo.latencyMs);
    } else {
        detailParts << QString("--ms");
    }

    // 协议
    if (!m_nodeInfo.protocol.isEmpty()) {
        detailParts << m_nodeInfo.protocol;
    }

    // 连接方式
    if (!m_nodeInfo.connMethod.isEmpty()) {
        detailParts << m_nodeInfo.connMethod;
    }

    m_detailLabel->setText(detailParts.join(" | "));
}

NodeInfo QtETNodeInfo::nodeInfo() const
{
    return m_nodeInfo;
}

void QtETNodeInfo::setNodeInfo(const NodeInfo &info)
{
    m_nodeInfo = info;
    updateContent();
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
}

void QtETNodeInfo::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);

    // 启动边框高亮动画（渐显）
    QPropertyAnimation *animation = new QPropertyAnimation(this, "borderOpacity");
    animation->setDuration(200);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->setStartValue(m_borderOpacity);
    animation->setEndValue(1.0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void QtETNodeInfo::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);

    // 启动边框淡出动画（渐隐）
    QPropertyAnimation *animation = new QPropertyAnimation(this, "borderOpacity");
    animation->setDuration(200);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->setStartValue(m_borderOpacity);
    animation->setEndValue(0.0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}