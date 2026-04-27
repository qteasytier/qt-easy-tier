#include "qtettabwidget.h"
#include "qtettheme.h"
#include "qtetdrawutils.h"

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleHints>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QKeyEvent>

QtETTabWidget::QtETTabWidget(QWidget *parent)
    : QTabWidget(parent)
{
    init();
}

QtETTabWidget::~QtETTabWidget() = default;

void QtETTabWidget::init()
{
    m_tabBar = new QtETTabBar(this);
    setTabBar(m_tabBar);

    setDocumentMode(true);
    setUsesScrollButtons(false);
    setTabsClosable(false);
    setMovable(false);

    updateColorScheme();

    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        updateColorScheme();
        update();
    });

    connect(this, &QTabWidget::currentChanged, this, [this]() {
        update();
    });
}

void QtETTabWidget::updateColorScheme()
{
    auto *theme = QtETTheme::instance();
    m_backgroundColor = theme->backgroundColor();
    m_tabBarBackgroundColor = theme->backgroundColor();
    m_borderColor = theme->normalBorderColor();
    update();
}

void QtETTabWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    constexpr int cornerRadius = 8;

    QPainterPath wholeBgPath;
    wholeBgPath.addRoundedRect(rect(), cornerRadius, cornerRadius);
    painter.fillPath(wholeBgPath, m_backgroundColor);

    int tabBarHeight = m_tabBar ? m_tabBar->height() : 36;

    painter.setPen(QPen(m_borderColor, 1));
    painter.drawLine(cornerRadius, tabBarHeight, width() - cornerRadius, tabBarHeight);
    painter.drawPath(wholeBgPath);
}

QtETTabBar::QtETTabBar(QWidget *parent)
    : QTabBar(parent)
    , m_isInitialShow(true)
{
    init();
}

QtETTabBar::~QtETTabBar()
{
    if (m_indicatorAnimGroup) {
        m_indicatorAnimGroup->stop();
    }
}

void QtETTabBar::init()
{
    setExpanding(false);
    setDrawBase(false);
    setUsesScrollButtons(false);
    setDocumentMode(true);
    setElideMode(Qt::ElideNone);
    setFocusPolicy(Qt::StrongFocus);

    m_indicatorAnimGroup = new QParallelAnimationGroup(this);

    m_indicatorWidthAnim = new QPropertyAnimation(this, "indicatorWidth", this);
    m_indicatorWidthAnim->setDuration(ANIMATION_DURATION);
    m_indicatorWidthAnim->setEasingCurve(QEasingCurve::OutCubic);

    m_indicatorXAnim = new QPropertyAnimation(this, "indicatorX", this);
    m_indicatorXAnim->setDuration(ANIMATION_DURATION);
    m_indicatorXAnim->setEasingCurve(QEasingCurve::OutCubic);

    m_indicatorAnimGroup->addAnimation(m_indicatorWidthAnim);
    m_indicatorAnimGroup->addAnimation(m_indicatorXAnim);

    updateColorScheme();

    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        updateColorScheme();
        update();
    });

    connect(this, &QTabBar::currentChanged, this, [this](int index) {
        if (index >= 0) {
            animateIndicatorTo(index);
        }
        m_previousIndex = index;
        update();
    });

    setFixedHeight(TAB_HEIGHT);
}

void QtETTabBar::updateColorScheme()
{
    auto *theme = QtETTheme::instance();
    m_textColor = theme->textColor();
    m_textInactiveColor = theme->textInactiveColor();
    m_indicatorColor = theme->highlightBorderColor();
    m_hoverBackgroundColor = theme->hoverBackgroundColor();
    m_selectedBackgroundColor = theme->selectedBackgroundColor();
    update();
}

qreal QtETTabBar::indicatorWidth() const { return m_indicatorWidth; }
void QtETTabBar::setIndicatorWidth(qreal width)
{
    if (!qFuzzyCompare(m_indicatorWidth, width)) {
        m_indicatorWidth = width;
        update();
    }
}

