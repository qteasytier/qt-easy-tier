#include "qtetlabellist.h"
#include "qtettheme.h"
#include "qtetdrawutils.h"

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleHints>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFontMetrics>
#include <QKeyEvent>

QtETLabelListItem::QtETLabelListItem()
{
}

QtETLabelListItem::QtETLabelListItem(const QString &text)
    : m_text(text)
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

QtETLabelList::QtETLabelList(QWidget *parent)
    : QWidget(parent)
    , m_highlightColor(QtETTheme::AccentColor)
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
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    m_hoverFillColor = QtETTheme::instance()->hoverFillColor(m_highlightColor);

    m_hoverAnimation = new QPropertyAnimation(this, "hoverOpacity", this);
    m_hoverAnimation->setDuration(ANIMATION_DURATION);
    m_hoverAnimation->setEasingCurve(QEasingCurve::OutCubic);

    m_selectionAnimation = new QPropertyAnimation(this, "selectionOpacity", this);
    m_selectionAnimation->setDuration(ANIMATION_DURATION);
    m_selectionAnimation->setEasingCurve(QEasingCurve::OutCubic);

    updateColorScheme();

    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        updateColorScheme();
        update();
    });
}

void QtETLabelList::updateColorScheme()
{
    auto *theme = QtETTheme::instance();
    m_textColor = theme->textColor();
    m_selectedTextColor = theme->selectedTextColor();
    m_bgColor = Qt::transparent;
    m_hoverFillColor = theme->hoverFillColor(m_highlightColor);
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

qreal QtETLabelList::hoverOpacity() const { return m_hoverOpacity; }
void QtETLabelList::setHoverOpacity(qreal opacity)
{
    if (!qFuzzyCompare(m_hoverOpacity, opacity)) {
        m_hoverOpacity = qBound(0.0, opacity, 1.0);
        update();
    }
}

qreal QtETLabelList::selectionOpacity() const { return m_selectionOpacity; }
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
    m_hoverFillColor = QtETTheme::instance()->hoverFillColor(color);
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
    setMinimumHeight(totalHeight);
}

void QtETLabelList::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), m_bgColor);

    int firstVisibleRow = m_scrollOffset / ITEM_HEIGHT;
    int lastVisibleRow = (m_scrollOffset + height() + ITEM_HEIGHT - 1) / ITEM_HEIGHT;
    lastVisibleRow = qMin(lastVisibleRow, static_cast<int>(m_items.size()) - 1);

    for (int row = firstVisibleRow; row <= lastVisibleRow; ++row) {
        QRect itemRect = calculateItemRect(row);
        QtETLabelListItem *itemData = m_items[row];

        if (itemRect.bottom() < 0 || itemRect.top() > height()) {
            continue;
        }

        painter.save();
        painter.setClipRect(itemRect);

        QRect drawRect = itemRect.adjusted(ITEM_MARGIN, ITEM_MARGIN / 2,
                                            -ITEM_MARGIN, -ITEM_MARGIN / 2);

        bool isSelected = (row == m_selectedRow);
        bool isHovered = (row == m_hoveredRow);

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

        QIcon icon = itemData->icon();
        if (!icon.isNull()) {
            int iconY = drawRect.top() + (drawRect.height() - ICON_SIZE) / 2;
            QRect iconRect(drawRect.left() + 8, iconY, ICON_SIZE, ICON_SIZE);
            icon.paint(&painter, iconRect);
        }

        QString text = itemData->text();
        if (!text.isEmpty()) {
            QFont font = painter.font();
            font.setPointSize(TEXT_SIZE);
            painter.setFont(font);

            QColor textColor = isSelected ? m_selectedTextColor : m_textColor;
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

void QtETLabelList::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        int clickedRow = getRowAtPosition(event->pos());
        if (clickedRow >= 0 && clickedRow < static_cast<int>(m_items.size())) {
            emit itemDoubleClicked(m_items[clickedRow]);
        }
    }
    QWidget::mouseDoubleClickEvent(event);
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

    int delta = event->angleDelta().y();
    int newOffset = m_scrollOffset - delta / 8;

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

void QtETLabelList::keyPressEvent(QKeyEvent *event)
{
    int current = m_selectedRow;
    int newIndex = current;

    switch (event->key()) {
    case Qt::Key_Up:
        newIndex = qMax(0, current - 1);
        break;
    case Qt::Key_Down:
        newIndex = qMin(count() - 1, current + 1);
        break;
    case Qt::Key_Home:
        newIndex = 0;
        break;
    case Qt::Key_End:
        newIndex = count() - 1;
        break;
    case Qt::Key_PageUp:
        newIndex = qMax(0, current - qMax(1, height() / ITEM_HEIGHT));
        break;
    case Qt::Key_PageDown:
        newIndex = qMin(count() - 1, current + qMax(1, height() / ITEM_HEIGHT));
        break;
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (current >= 0 && current < count()) {
            emit itemClicked(m_items[current]);
        }
        event->accept();
        return;
    default:
        QWidget::keyPressEvent(event);
        return;
    }

    if (newIndex != current) {
        setCurrentRow(newIndex);
        // 自动滚动到可视区域
        int itemTop = newIndex * ITEM_HEIGHT;
        int itemBottom = itemTop + ITEM_HEIGHT;
        if (itemTop < m_scrollOffset) {
            m_scrollOffset = itemTop;
        } else if (itemBottom > m_scrollOffset + height()) {
            m_scrollOffset = itemBottom - height();
        }
        update();
    }
    event->accept();
}
