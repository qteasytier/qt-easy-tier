#include "qtetlabellist.h"

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleHints>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFontMetrics>

// ============================================================================
// QtETLabelListItem 实现
// ============================================================================

QtETLabelListItem::QtETLabelListItem()
    : m_text()
    , m_icon()
{
}

QtETLabelListItem::QtETLabelListItem(const QString &text)
    : m_text(text)
    , m_icon()
{
}

QtETLabelListItem::QtETLabelListItem(const QIcon &icon, const QString &text)
    : m_text(text)
    , m_icon(icon)
{
}

QVariant QtETLabelListItem::data(int role) const
{
    return m_data.value(role);
}

void QtETLabelListItem::setData(int role, const QVariant &value)
{
    m_data[role] = value;
}

// ============================================================================
// QtETLabelList 实现
// ============================================================================

QtETLabelList::QtETLabelList(QWidget *parent)
    : QWidget(parent)
    , m_hoverAnimation(nullptr)
    , m_selectionAnimation(nullptr)
    , m_hoverOpacity(0.0)
    , m_selectionOpacity(0.0)
    , m_hoveredRow(-1)
    , m_selectedRow(-1)
    , m_scrollOffset(0)
    , m_highlightColor("#66ccff")
    , m_textColor(Qt::black)
    , m_bgColor(Qt::transparent)
{
    init();
}

QtETLabelList::~QtETLabelList()
{
    if (m_hoverAnimation) {
        m_hoverAnimation->stop();
    }
    if (m_selectionAnimation) {
        m_selectionAnimation->stop();
    }

    qDeleteAll(m_items);
    m_items.clear();
}

void QtETLabelList::init()
{
    // 设置背景透明
    setAttribute(Qt::WA_TranslucentBackground, true);
    
    // 设置鼠标追踪
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    // 默认垂直伸展
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    // 计算悬停填充颜色
    m_hoverFillColor = QColor::fromRgbF(
        m_highlightColor.redF(),
        m_highlightColor.greenF(),
        m_highlightColor.blueF(),
        0.15  // 15% 不透明度
    );

    // 初始化悬停动画
    m_hoverAnimation = new QPropertyAnimation(this, "hoverOpacity", this);
    m_hoverAnimation->setDuration(ANIMATION_DURATION);
    m_hoverAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // 初始化选中动画
    m_selectionAnimation = new QPropertyAnimation(this, "selectionOpacity", this);
    m_selectionAnimation->setDuration(ANIMATION_DURATION);
    m_selectionAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // 更新颜色方案
    updateColorScheme();

    // 监听系统主题变化
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        updateColorScheme();
        update();
    });
}

void QtETLabelList::updateColorScheme()
{
    const bool isDark = (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark);

    if (isDark) {
        m_textColor = QColor(220, 220, 220);
    } else {
        m_textColor = QColor(50, 50, 50);
    }

    // 背景始终透明
    m_bgColor = Qt::transparent;

    update();
}

void QtETLabelList::addItem(QtETLabelListItem *item)
{
    m_items.append(item);
    updateContentHeight();
    update();
}

void QtETLabelList::addItem(const QString &text)
{
    addItem(new QtETLabelListItem(text));
}

int QtETLabelList::count() const
{
    return static_cast<int>(m_items.size());
}

QtETLabelListItem* QtETLabelList::item(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_items.size())) {
        return m_items[index];
    }
    return nullptr;
}

QtETLabelListItem* QtETLabelList::currentItem() const
{
    return item(m_selectedRow);
}

int QtETLabelList::currentRow() const
{
    return m_selectedRow;
}

int QtETLabelList::row(QtETLabelListItem *item) const
{
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if (m_items[i] == item) {
            return i;
        }
    }
    return -1;
}

void QtETLabelList::setCurrentRow(int row)
{
    if (row >= -1 && row < static_cast<int>(m_items.size()) && row != m_selectedRow) {
        int oldRow = m_selectedRow;
        m_selectedRow = row;

        // 启动选中动画
        m_selectionAnimation->stop();
        m_selectionAnimation->setStartValue(m_selectionOpacity);
        m_selectionAnimation->setEndValue(row >= 0 ? 1.0 : 0.0);
        m_selectionAnimation->start();

        update();

        if (oldRow != row) {
            emit currentRowChanged(row);
            emit itemSelectionChanged();
        }
    }
}