qreal QtETTabBar::indicatorX() const { return m_indicatorX; }
void QtETTabBar::setIndicatorX(qreal x)
{
    if (!qFuzzyCompare(m_indicatorX, x)) {
        m_indicatorX = x;
        update();
    }
}

qreal QtETTabBar::hoverOpacity() const { return m_hoverOpacity; }
void QtETTabBar::setHoverOpacity(qreal opacity)
{
    if (!qFuzzyCompare(m_hoverOpacity, opacity)) {
        m_hoverOpacity = opacity;
        update();
    }
}

QSize QtETTabBar::sizeHint() const
{
    int totalWidth = 0;
    for (int i = 0; i < count(); ++i) {
        totalWidth += getTabTextWidth(i) + TAB_LEFT_PADDING + TAB_RIGHT_PADDING;
    }
    return QSize(totalWidth, TAB_HEIGHT);
}

void QtETTabBar::recalculateTabWidths()
{
    m_tabTextWidths.resize(count());
    m_tabCumulativeWidths.resize(count());

    int cumulative = 0;
    for (int i = 0; i < count(); ++i) {
        int textWidth = getTabTextWidth(i);
        int tabWidth = textWidth + TAB_LEFT_PADDING + TAB_RIGHT_PADDING;
        m_tabTextWidths[i] = textWidth;
        m_tabCumulativeWidths[i] = cumulative;
        cumulative += tabWidth;
    }
}

QRect QtETTabBar::calculateTabRect(int index) const
{
    if (index < 0 || index >= count()) {
        return QRect();
    }

    if (m_tabTextWidths.size() != count()) {
        const_cast<QtETTabBar*>(this)->recalculateTabWidths();
    }

    int width = m_tabTextWidths[index] + TAB_LEFT_PADDING + TAB_RIGHT_PADDING;
    return QRect(m_tabCumulativeWidths[index], 0, width, TAB_HEIGHT);
}

int QtETTabBar::getTabTextWidth(int index) const
{
    if (index < 0 || index >= count()) {
        return 0;
    }
    QFontMetrics fm(font());
    return fm.horizontalAdvance(tabText(index));
}

void QtETTabBar::animateIndicatorTo(int index)
{
    if (index < 0 || index >= count()) {
        return;
    }

    QRect targetRect = calculateTabRect(index);

    m_indicatorAnimGroup->stop();

    if (m_isInitialShow) {
        m_indicatorWidth = targetRect.width();
        m_indicatorX = targetRect.x();
        m_isInitialShow = false;
    }

    m_indicatorWidthAnim->setStartValue(m_indicatorWidth);
    m_indicatorWidthAnim->setEndValue(static_cast<qreal>(targetRect.width()));

    m_indicatorXAnim->setStartValue(m_indicatorX);
    m_indicatorXAnim->setEndValue(static_cast<qreal>(targetRect.x()));

    m_indicatorAnimGroup->start();
}

void QtETTabBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_tabTextWidths.size() != count()) {
        recalculateTabWidths();
    }

    for (int i = 0; i < count(); ++i) {
        QRect tabRect = calculateTabRect(i);
        bool isSelected = (i == currentIndex());
        bool isHovered = (i == m_hoveredTabIndex) && !isSelected;

        if (isSelected) {
            QPainterPath bgPath;
            bgPath.addRoundedRect(tabRect.adjusted(2, 2, -2, -2), BORDER_RADIUS, BORDER_RADIUS);
            painter.fillPath(bgPath, m_selectedBackgroundColor);
        } else if (isHovered) {
            QPainterPath bgPath;
            bgPath.addRoundedRect(tabRect.adjusted(2, 2, -2, -2), BORDER_RADIUS, BORDER_RADIUS);
            painter.fillPath(bgPath, m_hoverBackgroundColor);
        }

        QFontMetrics fm(font());
        QString text = tabText(i);
        int textWidth = fm.horizontalAdvance(text);
        int textX = tabRect.x() + (tabRect.width() - textWidth) / 2;
        int textY = (TAB_HEIGHT - fm.height()) / 2 + fm.ascent();

        painter.setFont(font());
        QColor textColor;
        if (isSelected) {
            textColor = m_textColor;
        } else if (isHovered) {
            textColor = QColor(
                (m_textColor.red() + m_textInactiveColor.red()) / 2,
                (m_textColor.green() + m_textInactiveColor.green()) / 2,
                (m_textColor.blue() + m_textInactiveColor.blue()) / 2
            );
        } else {
            textColor = m_textInactiveColor;
        }
        painter.setPen(textColor);
        painter.drawText(textX, textY, text);
    }

    if (count() > 0 && currentIndex() >= 0) {
        if (m_isInitialShow) {
            QRect currentRect = calculateTabRect(currentIndex());
            m_indicatorWidth = currentRect.width();
            m_indicatorX = currentRect.x();
            m_isInitialShow = false;
        }

        qreal indicatorHeight = INDICATOR_HEIGHT;
        qreal indicatorY = TAB_HEIGHT - indicatorHeight;
        qreal actualWidth = m_indicatorWidth * 0.7;
        qreal actualX = m_indicatorX + (m_indicatorWidth - actualWidth) / 2;

        QtETDrawUtils::drawBottomIndicator(&painter, actualX, indicatorY, actualWidth, indicatorHeight, m_indicatorColor);
    }
}

void QtETTabBar::resizeEvent(QResizeEvent *event)
{
    QTabBar::resizeEvent(event);
    recalculateTabWidths();
    if (currentIndex() >= 0) {
        QRect currentRect = calculateTabRect(currentIndex());
        m_indicatorWidth = currentRect.width();
        m_indicatorX = currentRect.x();
    }
    update();
}

void QtETTabBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QTabBar::mousePressEvent(event);
        return;
    }

    int clickedIndex = -1;
    for (int i = 0; i < count(); ++i) {
        if (calculateTabRect(i).contains(event->pos())) {
            clickedIndex = i;
            break;
        }
    }

    if (clickedIndex >= 0 && clickedIndex != currentIndex()) {
        m_hoveredTabIndex = clickedIndex;
        setCurrentIndex(clickedIndex);
    }

    event->accept();
}

void QtETTabBar::mouseMoveEvent(QMouseEvent *event)
{
    int newHoveredIndex = -1;
    for (int i = 0; i < count(); ++i) {
        if (calculateTabRect(i).contains(event->pos())) {
            newHoveredIndex = i;
            break;
        }
    }

    if (newHoveredIndex != m_hoveredTabIndex) {
        m_hoveredTabIndex = newHoveredIndex;
        update();
    }

    QTabBar::mouseMoveEvent(event);
}

void QtETTabBar::leaveEvent(QEvent *event)
{
    m_hoveredTabIndex = -1;
    update();
    QTabBar::leaveEvent(event);
}

void QtETTabBar::keyPressEvent(QKeyEvent *event)
{
    int current = currentIndex();
    int newIndex = current;

    switch (event->key()) {
    case Qt::Key_Left:
        newIndex = qMax(0, current - 1);
        break;
    case Qt::Key_Right:
        newIndex = qMin(count() - 1, current + 1);
        break;
    case Qt::Key_Home:
        newIndex = 0;
        break;
    case Qt::Key_End:
        newIndex = count() - 1;
        break;
    default:
        QTabBar::keyPressEvent(event);
        return;
    }

    if (newIndex != current) {
        setCurrentIndex(newIndex);
    }
    event->accept();
}

bool QtETTabBar::event(QEvent *event)
{
    if (event->type() == QEvent::LayoutRequest) {
        recalculateTabWidths();
        if (currentIndex() >= 0) {
            QRect currentRect = calculateTabRect(currentIndex());
            m_indicatorWidth = currentRect.width();
            m_indicatorX = currentRect.x();
        }
    }
    return QTabBar::event(event);
}
