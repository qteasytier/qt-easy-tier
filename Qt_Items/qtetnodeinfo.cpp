#include "qtetnodeinfo.h"
#include "qtettheme.h"
#include "qtetdrawutils.h"

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleHints>
#include <QFontMetrics>
#include <QMenu>
#include <QClipboard>
#include <QTextOption>

QtETNodeInfo::QtETNodeInfo(QWidget *parent)
    : QWidget(parent)
    , m_borderAnimation(nullptr)
{
    init();
}

QtETNodeInfo::QtETNodeInfo(const NodeInfo &info, QWidget *parent)
    : QWidget(parent)
    , m_nodeInfo(info)
    , m_borderAnimation(nullptr)
{
    init();
}

void QtETNodeInfo::init()
{
    setFixedHeight(WIDGET_HEIGHT);
    setMinimumWidth(250);

    m_borderAnimation = new QPropertyAnimation(this, "borderOpacity", this);
    m_borderAnimation->setDuration(200);
    m_borderAnimation->setEasingCurve(QEasingCurve::OutCubic);

    updateColorScheme();

    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        updateColorScheme();
        update();
    });
}

void QtETNodeInfo::updateColorScheme()
{
    auto *theme = QtETTheme::instance();
    m_bgColor = theme->widgetBackgroundColor();
    m_borderColor = theme->normalBorderColor();
    m_highlightBorderColor = theme->highlightBorderColor();
    m_textColor = theme->textColor();
    m_detailColor = theme->highlightBorderColor();
    update();
}

QColor QtETNodeInfo::getConnTypeColor(NodeConnType type)
{
    switch (type) {
    case NodeConnType::Direct: return QtETTheme::ConnDirect;
    case NodeConnType::Relay: return QtETTheme::ConnRelay;
    case NodeConnType::Server: return QtETTheme::ConnServer;
    }
    return QtETTheme::ConnDirect;
}

QString QtETNodeInfo::getConnTypeText(NodeConnType type)
{
    switch (type) {
    case NodeConnType::Direct: return QObject::tr("直连");
    case NodeConnType::Relay: return QObject::tr("中转");
    case NodeConnType::Server: return QObject::tr("服务器");
    }
    return QObject::tr("直连");
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

qreal QtETNodeInfo::borderOpacity() const { return m_borderOpacity; }
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

int QtETNodeInfo::calculateLabelsWidth() const
{
    QFont labelFont = font();
    labelFont.setPointSize(LABEL_FONT_SIZE);
    labelFont.setWeight(QFont::Bold);
    QFontMetrics fm(labelFont);

    int totalWidth = 0;

    // 连接类型标签
    QString connText = getConnTypeText(m_nodeInfo.connType);
    totalWidth += fm.horizontalAdvance(connText) + 2 * LABEL_PADDING_H + LABEL_SPACING;

    // 本机标签
    if (m_nodeInfo.isLocalNode) {
        QString localText = tr("本机");
        totalWidth += fm.horizontalAdvance(localText) + 2 * LABEL_PADDING_H + LABEL_SPACING;
    }

    return totalWidth;
}

void QtETNodeInfo::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect borderRect = rect().adjusted(1, 1, -1, -1);

    QColor finalBorderColor = QtETDrawUtils::blendColors(m_borderColor, m_highlightBorderColor, m_borderOpacity);

    QtETDrawUtils::drawRoundedRect(&painter, borderRect, BORDER_RADIUS, m_bgColor, finalBorderColor, 1);

    int contentLeft = CONTENT_MARGIN;
    int contentTop = CONTENT_MARGIN;
    int contentWidth = width() - 2 * CONTENT_MARGIN;

    QFont ipHostFont = font();
    ipHostFont.setPointSize(11);
    ipHostFont.setWeight(QFont::Medium);
    QFontMetrics ipHostFm(ipHostFont);
    int firstLineHeight = ipHostFm.height();

    bool showIp = !m_nodeInfo.virtualIp.isEmpty() && m_nodeInfo.virtualIp != "0.0.0.0";

    QString ipHostText = showIp
                             ? QString("%1 | %2").arg(m_nodeInfo.virtualIp, m_nodeInfo.hostname)
                             : m_nodeInfo.hostname;

    // 计算可用文本宽度（减去标签占用空间 + 额外安全边距）
    int labelsWidth = calculateLabelsWidth();
    int availableTextWidth = contentWidth - labelsWidth - 10;

    QString elidedText = ipHostFm.elidedText(ipHostText, Qt::ElideRight, availableTextWidth);

    painter.setFont(ipHostFont);
    painter.setPen(m_textColor);
    int ipHostY = contentTop + ipHostFm.ascent();
    painter.drawText(contentLeft, ipHostY, elidedText);

    int labelX = contentLeft + contentWidth;
    int labelY = contentTop + (firstLineHeight - (LABEL_PADDING_V * 2 + LABEL_FONT_SIZE + 2)) / 2;

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
        painter.fillPath(localPath, QtETTheme::ConnLocal);

        painter.setFont(labelFont);
        painter.setPen(Qt::white);
        int textX = labelX + (localWidth - labelFm.horizontalAdvance(localText)) / 2;
        int textY = labelY + LABEL_PADDING_V + labelFm.ascent();
        painter.drawText(textX, textY, localText);
    }

    QString connText = getConnTypeText(m_nodeInfo.connType);
    int connWidth = labelFm.horizontalAdvance(connText) + 2 * LABEL_PADDING_H;
    int connHeight = labelFm.height() + 2 * LABEL_PADDING_V;

    labelX -= connWidth;

    QPainterPath connPath;
    connPath.addRoundedRect(labelX, labelY, connWidth, connHeight,
                             LABEL_BORDER_RADIUS, LABEL_BORDER_RADIUS);
    painter.fillPath(connPath, getConnTypeColor(m_nodeInfo.connType));

    painter.setFont(labelFont);
    painter.setPen(Qt::white);
    int connTextX = labelX + (connWidth - labelFm.horizontalAdvance(connText)) / 2;
    int connTextY = labelY + LABEL_PADDING_V + labelFm.ascent();
    painter.drawText(connTextX, connTextY, connText);

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
    m_borderAnimation->stop();
    m_borderAnimation->setStartValue(m_borderOpacity);
    m_borderAnimation->setEndValue(1.0);
    m_borderAnimation->start();
}

void QtETNodeInfo::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    m_borderAnimation->stop();
    m_borderAnimation->setStartValue(m_borderOpacity);
    m_borderAnimation->setEndValue(0.0);
    m_borderAnimation->start();
}

void QtETNodeInfo::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void QtETNodeInfo::contextMenuEvent(QContextMenuEvent *event)
{
    bool showIp = !m_nodeInfo.virtualIp.isEmpty() && m_nodeInfo.virtualIp != "0.0.0.0";

    QMenu menu(this);

    if (showIp) {
        QAction *copyIpAction = menu.addAction(tr("复制地址"));
        connect(copyIpAction, &QAction::triggered, this, [this]() {
            QApplication::clipboard()->setText(m_nodeInfo.virtualIp);
        });
    }

    QAction *copyHostnameAction = menu.addAction(tr("复制设备名"));
    connect(copyHostnameAction, &QAction::triggered, this, [this]() {
        QApplication::clipboard()->setText(m_nodeInfo.hostname);
    });

    menu.exec(event->globalPos());
}
