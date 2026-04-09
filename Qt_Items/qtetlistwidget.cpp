#include "qtetlistwidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleHints>
#include <QAbstractItemView>
#include <QWheelEvent>
#include <QEasingCurve>

// ============================================================================
// QtETScrollBar 实现
// ============================================================================

QtETScrollBar::QtETScrollBar(Qt::Orientation orientation, QWidget *parent)
    : QScrollBar(orientation, parent)
    , m_scrollBarWidth(DEFAULT_WIDTH)
    , m_fadeDuration(DEFAULT_FADE_DURATION)
    , m_opacity(0.0)
    , m_isHovered(false)
    , m_fadeAnimation(nullptr)
    , m_handleColor()
    , m_grooveColor()
{
    // 隐藏默认滚动条样式
    setStyleSheet(QStringLiteral(R"(
        QScrollBar {
            background: transparent;
            border: none;
            margin: 0px;
        }
        QScrollBar::handle {
            background: transparent;
            border: none;
        }
        QScrollBar::add-line, QScrollBar::sub-line {
            height: 0px;
            width: 0px;
        }
        QScrollBar::add-page, QScrollBar::sub-page {
            background: transparent;
        }
    )"));

    // 设置固定宽度
    setFixedWidth(m_scrollBarWidth);

    // 初始化颜色方案
    updateColorScheme();

    // 创建透明度动画
    m_fadeAnimation = new QPropertyAnimation(this, "opacity", this);
    m_fadeAnimation->setDuration(m_fadeDuration);
    m_fadeAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // 初始状态为隐藏
    m_opacity = 0.0;
}

void QtETScrollBar::setScrollBarWidth(int width)
{
    if (width > 0 && m_scrollBarWidth != width) {
        m_scrollBarWidth = width;
        setFixedWidth(m_scrollBarWidth);
        update();
    }
}

int QtETScrollBar::scrollBarWidth() const
{
    return m_scrollBarWidth;
}

void QtETScrollBar::setFadeDuration(int duration)
{
    m_fadeDuration = duration;
    if (m_fadeAnimation) {
        m_fadeAnimation->setDuration(m_fadeDuration);
    }
}

int QtETScrollBar::fadeDuration() const
{
    return m_fadeDuration;
}

void QtETScrollBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    // 如果完全透明则不绘制
    if (m_opacity < 0.01) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(m_opacity);

    // 更新颜色方案
    updateColorScheme();

    // 计算滑块区域
    int sliderMin = minimum();
    int sliderMax = maximum();
    int sliderRange = sliderMax - sliderMin;

    if (sliderRange <= 0) {
        return;
    }

    QRect grooveRect = rect();

    if (orientation() == Qt::Vertical) {
        // 垂直滚动条
        int sliderPos = sliderPosition() - sliderMin;
        int sliderLen = pageStep();

        // 计算滑块高度和位置
        int grooveHeight = grooveRect.height();
        qreal ratio = static_cast<qreal>(sliderLen) / (sliderRange + sliderLen);
        int handleHeight = qMax(static_cast<int>(grooveHeight * ratio), 20);  // 最小高度 20px
        int handleY = static_cast<int>((grooveHeight - handleHeight) * sliderPos / sliderRange);

        QRect handleRect(grooveRect.left(), handleY, grooveRect.width(), handleHeight);

        // 绘制轨道背景（非常淡）
        QPainterPath groovePath;
        groovePath.addRoundedRect(grooveRect, grooveRect.width() / 2, grooveRect.width() / 2);
        QColor grooveColor = m_grooveColor;
        grooveColor.setAlphaF(grooveColor.alphaF() * m_opacity * 0.3);
        painter.fillPath(groovePath, grooveColor);

        // 绘制滑块
        QPainterPath handlePath;
        handlePath.addRoundedRect(handleRect, handleRect.width() / 2, handleRect.width() / 2);

        QColor handleColor = m_handleColor;
        if (m_isHovered) {
            // 悬停时稍微加深
            handleColor = handleColor.darker(110);
        }
        painter.fillPath(handlePath, handleColor);
    } else {
        // 水平滚动条（暂不支持）
        QScrollBar::paintEvent(event);
    }
}

void QtETScrollBar::enterEvent(QEnterEvent *event)
{
    QScrollBar::enterEvent(event);
    m_isHovered = true;
    update();
}

void QtETScrollBar::leaveEvent(QEvent *event)
{
    QScrollBar::leaveEvent(event);
    m_isHovered = false;
    update();
}

