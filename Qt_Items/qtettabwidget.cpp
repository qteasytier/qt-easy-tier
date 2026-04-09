#include "qtettabwidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleHints>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QLinearGradient>

// ==================== QtETTabWidget 实现 ====================

QtETTabWidget::QtETTabWidget(QWidget *parent)
    : QTabWidget(parent)
    , m_tabBar(nullptr)
{
    init();
}

QtETTabWidget::~QtETTabWidget()
{
    // m_tabBar 由 Qt 对象树自动管理
}

void QtETTabWidget::init()
{
    // 创建自定义 TabBar
    m_tabBar = new QtETTabBar(this);
    setTabBar(m_tabBar);

    // 设置 TabWidget 属性
    setDocumentMode(true);  // 文档模式，移除默认边框
    setUsesScrollButtons(false);
    setTabsClosable(false);
    setMovable(false);

    // 初始化颜色
    updateColorScheme();

    // 监听系统主题变化
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        updateColorScheme();
        update();
    });

    // 监听当前索引变化
    connect(this, &QTabWidget::currentChanged, this, [this]() {
        update();
    });
}

void QtETTabWidget::updateColorScheme()
{
    const bool isDark = (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark);

    if (isDark) {
        m_backgroundColor = QColor(35, 35, 35);
        m_tabBarBackgroundColor = QColor(35, 35, 35);
        m_borderColor = QColor(60, 60, 60);
    } else {
        m_backgroundColor = palette().color(QPalette::Window);
        m_tabBarBackgroundColor = QColor(240, 240, 240);
        m_borderColor = QColor(220, 220, 220);
    }

    update();
}

void QtETTabWidget::setupTabBarStyle()
{
    // TabBar 样式由 QtETTabBar 自行处理
}

void QtETTabWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    constexpr int cornerRadius = 8;

    // 1. 绘制整个控件的圆角背景（四周圆角，统一颜色）
    QPainterPath wholeBgPath;
    wholeBgPath.addRoundedRect(rect(), cornerRadius, cornerRadius);
    painter.fillPath(wholeBgPath, m_backgroundColor);

    // Tab 栏高度
    int tabBarHeight = m_tabBar ? m_tabBar->height() : 36;

    // 2. 绘制分隔线（在 Tab 栏底部）
    painter.setPen(QPen(m_borderColor, 1));
    painter.drawLine(cornerRadius, tabBarHeight, width() - cornerRadius, tabBarHeight);

    // 3. 绘制圆角边框
    painter.setPen(QPen(m_borderColor, 1));
    painter.drawPath(wholeBgPath);
}

// ==================== QtETTabBar 实现 ====================

QtETTabBar::QtETTabBar(QWidget *parent)
    : QTabBar(parent)
    , m_indicatorWidthAnim(nullptr)
    , m_indicatorXAnim(nullptr)
    , m_indicatorAnimGroup(nullptr)
    , m_indicatorWidth(0)
    , m_indicatorX(0)
    , m_hoverOpacity(0.0)
    , m_hoveredTabIndex(-1)
    , m_previousIndex(-1)
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
    // 设置 TabBar 属性
    setExpanding(false);
    setDrawBase(false);
    setUsesScrollButtons(false);
    setDocumentMode(true);
    setElideMode(Qt::ElideNone);

    // 初始化动画组
    m_indicatorAnimGroup = new QParallelAnimationGroup(this);

    // 指示器宽度动画
    m_indicatorWidthAnim = new QPropertyAnimation(this, "indicatorWidth", this);
    m_indicatorWidthAnim->setDuration(ANIMATION_DURATION);
    m_indicatorWidthAnim->setEasingCurve(QEasingCurve::OutCubic);

    // 指示器位置动画
    m_indicatorXAnim = new QPropertyAnimation(this, "indicatorX", this);
    m_indicatorXAnim->setDuration(ANIMATION_DURATION);
    m_indicatorXAnim->setEasingCurve(QEasingCurve::OutCubic);

    m_indicatorAnimGroup->addAnimation(m_indicatorWidthAnim);
    m_indicatorAnimGroup->addAnimation(m_indicatorXAnim);

    // 初始化颜色
    updateColorScheme();

    // 监听系统主题变化
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        updateColorScheme();
        update();
    });

    // 监听当前索引变化
    connect(this, &QTabBar::currentChanged, this, [this](int index) {
        if (index >= 0) {
            animateIndicatorTo(index);
            // 切换后，如果鼠标不在新的 Tab 上，清除悬停状态
            // 这样可以避免之前的 Tab 保持高亮背景
        }
        m_previousIndex = index;
        // 强制完整重绘
        update();
    });

    // 设置固定高度
    setFixedHeight(TAB_HEIGHT);
}

void QtETTabBar::updateColorScheme()
{
    const bool isDark = (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark);

    if (isDark) {
        m_textColor = QColor(220, 220, 220);
        m_textInactiveColor = QColor(140, 140, 140);
        m_indicatorColor = QColor("#66ccff");
        m_hoverBackgroundColor = QColor(60, 60, 60, 120);
        m_selectedBackgroundColor = QColor(70, 70, 70, 50);
    } else {
        m_textColor = QColor(50, 50, 50);
        m_textInactiveColor = QColor(130, 130, 130);
        m_indicatorColor = QColor("#66ccff");
        m_hoverBackgroundColor = QColor(0, 0, 0, 25);
        m_selectedBackgroundColor = QColor(102, 204, 255, 30);
    }

    update();
}

qreal QtETTabBar::indicatorWidth() const
{
    return m_indicatorWidth;
}

void QtETTabBar::setIndicatorWidth(qreal width)
{
    if (!qFuzzyCompare(m_indicatorWidth, width)) {
        m_indicatorWidth = width;
        update();
    }
}

