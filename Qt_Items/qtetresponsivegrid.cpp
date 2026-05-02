#include "qtetresponsivegrid.h"

QtETResponsiveGrid::QtETResponsiveGrid(QWidget *parent)
    : QWidget(parent)
    , m_gridLayout(new QGridLayout(this))
{
    m_gridLayout->setHorizontalSpacing(DEFAULT_H_SPACING);
    m_gridLayout->setVerticalSpacing(DEFAULT_V_SPACING);
    m_gridLayout->setContentsMargins(DEFAULT_MARGIN_L, DEFAULT_MARGIN_T,
                                     DEFAULT_MARGIN_R, DEFAULT_MARGIN_B);

    setLayout(m_gridLayout);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setAutoFillBackground(false);
}

QtETResponsiveGrid::~QtETResponsiveGrid() = default;

void QtETResponsiveGrid::addItem(QWidget *widget)
{
    if (!widget) {
        return;
    }

    m_items.append(widget);

    int row = m_items.size() - 1;
    int col = row % m_currentColumns;
    row /= m_currentColumns;
    m_gridLayout->addWidget(widget, row, col);
}

void QtETResponsiveGrid::removeItem(QWidget *widget)
{
    if (!widget) {
        return;
    }

    m_gridLayout->removeWidget(widget);
    m_items.removeOne(widget);
    recalculateLayout();
}

void QtETResponsiveGrid::clearItems()
{
    for (QWidget *item : std::as_const(m_items)) {
        m_gridLayout->removeWidget(item);
    }
    m_items.clear();
}

void QtETResponsiveGrid::setMinColumns(int columns)
{
    m_minColumns = qMax(1, columns);
    if (m_maxColumns < m_minColumns) {
        m_maxColumns = m_minColumns;
    }
    recalculateLayout();
}

int QtETResponsiveGrid::minColumns() const
{
    return m_minColumns;
}

void QtETResponsiveGrid::setMaxColumns(int columns)
{
    m_maxColumns = qMax(m_minColumns, columns);
    recalculateLayout();
}

int QtETResponsiveGrid::maxColumns() const
{
    return m_maxColumns;
}

int QtETResponsiveGrid::currentColumns() const
{
    return m_currentColumns;
}

int QtETResponsiveGrid::itemCount() const
{
    return static_cast<int>(m_items.size());
}

void QtETResponsiveGrid::setGridSpacing(int horizontal, int vertical)
{
    m_gridLayout->setHorizontalSpacing(horizontal);
    m_gridLayout->setVerticalSpacing(vertical);
}

void QtETResponsiveGrid::setGridMargins(int left, int top, int right, int bottom)
{
    m_gridLayout->setContentsMargins(left, top, right, bottom);
}

void QtETResponsiveGrid::setSafetyRatio(qreal ratio)
{
    m_safetyRatio = qMax(0.8, ratio);
    recalculateLayout();
}

QSize QtETResponsiveGrid::sizeHint() const
{
    return m_gridLayout->sizeHint();
}

QSize QtETResponsiveGrid::minimumSizeHint() const
{
    return QSize(requiredWidthForColumns(m_minColumns), m_gridLayout->minimumSize().height());
}

void QtETResponsiveGrid::resizeEvent(QResizeEvent *event)
{
    recalculateLayout();
    QWidget::resizeEvent(event);
}

void QtETResponsiveGrid::recalculateLayout()
{
    if (m_items.isEmpty()) {
        return;
    }

    const int marginsH = m_gridLayout->contentsMargins().left()
                         + m_gridLayout->contentsMargins().right();
    const int availableWidth = width() - marginsH;

    if (availableWidth <= 0) {
        return;
    }

    int optimalColumns = m_minColumns;

    for (int cols = m_maxColumns; cols >= m_minColumns; --cols) {
        const int required = requiredWidthForColumns(cols);
        if (static_cast<qreal>(availableWidth) >= static_cast<qreal>(required) * m_safetyRatio) {
            optimalColumns = cols;
            break;
        }
    }

    if (optimalColumns == m_currentColumns) {
        return;
    }

    m_currentColumns = optimalColumns;

    for (QWidget *item : std::as_const(m_items)) {
        m_gridLayout->removeWidget(item);
    }

    int row = 0;
    int col = 0;
    for (QWidget *item : std::as_const(m_items)) {
        m_gridLayout->addWidget(item, row, col);
        ++col;
        if (col >= m_currentColumns) {
            col = 0;
            ++row;
        }
    }
}

int QtETResponsiveGrid::requiredWidthForColumns(int columns) const
{
    if (columns <= 0 || m_items.isEmpty()) {
        return 0;
    }

    const int numItems = static_cast<int>(m_items.size());
    const int hSpacing = m_gridLayout->horizontalSpacing();

    int totalWidth = 0;
    for (int c = 0; c < columns; ++c) {
        int maxItemWidth = 0;
        int row = 0;
        int idx = c;
        while (idx < numItems) {
            maxItemWidth = qMax(maxItemWidth, m_items[idx]->sizeHint().width());
            ++row;
            idx = row * columns + c;
        }
        totalWidth += maxItemWidth;
    }

    totalWidth += (columns - 1) * hSpacing;

    return totalWidth;
}