void QtETLabelList::clear()
{
    qDeleteAll(m_items);
    m_items.clear();
    m_selectedRow = -1;
    m_hoveredRow = -1;
    m_scrollOffset = 0;
    updateContentHeight();
    update();
}

QtETLabelListItem* QtETLabelList::takeItem(int row)
{
    if (row >= 0 && row < static_cast<int>(m_items.size())) {
        QtETLabelListItem *item = m_items.takeAt(row);
        if (m_selectedRow == row) {
            m_selectedRow = -1;
        } else if (m_selectedRow > row) {
            m_selectedRow--;
        }
        if (m_hoveredRow == row) {
            m_hoveredRow = -1;
        } else if (m_hoveredRow > row) {
            m_hoveredRow--;
        }
        updateContentHeight();
        update();
        return item;
    }
    return nullptr;
}

QRect QtETLabelList::visualItemRect(QtETLabelListItem *item) const
{
    if (!item) {
        return QRect();
    }

    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if (m_items[i] == item) {
            return calculateItemRect(i);
        }
    }
    return QRect();
}

QtETLabelListItem* QtETLabelList::itemAt(const QPoint &pos) const
{
    int row = getRowAtPosition(pos);
    if (row >= 0 && row < static_cast<int>(m_items.size())) {
        return m_items[row];
    }
    return nullptr;
}

void QtETLabelList::setItemIcon(int index, const QIcon &icon)
{
    if (index >= 0 && index < static_cast<int>(m_items.size())) {
        m_items[index]->setIcon(icon);
        update();
    }
}

qreal QtETLabelList::hoverOpacity() const
{
    return m_hoverOpacity;
}

void QtETLabelList::setHoverOpacity(qreal opacity)
{
    if (!qFuzzyCompare(m_hoverOpacity, opacity)) {
        m_hoverOpacity = qBound(0.0, opacity, 1.0);
        update();
    }
}

qreal QtETLabelList::selectionOpacity() const
{
    return m_selectionOpacity;
}

void QtETLabelList::setSelectionOpacity(qreal opacity)
{
    if (!qFuzzyCompare(m_selectionOpacity, opacity)) {
        m_selectionOpacity = qBound(0.0, opacity, 1.0);
        update();
    }
}

void QtETLabelList::setHighlightColor(const QColor &color)
{
    m_highlightColor = color;
    m_hoverFillColor = QColor::fromRgbF(
        color.redF(),
        color.greenF(),
        color.blueF(),
        0.15
    );
    update();
}

QColor QtETLabelList::highlightColor() const
{
    return m_highlightColor;
}

QRect QtETLabelList::calculateItemRect(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_items.size())) {
        return QRect();
    }

    int y = row * ITEM_HEIGHT - m_scrollOffset;
    return QRect(0, y, width(), ITEM_HEIGHT);
}

int QtETLabelList::getRowAtPosition(const QPoint &pos) const
{
    int y = pos.y() + m_scrollOffset;
    int row = y / ITEM_HEIGHT;

    if (row >= 0 && row < static_cast<int>(m_items.size())) {
        return row;
    }
    return -1;
}

void QtETLabelList::updateContentHeight()
{
    int totalHeight = static_cast<int>(m_items.size()) * ITEM_HEIGHT;
    // 设置最小高度为内容高度，允许自动伸展
    setMinimumHeight(totalHeight);
}