void QtETScrollBar::sliderChange(SliderChange change)
{
    QScrollBar::sliderChange(change);

    if (change == SliderChange::SliderValueChange) {
        // 滚动时显示滚动条
        startShowAnimation();
    }
}

void QtETScrollBar::updateColorScheme()
{
    const bool isDark = (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark);

    if (isDark) {
        // 深色模式
        m_handleColor = QColor(120, 120, 120);
        m_grooveColor = QColor(60, 60, 60);
    } else {
        // 浅色模式
        m_handleColor = QColor(180, 180, 180);
        m_grooveColor = QColor(200, 200, 200);
    }
}

void QtETScrollBar::startShowAnimation()
{
    if (m_fadeAnimation->state() == QPropertyAnimation::Running) {
        m_fadeAnimation->stop();
    }

    m_fadeAnimation->setStartValue(m_opacity);
    m_fadeAnimation->setEndValue(1.0);
    m_fadeAnimation->start();
}

void QtETScrollBar::startHideAnimation()
{
    if (m_fadeAnimation->state() == QPropertyAnimation::Running) {
        m_fadeAnimation->stop();
    }

    m_fadeAnimation->setStartValue(m_opacity);
    m_fadeAnimation->setEndValue(0.0);
    m_fadeAnimation->start();
}

void QtETScrollBar::setOpacity(qreal opacity)
{
    if (!qFuzzyCompare(m_opacity, opacity)) {
        m_opacity = opacity;
        update();
    }
}

qreal QtETScrollBar::opacity() const
{
    return m_opacity;
}

// ============================================================================
// QtETListWidgetItem 实现
// ============================================================================

QtETListWidgetItem::QtETListWidgetItem(QListWidget *parent, int type)
    : QListWidgetItem(parent, type)
    , m_detailText()
{
}

QtETListWidgetItem::QtETListWidgetItem(const QString &text, QListWidget *parent, int type)
    : QListWidgetItem(text, parent, type)
    , m_detailText()
{
}

QtETListWidgetItem::QtETListWidgetItem(const QIcon &icon, const QString &text,
                                       QListWidget *parent, int type)
    : QListWidgetItem(icon, text, parent, type)
    , m_detailText()
{
}

QtETListWidgetItem::QtETListWidgetItem(const QtETListWidgetItem &other)
    : QListWidgetItem(other)
    , m_detailText(other.m_detailText)
{
}

QtETListWidgetItem &QtETListWidgetItem::operator=(const QtETListWidgetItem &other)
{
    QListWidgetItem::operator=(other);
    m_detailText = other.m_detailText;
    return *this;
}

void QtETListWidgetItem::setDetailText(const QString &text)
{
    m_detailText = text;
    setData(Qt::UserRole, text);
}

QString QtETListWidgetItem::detailText() const
{
    return m_detailText;
}

// ============================================================================
// QtETListWidgetDelegate 实现
// ============================================================================

QtETListWidgetDelegate::QtETListWidgetDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
    , m_highlightColor("#66ccff")
    , m_hoverFillColor()
    , m_selectedFillColor()
    , m_normalBorderColor()
    , m_backgroundColor()
    , m_textColor()
    , m_detailTextColor()
    , m_itemHeight(45)  // 默认项目高度 45px
    , m_rightMargin(0)  // 默认右边距为 0
{
    // 初始化颜色方案
    updateColorScheme();
}

void QtETListWidgetDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // 更新颜色方案（响应深浅模式变化）
    updateColorScheme();

    // 判断状态
    const bool isSelected = option.state & QStyle::State_Selected;
    const bool isHovered = option.state & QStyle::State_MouseOver;

    // 只有悬停或选中时才绘制高亮效果
    if (isHovered || isSelected) {
        // 计算项目矩形（考虑与 Widget 边框的距离和滚动条右边距）
        QRect itemRect = option.rect.adjusted(4, 2, -4 - m_rightMargin, -2);

        // 绘制圆角高亮背景
        QPainterPath path;
        path.addRoundedRect(itemRect, BORDER_RADIUS, BORDER_RADIUS);

        // 根据状态设置背景和边框
        QColor bgColor;
        QColor borderColor;

        if (isSelected) {
            // 选中状态：高亮色边框 + 淡色填充
            borderColor = m_highlightColor;
            bgColor = m_selectedFillColor;
        } else {
            // 悬停状态：高亮色边框 + 更淡填充
            borderColor = m_highlightColor;
            bgColor = m_hoverFillColor;
        }

        // 绘制背景
        painter->fillPath(path, bgColor);

        // 绘制边框
        painter->setPen(QPen(borderColor, 1));
        painter->drawPath(path);
    }

    // 绘制图标
    QRect itemRect = option.rect.adjusted(4, 2, -4 - m_rightMargin, -2);
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    int contentLeft = itemRect.left() + ITEM_PADDING_H;
    if (!icon.isNull()) {
        int iconY = itemRect.top() + (itemRect.height() - ICON_SIZE) / 2;
        QRect iconRect(contentLeft, iconY, ICON_SIZE, ICON_SIZE);
        icon.paint(painter, iconRect);
        contentLeft += ICON_SIZE + ICON_TEXT_SPACING;
    }

    // 获取文字数据
    QString text = index.data(Qt::DisplayRole).toString();
    QString detailText = index.data(Qt::UserRole).toString();  // 详细文字

    // 计算垂直布局
    int textAreaWidth = itemRect.right() - contentLeft - ITEM_PADDING_H;
    bool hasDetail = !detailText.isEmpty();

    // 设置主文字字体（使用 widget 的字体）
    QFont mainFont = option.widget ? option.widget->font() : painter->font();
    painter->setFont(mainFont);
    painter->setPen(m_textColor);

    QFontMetrics mainFm(mainFont);
    int mainTextHeight = mainFm.height();

    int textTop;
    if (hasDetail) {
        // 有详细文字时，主文字在上
        textTop = itemRect.top() + ITEM_PADDING_V;
    } else {
        // 无详细文字时，垂直居中
        textTop = itemRect.top() + (itemRect.height() - mainTextHeight) / 2;
    }

    QRect textRect(contentLeft, textTop, textAreaWidth, mainTextHeight);
    QString elidedText = mainFm.elidedText(text, Qt::ElideRight, textAreaWidth);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    // 绘制详细文字
    if (hasDetail) {
        // 详细文字使用相同字体，但字号略小
        QFont detailFont = mainFont;
        detailFont.setPointSize(mainFont.pointSize() - 1);
        painter->setFont(detailFont);
        painter->setPen(m_detailTextColor);  // #66ccff

        QFontMetrics detailFm(detailFont);
        int detailTop = textTop + mainTextHeight + LINE_SPACING;
        QRect detailRect(contentLeft, detailTop, textAreaWidth, detailFm.height());
        QString elidedDetail = detailFm.elidedText(detailText, Qt::ElideRight, textAreaWidth);
        painter->drawText(detailRect, Qt::AlignLeft | Qt::AlignVCenter, elidedDetail);
    }

    painter->restore();
}

QSize QtETListWidgetDelegate::sizeHint(const QStyleOptionViewItem &option,
                                       const QModelIndex &index) const
{
    // 使用 widget 的字体计算高度
    QFont font = option.widget ? option.widget->font() : QFont();
    QFontMetrics fm(font);
    int fontHeight = fm.height();

    // 检查是否有详细文字
    QString detailText = index.data(Qt::UserRole).toString();
    bool hasDetail = !detailText.isEmpty();

    // 基础宽度
    int width = ICON_SIZE + ICON_TEXT_SPACING + 150 + ITEM_PADDING_H * 2 + 8;

    // 高度：字体高度 + 6px
    int height = fontHeight + 6;
    if (hasDetail) {
        // 有详细文字时增加高度
        QFont detailFont = font;
        detailFont.setPointSize(font.pointSize() - 1);
        QFontMetrics detailFm(detailFont);
        height += detailFm.height() + LINE_SPACING;
    }

    return QSize(width, height);
}

void QtETListWidgetDelegate::setHighlightColor(const QColor &color)
{
    m_highlightColor = color;
}

QColor QtETListWidgetDelegate::highlightColor() const
{
    return m_highlightColor;
}

void QtETListWidgetDelegate::setItemHeight(int height)
{
    if (height > 0 && m_itemHeight != height) {
        m_itemHeight = height;
    }
}

int QtETListWidgetDelegate::itemHeight() const
{
    return m_itemHeight;
}

void QtETListWidgetDelegate::setRightMargin(int margin)
{
    if (margin >= 0 && m_rightMargin != margin) {
        m_rightMargin = margin;
    }
}

int QtETListWidgetDelegate::rightMargin() const
{
    return m_rightMargin;
}

