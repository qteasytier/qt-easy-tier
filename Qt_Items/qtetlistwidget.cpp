#include "qtetlistwidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleHints>
#include <QMouseEvent>
#include <QAbstractItemView>

// ============================================================================
// QtETListWidgetItem 实现
// ============================================================================

QtETListWidgetItem::QtETListWidgetItem(QListWidget *parent, int type)
    : QListWidgetItem(parent, type)
{
}

QtETListWidgetItem::QtETListWidgetItem(const QString &text, QListWidget *parent, int type)
    : QListWidgetItem(text, parent, type)
{
}

QtETListWidgetItem::QtETListWidgetItem(const QIcon &icon, const QString &text,
                                       QListWidget *parent, int type)
    : QListWidgetItem(icon, text, parent, type)
{
}

QtETListWidgetItem::QtETListWidgetItem(const QtETListWidgetItem &other)
    : QListWidgetItem(other)
{
}

QtETListWidgetItem &QtETListWidgetItem::operator=(const QtETListWidgetItem &other)
{
    QListWidgetItem::operator=(other);
    return *this;
}

// ============================================================================
// QtETListWidgetDelegate 实现
// ============================================================================

QtETListWidgetDelegate::QtETListWidgetDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
    , m_highlightColor("#66ccff")
{
    // 悬停填充颜色：高亮色的淡化版本
    m_hoverFillColor = QColor::fromRgbF(
        m_highlightColor.redF(),
        m_highlightColor.greenF(),
        m_highlightColor.blueF(),
        0.15  // 15% 不透明度
    );
}

void QtETListWidgetDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // 绘制背景
    QRect rect = option.rect.adjusted(ITEM_MARGIN, ITEM_MARGIN / 2,
                                      -ITEM_MARGIN, -ITEM_MARGIN / 2);

    // 判断状态
    const bool isSelected = option.state & QStyle::State_Selected;
    const bool isHovered = option.state & QStyle::State_MouseOver;

    if (isSelected) {
        // 选中状态：实心圆角矩形高亮
        QPainterPath path;
        path.addRoundedRect(rect, BORDER_RADIUS, BORDER_RADIUS);

        // 使用不透明的高亮色填充
        painter->fillPath(path, m_highlightColor);

        // 绘制不透明边框
        painter->setPen(QPen(m_highlightColor, 1.5));
        painter->drawPath(path);
    } else if (isHovered) {
        // 悬停状态：空心圆角矩形 + 淡色填充
        QPainterPath path;
        path.addRoundedRect(rect, BORDER_RADIUS, BORDER_RADIUS);

        // 填充淡色
        painter->fillPath(path, m_hoverFillColor);

        // 绘制边框
        painter->setPen(QPen(m_highlightColor, 1.0));
        painter->drawPath(path);
    }

    // 绘制图标
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    if (!icon.isNull()) {
        int iconY = rect.top() + (rect.height() - ICON_SIZE) / 2;
        QRect iconRect(rect.left() + 8, iconY, ICON_SIZE, ICON_SIZE);
        icon.paint(painter, iconRect);
    }

    // 绘制文字
    QString text = index.data(Qt::DisplayRole).toString();
    if (!text.isEmpty()) {
        QFont font = painter->font();
        font.setPointSize(TEXT_SIZE);
        painter->setFont(font);

        // 设置文字颜色
        QColor textColor;
        if (isSelected) {
            // 选中状态：文字设为黑色
            textColor = QColor(0, 0, 0);
        } else {
            // 未选中状态：使用默认文字颜色
            textColor = option.palette.color(QPalette::Text);
        }
        painter->setPen(textColor);

        // 计算文字位置
        int textLeft = rect.left() + 8;
        if (!icon.isNull()) {
            textLeft += ICON_SIZE + ICON_TEXT_SPACING;
        }

        QRect textRect(textLeft, rect.top(),
                       rect.right() - textLeft - 8, rect.height());
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
    }

    painter->restore();
}

QSize QtETListWidgetDelegate::sizeHint(const QStyleOptionViewItem &option,
                                       const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    // 增大项高度 (原来: ICON_SIZE + ITEM_MARGIN * 2 + 4 = 18 + 8 + 4 = 30)
    constexpr int ITEM_HEIGHT = 36;

    // 基础宽度（至少能容纳图标和少量文字）
    int width = ICON_SIZE + ICON_TEXT_SPACING + 100 + ITEM_MARGIN * 2 + 16;

    return QSize(width, ITEM_HEIGHT);
}

void QtETListWidgetDelegate::setHighlightColor(const QColor &color)
{
    m_highlightColor = color;
    m_hoverFillColor = QColor::fromRgbF(
        color.redF(),
        color.greenF(),
        color.blueF(),
        0.15
    );
}

