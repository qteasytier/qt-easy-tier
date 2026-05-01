#include "qtetcombobox.h"
#include "qtettheme.h"
#include "qtetdrawutils.h"

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleHints>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QFontMetrics>
#include <QListView>
#include <QStyledItemDelegate>

// ==================== 下拉列表委托 ====================

class QtETComboDelegate : public QStyledItemDelegate
{
public:
    explicit QtETComboDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        auto *theme = QtETTheme::instance();
        QRect itemRect = option.rect;

        if (option.state & QStyle::State_Selected) {
            QColor selBg = theme->selectedBackgroundColor();
            QPainterPath path;
            path.addRoundedRect(itemRect.adjusted(1, 1, -1, -1), 4, 4);
            painter->fillPath(path, selBg);
        } else if (option.state & QStyle::State_MouseOver) {
            QColor hoverBg = theme->hoverBackgroundColor();
            QPainterPath path;
            path.addRoundedRect(itemRect.adjusted(1, 1, -1, -1), 4, 4);
            painter->fillPath(path, hoverBg);
        }

        painter->setPen(theme->textColor());
        QFontMetrics fm(option.font);
        QString text = index.data(Qt::DisplayRole).toString();
        QString elidedText = fm.elidedText(text, Qt::ElideRight, itemRect.width() - 16);
        QRect textRect = itemRect.adjusted(8, 0, -8, 0);
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedText);

        painter->restore();
    }

    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(qMax(size.height(), 28));
        return size;
    }
};

// ==================== QtETComboBox 实现 ====================

QtETComboBox::QtETComboBox(QWidget *parent)
    : QComboBox(parent)
    , m_isPressed(false)
{
    init();
}

QtETComboBox::~QtETComboBox()
{
    if (m_borderAnimation) {
        m_borderAnimation->stop();
    }
}

void QtETComboBox::init()
{
    m_borderAnimation = new QPropertyAnimation(this, "borderOpacity", this);
    m_borderAnimation->setDuration(BORDER_ANIMATION_DURATION);
    m_borderAnimation->setEasingCurve(QEasingCurve::OutCubic);

    m_popupView = new QListView(this);
    m_popupView->setItemDelegate(new QtETComboDelegate(m_popupView));
    m_popupView->setFrameShape(QFrame::NoFrame);
    m_popupView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_popupView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setView(m_popupView);

    updateColorScheme();
    updatePopupTheme();

    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        updateColorScheme();
        updatePopupTheme();
        update();
    });
}

void QtETComboBox::updateColorScheme()
{
    auto *theme = QtETTheme::instance();
    m_backgroundColor = theme->widgetBackgroundColor();
    m_normalBorderColor = theme->normalBorderColor();
    m_highlightBorderColor = theme->highlightBorderColor();
    m_pressedBorderColor = theme->pressedBorderColor();
    m_disabledTextColor = theme->textInactiveColor();
    update();
}

void QtETComboBox::updatePopupTheme()
{
    if (!m_popupView) {
        return;
    }
    auto *theme = QtETTheme::instance();
    QPalette pal = m_popupView->palette();
    pal.setColor(QPalette::Base, theme->widgetBackgroundColor());
    pal.setColor(QPalette::Text, theme->textColor());
    pal.setColor(QPalette::Highlight, Qt::transparent);
    pal.setColor(QPalette::HighlightedText, theme->textColor());
    m_popupView->setPalette(pal);
}

qreal QtETComboBox::borderOpacity() const
{
    return m_borderOpacity;
}

void QtETComboBox::setBorderOpacity(qreal opacity)
{
    if (!qFuzzyCompare(m_borderOpacity, opacity)) {
        m_borderOpacity = qBound(0.0, opacity, 1.0);
        update();
    }
}

void QtETComboBox::startBorderAnimation(qreal targetOpacity)
{
    if (m_borderAnimation->state() == QAbstractAnimation::Running) {
        m_borderAnimation->stop();
    }
    m_borderAnimation->setStartValue(m_borderOpacity);
    m_borderAnimation->setEndValue(targetOpacity);
    m_borderAnimation->start();
}