void QtETListWidgetDelegate::updateColorScheme() const
{
    const bool isDark = (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark);

    if (isDark) {
        // 深色模式
        m_backgroundColor = QColor(45, 45, 45);
        m_normalBorderColor = QColor(70, 70, 70);
        m_highlightColor = QColor("#66ccff");
        m_textColor = QColor(220, 220, 220);
        m_detailTextColor = QColor("#66ccff");
        m_hoverFillColor = QColor(55, 55, 55);
        m_selectedFillColor = QColor(50, 60, 70);
    } else {
        // 浅色模式
        m_backgroundColor = QColor(250, 250, 250);
        m_normalBorderColor = QColor(200, 200, 200);
        m_highlightColor = QColor("#66ccff");
        m_textColor = QColor(40, 40, 40);
        m_detailTextColor = QColor("#66ccff");
        m_hoverFillColor = QColor(240, 250, 255);
        m_selectedFillColor = QColor(230, 245, 255);
    }
}

// ============================================================================
// QtETListWidget 实现
// ============================================================================

QtETListWidget::QtETListWidget(QWidget *parent)
    : QListWidget(parent)
    , m_delegate(nullptr)
    , m_scrollBar(nullptr)
    , m_emptyText(tr("暂无数据"))
    , m_listBorderColor()
    , m_listBackgroundColor()
    , m_scrollAnimation(nullptr)
    , m_smoothScrollDuration(300)
{
    init();
}

QtETListWidget::~QtETListWidget()
{
}

void QtETListWidget::init()
{
    // 设置自定义委托
    m_delegate = new QtETListWidgetDelegate(this);
    setItemDelegate(m_delegate);

    // 设置列表属性
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setMouseTracking(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 隐藏默认垂直滚动条，使用自定义滚动条
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 初始化自定义滚动条
    initScrollBar();

    // 初始化平滑滚动
    m_scrollAnimation = new QPropertyAnimation(verticalScrollBar(), "value", this);
    m_scrollAnimation->setDuration(300);  // 平滑滚动时长 300ms
    m_scrollAnimation->setEasingCurve(QEasingCurve::OutCubic);
    m_smoothScrollDuration = 300;

    // 去除默认的选中框和背景
    setStyleSheet(QStringLiteral(R"(
        QListWidget {
            outline: none;
            border: none;
            background: transparent;
        }
        QListWidget::item {
            background: transparent;
            border: none;
            padding: 0px;
        }
    )"));

    // 更新颜色方案
    updateColorScheme();

    // 监听系统主题变化
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        updateColorScheme();
        viewport()->update();
    });

    // 监听内容变化，更新滚动条
    connect(model(), &QAbstractItemModel::rowsInserted, this, &QtETListWidget::updateScrollBar);
    connect(model(), &QAbstractItemModel::rowsRemoved, this, &QtETListWidget::updateScrollBar);
    connect(model(), &QAbstractItemModel::dataChanged, this, &QtETListWidget::updateScrollBar);
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &QtETListWidget::updateScrollBar);
}

void QtETListWidget::updateColorScheme()
{
    const bool isDark = (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark);

    if (isDark) {
        // 深色模式
        m_listBackgroundColor = QColor(35, 35, 35);
        m_listBorderColor = QColor(60, 60, 60);
    } else {
        // 浅色模式
        m_listBackgroundColor = QColor(245, 245, 245);
        m_listBorderColor = QColor(180, 180, 180);
    }
}

void QtETListWidget::setEmptyText(const QString &text)
{
    m_emptyText = text;
    viewport()->update();
}

QString QtETListWidget::emptyText() const
{
    return m_emptyText;
}

void QtETListWidget::setHighlightColor(const QColor &color)
{
    m_delegate->setHighlightColor(color);
    viewport()->update();
}

QColor QtETListWidget::highlightColor() const
{
    return m_delegate->highlightColor();
}

void QtETListWidget::setItemHeight(int height)
{
    m_delegate->setItemHeight(height);
    // 触发重新布局
    for (int i = 0; i < count(); ++i) {
        update(indexFromItem(item(i)));
    }
}

int QtETListWidget::itemHeight() const
{
    return m_delegate->itemHeight();
}