void QtETLabelList::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制背景
    painter.fillRect(rect(), m_bgColor);

    // 计算可见范围
    int firstVisibleRow = m_scrollOffset / ITEM_HEIGHT;
    int lastVisibleRow = (m_scrollOffset + height() + ITEM_HEIGHT - 1) / ITEM_HEIGHT;
    lastVisibleRow = qMin(lastVisibleRow, static_cast<int>(m_items.size()) - 1);

    // 绘制每个可见项目
    for (int row = firstVisibleRow; row <= lastVisibleRow; ++row) {
        QRect itemRect = calculateItemRect(row);
        QtETLabelListItem *itemData = m_items[row];

        // 完全在可视区域外的项目不绘制
        if (itemRect.bottom() < 0 || itemRect.top() > height()) {
            continue;
        }

        // 设置裁剪区域
        painter.save();
        painter.setClipRect(itemRect);

        // 计算绘制区域（减去边距）
        QRect drawRect = itemRect.adjusted(ITEM_MARGIN, ITEM_MARGIN / 2,
                                            -ITEM_MARGIN, -ITEM_MARGIN / 2);

        bool isSelected = (row == m_selectedRow);
        bool isHovered = (row == m_hoveredRow);

        // 绘制选中或悬停背景
        if (isSelected) {
            QPainterPath path;
            path.addRoundedRect(drawRect, BORDER_RADIUS, BORDER_RADIUS);
            painter.fillPath(path, m_highlightColor);
            painter.setPen(QPen(m_highlightColor, 1.5));
            painter.drawPath(path);
        } else if (isHovered) {
            QPainterPath path;
            path.addRoundedRect(drawRect, BORDER_RADIUS, BORDER_RADIUS);
            painter.fillPath(path, m_hoverFillColor);
            painter.setPen(QPen(m_highlightColor, 1.0));
            painter.drawPath(path);
        }

        // 绘制图标
        QIcon icon = itemData->icon();
        if (!icon.isNull()) {
            int iconY = drawRect.top() + (drawRect.height() - ICON_SIZE) / 2;
            QRect iconRect(drawRect.left() + 8, iconY, ICON_SIZE, ICON_SIZE);
            icon.paint(&painter, iconRect);
        }

        // 绘制文字
        QString text = itemData->text();
        if (!text.isEmpty()) {
            QFont font = painter.font();
            font.setPointSize(TEXT_SIZE);
            painter.setFont(font);

            QColor textColor = isSelected ? QColor(0, 0, 0) : m_textColor;
            painter.setPen(textColor);

            int textLeft = drawRect.left() + 8;
            if (!icon.isNull()) {
                textLeft += ICON_SIZE + ICON_TEXT_SPACING;
            }

            QRect textRect(textLeft, drawRect.top(),
                           drawRect.right() - textLeft - 8, drawRect.height());
            painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
        }

        painter.restore();
    }

    // 当没有任何选项时，居中显示提示文字
    if (m_items.isEmpty()) {
        QFont font = painter.font();
        font.setPointSize(14);
        painter.setFont(font);

        QColor textColor = m_textColor;
        textColor.setAlphaF(0.4);
        painter.setPen(textColor);

        painter.drawText(rect(), Qt::AlignCenter, tr("空空如也"));
    }
}

void QtETLabelList::mouseMoveEvent(QMouseEvent *event)
{
    int newHoveredRow = getRowAtPosition(event->pos());

    if (newHoveredRow != m_hoveredRow) {
        m_hoveredRow = newHoveredRow;
        update();
    }

    QWidget::mouseMoveEvent(event);
}

void QtETLabelList::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        int clickedRow = getRowAtPosition(event->pos());

        if (clickedRow >= 0 && clickedRow < static_cast<int>(m_items.size())) {
            setCurrentRow(clickedRow);
            emit itemClicked(m_items[clickedRow]);
        }
    }

    QWidget::mousePressEvent(event);
}

void QtETLabelList::leaveEvent(QEvent *event)
{
    m_hoveredRow = -1;
    update();
    QWidget::leaveEvent(event);
}

void QtETLabelList::wheelEvent(QWheelEvent *event)
{
    int totalHeight = static_cast<int>(m_items.size()) * ITEM_HEIGHT;
    int visibleHeight = height();

    if (totalHeight <= visibleHeight) {
        event->accept();
        return;
    }

    // 滚动
    int delta = event->angleDelta().y();
    int newOffset = m_scrollOffset - delta / 8;  // 标准滚动速度

    // 限制范围
    int maxOffset = totalHeight - visibleHeight;
    newOffset = qBound(0, newOffset, maxOffset);

    if (newOffset != m_scrollOffset) {
        m_scrollOffset = newOffset;
        update();
    }

    event->accept();
}

void QtETLabelList::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}