QColor QtETListWidgetDelegate::highlightColor() const
{
    return m_highlightColor;
}

// ============================================================================
// QtETListWidget 实现
// ============================================================================

QtETListWidget::QtETListWidget(QWidget *parent)
    : QListWidget(parent)
    , m_delegate(nullptr)
    , m_hoverAnimation(nullptr)
    , m_selectionAnimation(nullptr)
    , m_hoverOpacity(0.0)
    , m_selectionOpacity(0.0)
    , m_hoverRow(-1)
    , m_selectedRow(-1)
    , m_highlightColor("#66ccff")
{
    init();
}

QtETListWidget::~QtETListWidget()
{
    if (m_hoverAnimation) {
        m_hoverAnimation->stop();
    }
    if (m_selectionAnimation) {
        m_selectionAnimation->stop();
    }
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

    // 去除默认的选中框
    setStyleSheet(QStringLiteral(R"(
        QListWidget {
            outline: none;
            border: none;
            background: transparent;
        }
        QListWidget::item {
            background: transparent;
            border: none;
            padding: 2px 4px;
        }
    )"));

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
        viewport()->update();
    });
}

void QtETListWidget::updateColorScheme()
{
    // 高亮颜色保持不变，但可以根据需要调整
    m_delegate->setHighlightColor(m_highlightColor);
}

qreal QtETListWidget::hoverOpacity() const
{
    return m_hoverOpacity;
}

void QtETListWidget::setHoverOpacity(qreal opacity)
{
    if (!qFuzzyCompare(m_hoverOpacity, opacity)) {
        m_hoverOpacity = qBound(0.0, opacity, 1.0);
        viewport()->update();
    }
}

qreal QtETListWidget::selectionOpacity() const
{
    return m_selectionOpacity;
}

void QtETListWidget::setSelectionOpacity(qreal opacity)
{
    if (!qFuzzyCompare(m_selectionOpacity, opacity)) {
        m_selectionOpacity = qBound(0.0, opacity, 1.0);
        viewport()->update();
    }
}

void QtETListWidget::setHighlightColor(const QColor &color)
{
    m_highlightColor = color;
    m_delegate->setHighlightColor(color);
    viewport()->update();
}

QColor QtETListWidget::highlightColor() const
{
    return m_highlightColor;
}

void QtETListWidget::paintEvent(QPaintEvent *event)
{
    // 调用父类绘制
    QListWidget::paintEvent(event);

    // 当没有任何选项时，居中显示提示文字
    if (count() == 0) {
        QPainter painter(viewport());
        painter.setRenderHint(QPainter::Antialiasing);

        // 设置字体
        QFont font = painter.font();
        font.setPointSize(14);
        painter.setFont(font);

        // 设置淡色文字
        QColor textColor = palette().color(QPalette::Text);
        textColor.setAlphaF(0.4);  // 40% 不透明度，偏淡
        painter.setPen(textColor);

        // 居中绘制文字
        QRect rect = viewport()->rect();
        painter.drawText(rect, Qt::AlignCenter, tr("空空如也"));
    }
}

void QtETListWidget::mouseMoveEvent(QMouseEvent *event)
{
    QListWidget::mouseMoveEvent(event);
    updateHoverItem(event->pos());
}

void QtETListWidget::leaveEvent(QEvent *event)
{
    QListWidget::leaveEvent(event);

    // 重置悬停状态
    m_hoverRow = -1;
    viewport()->update();
}

void QtETListWidget::selectionChanged(const QItemSelection &selected,
                                      const QItemSelection &deselected)
{
    QListWidget::selectionChanged(selected, deselected);

    // 更新选中行
    if (!selected.isEmpty()) {
        m_selectedRow = selected.indexes().first().row();
    } else {
        m_selectedRow = -1;
    }
}

void QtETListWidget::startHoverAnimation(bool visible)
{
    m_hoverAnimation->stop();
    m_hoverAnimation->setStartValue(m_hoverOpacity);
    m_hoverAnimation->setEndValue(visible ? 1.0 : 0.0);
    m_hoverAnimation->start();
}

void QtETListWidget::startSelectionAnimation(bool visible)
{
    m_selectionAnimation->stop();
    m_selectionAnimation->setStartValue(m_selectionOpacity);
    m_selectionAnimation->setEndValue(visible ? 1.0 : 0.0);
    m_selectionAnimation->start();
}

void QtETListWidget::updateHoverItem(const QPoint &pos)
{
    Q_UNUSED(pos)
    // 委托已经处理了悬停状态的绘制
    viewport()->update();
}