QSize QtETComboBox::sizeHint() const
{
    QFontMetrics fm(font());
    int textWidth = fm.horizontalAdvance(currentText());
    int textHeight = fm.height();

    int width = textWidth + ARROW_SIZE + CONTENT_MARGIN * 2 + 12;
    int height = textHeight + CONTENT_MARGIN * 2;

    return QSize(width, height);
}

QSize QtETComboBox::minimumSizeHint() const
{
    return QSize(80, sizeHint().height());
}

void QtETComboBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect borderRect = rect().adjusted(1, 1, -1, -1);

    QColor borderColor;
    QColor bgColor = m_backgroundColor;

    if (!isEnabled()) {
        borderColor = m_normalBorderColor;
        bgColor = QtETDrawUtils::blendColors(bgColor, QColor(128, 128, 128), 0.1);
    } else if (m_isPressed) {
        borderColor = m_pressedBorderColor;
    } else {
        borderColor = QtETDrawUtils::blendColors(m_normalBorderColor, m_highlightBorderColor, m_borderOpacity);
    }

    QtETDrawUtils::drawRoundedRect(&painter, borderRect, BORDER_RADIUS, bgColor, borderColor, BORDER_WIDTH);

    if (m_isPressed && isEnabled()) {
        QColor pressedOverlay = QtETTheme::AccentColor;
        pressedOverlay.setAlphaF(0.3);
        QPainterPath overlayPath;
        overlayPath.addRoundedRect(borderRect, BORDER_RADIUS, BORDER_RADIUS);
        painter.fillPath(overlayPath, pressedOverlay);
    }

    // 绘制选中文字
    if (!currentText().isEmpty()) {
        QFontMetrics fm(font());
        QString elidedText = fm.elidedText(currentText(), Qt::ElideRight,
                                           width() - ARROW_SIZE - CONTENT_MARGIN * 2 - 12);
        QRect textRect = rect().adjusted(CONTENT_MARGIN + 2, 0,
                                         -(ARROW_SIZE + CONTENT_MARGIN + 6), 0);
        painter.setFont(font());
        painter.setPen(isEnabled() ? palette().color(QPalette::Text) : m_disabledTextColor);
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedText);
    }

    // 绘制下拉箭头三角形
    int arrowX = width() - ARROW_SIZE - CONTENT_MARGIN - 4;
    int arrowY = (height() - ARROW_HEIGHT) / 2;
    QPainterPath arrowPath;
    arrowPath.moveTo(arrowX, arrowY);
    arrowPath.lineTo(arrowX + ARROW_SIZE, arrowY);
    arrowPath.lineTo(arrowX + ARROW_SIZE / 2, arrowY + ARROW_HEIGHT);
    arrowPath.closeSubpath();
    painter.fillPath(arrowPath, isEnabled() ? palette().color(QPalette::Text) : m_disabledTextColor);
}

void QtETComboBox::enterEvent(QEnterEvent *event)
{
    QComboBox::enterEvent(event);
    if (isEnabled()) {
        startBorderAnimation(1.0);
    }
}

void QtETComboBox::leaveEvent(QEvent *event)
{
    QComboBox::leaveEvent(event);
    if (isEnabled()) {
        startBorderAnimation(0.0);
    }
}

void QtETComboBox::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isEnabled()) {
        m_isPressed = true;
        update();
    }
    QComboBox::mousePressEvent(event);
}

void QtETComboBox::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isPressed = false;
        update();
    }
    QComboBox::mouseReleaseEvent(event);
}

void QtETComboBox::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::EnabledChange) {
        update();
    }
    QComboBox::changeEvent(event);
}

void QtETComboBox::showPopup()
{
    updatePopupTheme();
    // 设置下拉列表的最小宽度为控件宽度
    m_popupView->setMinimumWidth(width());
    m_popupView->setMaximumHeight(m_popupView->sizeHintForRow(0) * 7);
    QComboBox::showPopup();
}