qreal QtETTabBar::indicatorX() const
{
    return m_indicatorX;
}

void QtETTabBar::setIndicatorX(qreal x)
{
    if (!qFuzzyCompare(m_indicatorX, x)) {
        m_indicatorX = x;
        update();
    }
}

qreal QtETTabBar::hoverOpacity() const
{
    return m_hoverOpacity;
}

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

QRect QtETTabBar::calculateTabRect(int index) const
{
    if (index < 0 || index >= count()) {
        return QRect();
    }

    int x = 0;
    for (int i = 0; i < index; ++i) {
        x += getTabTextWidth(i) + TAB_LEFT_PADDING + TAB_RIGHT_PADDING + TAB_SPACING;
    }

    int width = getTabTextWidth(index) + TAB_LEFT_PADDING + TAB_RIGHT_PADDING;
    return QRect(x, 0, width, TAB_HEIGHT);
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

    // 停止当前动画
    m_indicatorAnimGroup->stop();

    // 设置动画起始和结束值
    m_indicatorWidthAnim->setStartValue(m_indicatorWidth);
    m_indicatorWidthAnim->setEndValue(static_cast<qreal>(targetRect.width()));

    m_indicatorXAnim->setStartValue(m_indicatorX);
    m_indicatorXAnim->setEndValue(static_cast<qreal>(targetRect.x()));

    // 启动动画组
    m_indicatorAnimGroup->start();
}

void QtETTabBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制每个 Tab
    for (int i = 0; i < count(); ++i) {
        QRect tabRect = calculateTabRect(i);
        bool isSelected = (i == currentIndex());
        // 选中状态的 tab 不显示悬停背景，避免视觉混乱
        bool isHovered = (i == m_hoveredTabIndex) && !isSelected;

        // 绘制选中或悬停背景
        if (isSelected) {
            // 选中状态背景
            QPainterPath bgPath;
            bgPath.addRoundedRect(tabRect.adjusted(2, 2, -2, -2), BORDER_RADIUS, BORDER_RADIUS);
            painter.fillPath(bgPath, m_selectedBackgroundColor);
        } else if (isHovered) {
            // 悬停状态背景
            QPainterPath bgPath;
            bgPath.addRoundedRect(tabRect.adjusted(2, 2, -2, -2), BORDER_RADIUS, BORDER_RADIUS);
            painter.fillPath(bgPath, m_hoverBackgroundColor);
        }

        // 绘制文字
        QFontMetrics fm(font());
        QString text = tabText(i);
        int textWidth = fm.horizontalAdvance(text);
        int textX = tabRect.x() + (tabRect.width() - textWidth) / 2;
        int textY = (TAB_HEIGHT - fm.height()) / 2 + fm.ascent();

        painter.setFont(font());
        // 悬停时非选中 tab 的文字颜色也变亮
        QColor textColor;
        if (isSelected) {
            textColor = m_textColor;
        } else if (isHovered) {
            // 悬停时使用介于活动和非活动之间的颜色
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

    // 绘制底部指示器（圆角矩形）
    if (count() > 0 && currentIndex() >= 0) {
        // 如果动画未启动，直接计算位置
        if (m_indicatorWidth <= 0) {
            QRect currentRect = calculateTabRect(currentIndex());
            m_indicatorWidth = currentRect.width();
            m_indicatorX = currentRect.x();
        }

        // 指示器区域（底部圆角矩形）
        qreal indicatorHeight = INDICATOR_HEIGHT;
        qreal indicatorY = TAB_HEIGHT - INDICATOR_BOTTOM_MARGIN - indicatorHeight;
        
        // 指示器宽度稍小于 Tab 宽度，更美观
        qreal actualWidth = m_indicatorWidth * 0.7;
        qreal actualX = m_indicatorX + (m_indicatorWidth - actualWidth) / 2;

        QPainterPath indicatorPath;
        indicatorPath.addRoundedRect(
            QRectF(actualX, indicatorY, actualWidth, indicatorHeight),
            indicatorHeight / 2,
            indicatorHeight / 2
        );

        // 绘制指示器
        painter.fillPath(indicatorPath, m_indicatorColor);
    }
}

void QtETTabBar::resizeEvent(QResizeEvent *event)
{
    QTabBar::resizeEvent(event);

    // 更新指示器位置
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

    // 使用自定义的 calculateTabRect 检测点击位置
    int clickedIndex = -1;
    for (int i = 0; i < count(); ++i) {
        QRect tabRect = calculateTabRect(i);
        if (tabRect.contains(event->pos())) {
            clickedIndex = i;
            break;
        }
    }

    if (clickedIndex >= 0 && clickedIndex != currentIndex()) {
        // 清除之前的悬停状态，设置新的悬停索引为点击的索引
        m_hoveredTabIndex = clickedIndex;
        setCurrentIndex(clickedIndex);
    }

    // 接受事件，防止基类处理
    event->accept();
}

void QtETTabBar::mouseMoveEvent(QMouseEvent *event)
{
    int newHoveredIndex = -1;

    // 检测鼠标悬停在哪个 Tab 上
    for (int i = 0; i < count(); ++i) {
        QRect tabRect = calculateTabRect(i);
        if (tabRect.contains(event->pos())) {
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

bool QtETTabBar::event(QEvent *event)
{
    // 处理 Tab 添加/删除事件
    if (event->type() == QEvent::LayoutRequest) {
        // 更新指示器位置
        if (currentIndex() >= 0) {
            QRect currentRect = calculateTabRect(currentIndex());
            m_indicatorWidth = currentRect.width();
            m_indicatorX = currentRect.x();
        }
    }

    return QTabBar::event(event);
}
