#include "qtetlineedit.h"
#include "qtettheme.h"
#include "qtetdrawutils.h"

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleHints>
#include <QFocusEvent>
#include <QResizeEvent>
#include <QIcon>
#include <QPalette>
#include <QFontMetrics>

QtETLineEdit::QtETLineEdit(QWidget *parent)
    : QLineEdit(parent)
    , m_echoModeToggleEnabled(true)
{
    init();
}

QtETLineEdit::~QtETLineEdit()
{
    if (m_borderAnimation) {
        m_borderAnimation->stop();
    }
}

void QtETLineEdit::init()
{
    setFrame(false);
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_borderAnimation = new QPropertyAnimation(this, "borderOpacity", this);
    m_borderAnimation->setDuration(BORDER_ANIMATION_DURATION);
    m_borderAnimation->setEasingCurve(QEasingCurve::OutCubic);

    m_echoToggleAction = addAction(QIcon(":/icons/eye.svg"), QLineEdit::TrailingPosition);
    m_echoToggleAction->setVisible(false);
    m_echoToggleAction->setToolTip(tr("Toggle password visibility"));

    connect(m_echoToggleAction, &QAction::triggered, this, [this]() {
        if (echoMode() == QLineEdit::Password) {
            setEchoMode(QLineEdit::Normal);
        } else {
            setEchoMode(QLineEdit::Password);
        }
        updateButtonIcon();
    });

    updateColorScheme();

    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        updateColorScheme();
        update();
    });

    connect(this, &QLineEdit::textChanged, this, [this]() {
        updateLayout();
    });

    updateLayout();
}

void QtETLineEdit::updateColorScheme()
{
    auto *theme = QtETTheme::instance();
    m_backgroundColor = theme->widgetBackgroundColor();
    m_normalBorderColor = theme->normalBorderColor();
    m_highlightBorderColor = theme->highlightBorderColor();
    m_focusBorderColor = theme->focusBorderColor();
    m_textColor = theme->textColor();

    QPalette pal = palette();
    pal.setColor(QPalette::Text, m_textColor);
    pal.setColor(QPalette::ButtonText, m_textColor);
    pal.setColor(QPalette::Highlight, QtETTheme::AccentColor);
    pal.setColor(QPalette::HighlightedText, Qt::black);
    pal.setColor(QPalette::Base, Qt::transparent);
    pal.setColor(QPalette::Window, Qt::transparent);
    setPalette(pal);

    updateButtonIcon();
    update();
}

void QtETLineEdit::updateButtonIcon()
{
    QString iconPath = (echoMode() == QLineEdit::Password)
                           ? QStringLiteral(":/icons/eye-slash.svg")
                           : QStringLiteral(":/icons/eye.svg");
    m_echoToggleAction->setIcon(QIcon(iconPath));
}

void QtETLineEdit::updateLayout()
{
    bool shouldShowButton = m_echoModeToggleEnabled &&
                            (echoMode() == QLineEdit::Password ||
                             echoMode() == QLineEdit::PasswordEchoOnEdit);

    if (shouldShowButton) {
        m_echoToggleAction->setVisible(true);
        updateButtonIcon();
        setTextMargins(CONTENT_MARGIN_H, 0, BUTTON_SIZE + BUTTON_SPACING, 0);
    } else {
        m_echoToggleAction->setVisible(false);
        setTextMargins(CONTENT_MARGIN_H, 0, CONTENT_MARGIN_H, 0);
    }
}

qreal QtETLineEdit::borderOpacity() const { return m_borderOpacity; }

void QtETLineEdit::setBorderOpacity(qreal opacity)
{
    if (!qFuzzyCompare(m_borderOpacity, opacity)) {
        m_borderOpacity = qBound(0.0, opacity, 1.0);
        update();
    }
}

void QtETLineEdit::startBorderAnimation(qreal targetOpacity)
{
    if (m_borderAnimation->state() == QAbstractAnimation::Running) {
        m_borderAnimation->stop();
    }
    m_borderAnimation->setStartValue(m_borderOpacity);
    m_borderAnimation->setEndValue(targetOpacity);
    m_borderAnimation->start();
}

void QtETLineEdit::setEchoModeToggleEnabled(bool enabled)
{
    if (m_echoModeToggleEnabled != enabled) {
        m_echoModeToggleEnabled = enabled;
        updateLayout();
    }
}

bool QtETLineEdit::isEchoModeToggleEnabled() const { return m_echoModeToggleEnabled; }

QSize QtETLineEdit::sizeHint() const
{
    QFontMetrics fm(font());
    int textHeight = fm.height();
    int height = textHeight + CONTENT_MARGIN_V * 2;
    int width = fm.horizontalAdvance(QStringLiteral("XXXXXXXXXX")) + CONTENT_MARGIN_H * 2;
    return QSize(width, height);
}

QSize QtETLineEdit::minimumSizeHint() const
{
    QFontMetrics fm(font());
    int textHeight = fm.height();
    int height = textHeight + CONTENT_MARGIN_V * 2;
    int width = fm.horizontalAdvance(QStringLiteral("XX")) + CONTENT_MARGIN_H * 2;
    return QSize(width, height);
}

void QtETLineEdit::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect borderRect = rect().adjusted(1, 1, -1, -1);

    QColor borderColor;
    QColor bgColor = m_backgroundColor;

    if (!isEnabled()) {
        borderColor = m_normalBorderColor;
        bgColor = QtETDrawUtils::blendColors(bgColor, QColor(128, 128, 128), 0.1);
    } else if (m_hasFocus) {
        borderColor = m_focusBorderColor;
    } else {
        borderColor = QtETDrawUtils::blendColors(m_normalBorderColor, m_highlightBorderColor, m_borderOpacity);
    }

    QtETDrawUtils::drawRoundedRect(&painter, borderRect, BORDER_RADIUS, bgColor, borderColor, BORDER_WIDTH);

    QtETDrawUtils::safeEndPainter(&painter);

    QLineEdit::paintEvent(event);
}

void QtETLineEdit::enterEvent(QEnterEvent *event)
{
    QLineEdit::enterEvent(event);
    if (!isEnabled()) {
        setCursor(Qt::ForbiddenCursor);
    } else if (!m_hasFocus) {
        startBorderAnimation(1.0);
    }
}

void QtETLineEdit::leaveEvent(QEvent *event)
{
    QLineEdit::leaveEvent(event);
    if (!isEnabled()) {
        setCursor(Qt::ArrowCursor);
    } else if (!m_hasFocus) {
        startBorderAnimation(0.0);
    }
}

void QtETLineEdit::focusInEvent(QFocusEvent *event)
{
    m_hasFocus = true;
    update();
    QLineEdit::focusInEvent(event);
}

void QtETLineEdit::focusOutEvent(QFocusEvent *event)
{
    m_hasFocus = false;
    update();
    QLineEdit::focusOutEvent(event);
}

void QtETLineEdit::resizeEvent(QResizeEvent *event)
{
    QLineEdit::resizeEvent(event);
    updateLayout();
}

void QtETLineEdit::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::EnabledChange) {
        update();
    }
    QLineEdit::changeEvent(event);
}