void QtETListWidget::paintEvent(QPaintEvent *event)
{
    // 绘制列表整体边框和背景
    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing);

    // 计算绘制区域，考虑滚动条宽度
    int rightMargin = 0;
    if (m_scrollBar && m_scrollBar->isVisible()) {
        rightMargin = m_scrollBar->scrollBarWidth() + SCROLL_BAR_MARGIN;
    }

    QRect borderRect = rect().adjusted(1, 1, -1 - rightMargin, -1);

    // 绘制列表背景
    QPainterPath bgPath;
    bgPath.addRoundedRect(borderRect, LIST_BORDER_RADIUS, LIST_BORDER_RADIUS);
    painter.fillPath(bgPath, m_listBackgroundColor);

    // 绘制列表边框
    painter.setPen(QPen(m_listBorderColor, LIST_BORDER_WIDTH));
    painter.drawPath(bgPath);

    // 调用父类绘制项目
    painter.end();
    QListWidget::paintEvent(event);

    // 空状态提示
    if (count() == 0) {
        QPainter emptyPainter(viewport());
        emptyPainter.setRenderHint(QPainter::Antialiasing);

        QFont font = emptyPainter.font();
        font.setPointSize(14);
        emptyPainter.setFont(font);

        QColor textColor = palette().color(QPalette::Text);
        textColor.setAlphaF(0.4);
        emptyPainter.setPen(textColor);

        emptyPainter.drawText(viewport()->rect(), Qt::AlignCenter, m_emptyText);
    }
}

void QtETListWidget::resizeEvent(QResizeEvent *event)
{
    QListWidget::resizeEvent(event);
    updateScrollBar();
    viewport()->update();
}

void QtETListWidget::wheelEvent(QWheelEvent *event)
{
    // 平滑滚动处理
    int delta = -event->angleDelta().y();
    startSmoothScroll(delta);
    event->accept();
}

void QtETListWidget::initScrollBar()
{
    m_scrollBar = new QtETScrollBar(Qt::Vertical, this);
    m_scrollBar->hide();

    // 连接滚动条信号
    connect(m_scrollBar, &QScrollBar::valueChanged, verticalScrollBar(), &QScrollBar::setValue);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, m_scrollBar, &QScrollBar::setValue);
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, m_scrollBar, &QScrollBar::setRange);
}

void QtETListWidget::updateScrollBar()
{
    if (!m_scrollBar) {
        return;
    }

    int min = verticalScrollBar()->minimum();
    int max = verticalScrollBar()->maximum();

    if (min == max) {
        // 内容不需要滚动，隐藏滚动条
        m_scrollBar->hide();
        m_scrollBar->startHideAnimation();

        // 清除委托的右边距
        if (m_delegate) {
            m_delegate->setRightMargin(0);
            viewport()->update();
        }
    } else {
        // 内容需要滚动，显示滚动条
        // 计算滚动条位置，确保不覆盖边框
        int scrollBarX = width() - m_scrollBar->scrollBarWidth() - SCROLL_BAR_MARGIN - LIST_BORDER_WIDTH;
        int scrollBarY = SCROLL_BAR_MARGIN + LIST_BORDER_WIDTH;
        int scrollBarHeight = height() - SCROLL_BAR_MARGIN * 2 - LIST_BORDER_WIDTH * 2;

        m_scrollBar->setGeometry(scrollBarX, scrollBarY,
                                 m_scrollBar->scrollBarWidth(), scrollBarHeight);

        // 同步滚动条范围和值
        m_scrollBar->setRange(min, max);
        m_scrollBar->setValue(verticalScrollBar()->value());
        m_scrollBar->setPageStep(verticalScrollBar()->pageStep());

        m_scrollBar->show();
        m_scrollBar->startShowAnimation();

        // 更新委托的右边距（滚动条宽度 + 间距）
        if (m_delegate) {
            int rightMargin = m_scrollBar->scrollBarWidth() + SCROLL_BAR_MARGIN;
            m_delegate->setRightMargin(rightMargin);
            viewport()->update();
        }
    }
}

void QtETListWidget::startSmoothScroll(int delta)
{
    // 计算目标滚动位置
    int currentValue = verticalScrollBar()->value();
    int targetValue = currentValue + delta;

    // 限制在有效范围内
    targetValue = qBound(verticalScrollBar()->minimum(), targetValue, verticalScrollBar()->maximum());

    // 如果目标值与当前值相同，不执行动画
    if (targetValue == currentValue) {
        return;
    }

    // 停止当前动画
    if (m_scrollAnimation->state() == QPropertyAnimation::Running) {
        m_scrollAnimation->stop();
    }

    // 设置动画参数
    m_scrollAnimation->setStartValue(currentValue);
    m_scrollAnimation->setEndValue(targetValue);
    m_scrollAnimation->start();

    // 显示滚动条
    if (m_scrollBar) {
        m_scrollBar->startShowAnimation();
    }
}
