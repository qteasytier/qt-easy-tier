#include "qtetlineedit.h"

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleHints>
#include <QFocusEvent>
#include <QResizeEvent>
#include <QPushButton>
#include <QIcon>

QtETLineEdit::QtETLineEdit(QWidget *parent)
    : QLineEdit(parent)
    , m_borderOpacity(0.0)
    , m_hasFocus(false)
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
    // 初始化边框高亮动画
    m_borderAnimation = new QPropertyAnimation(this, "borderOpacity", this);
    m_borderAnimation->setDuration(BORDER_ANIMATION_DURATION);
    m_borderAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // 设置QLineEdit为透明背景、无边框，完全自定义绘制
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAutoFillBackground(false);
    setStyleSheet(
        "QLineEdit {"
        "  background: transparent;"
        "  border: none;"
        "  padding: 0px;"
        "  margin: 0px;"
        "}"
    );

    // 初始化密码切换按钮
    m_echoToggleButton = new QPushButton(this);
    m_echoToggleButton->setFixedSize(BUTTON_SIZE, BUTTON_SIZE);
    m_echoToggleButton->setCursor(Qt::PointingHandCursor);
    m_echoToggleButton->setFlat(true);
    m_echoToggleButton->setToolTip(tr("Toggle password visibility"));
    m_echoToggleButton->hide();

    // 连接密码切换按钮点击信号
    connect(m_echoToggleButton, &QPushButton::clicked, this, [this]() {
        if (echoMode() == QLineEdit::Password) {
            setEchoMode(QLineEdit::Normal);
        } else {
            setEchoMode(QLineEdit::Password);
        }
        updateButtonIcon();
    });

    // 初始化颜色
    updateColorScheme();

    // 监听系统主题变化
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        updateColorScheme();
        QWidget::update();
    });

    // 监听文本变化，更新按钮显示
    connect(this, &QLineEdit::textChanged, this, [this]() {
        updateLayout();
    });

    // 初始更新布局
    updateLayout();
}

void QtETLineEdit::updateColorScheme()
{
    const bool isDark = (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark);

    if (isDark) {
        // 深色模式
        m_backgroundColor = QColor(45, 45, 45);
        m_normalBorderColor = QColor(70, 70, 70);
        m_highlightBorderColor = QColor("#66ccff");
        m_focusBorderColor = QColor("#66ccff");
        m_textColor = QColor(220, 220, 220);
    } else {
        // 浅色模式
        m_backgroundColor = QColor(240, 240, 240);
        m_normalBorderColor = QColor(180, 180, 180);
        m_highlightBorderColor = QColor("#66ccff");
        m_focusBorderColor = QColor("#66ccff");
        m_textColor = QColor(30, 30, 30);
    }

    // 设置QLineEdit样式：透明背景、无边框、正确的文字颜色
    // 这确保了QLineEdit不会绘制自己的边框和背景
    const QString &lineEditStyle = QString(
        "QLineEdit {"
        "  background: transparent;"
        "  border: none;"
        "  padding: 6px;"
        "  margin: 0px;"
        "  color: %1;"
        "  selection-background-color: rgba(102, 204, 255, 0.3);"
        "  selection-color: %1;"
        "}"
    ).arg(m_textColor.name());
    setStyleSheet(lineEditStyle);

    // 更新密码切换按钮样式
    QString buttonStyle = QString(
        "QPushButton {"
        "  border: none;"
        "  background: transparent;"
        "  padding: 2px;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(102, 204, 255, 0.2);"
        "  border-radius: 3px;"
        "}"
    );
    m_echoToggleButton->setStyleSheet(buttonStyle);

    QWidget::update();
}

void QtETLineEdit::updateButtonIcon()
{
    // 根据当前echoMode设置图标
    QString iconPath;
    if (echoMode() == QLineEdit::Password) {
        // 密码模式，显示"显示密码"图标（eye）
        iconPath = QStringLiteral(":/icons/eye.svg");
    } else {
        // 正常模式，显示"隐藏密码"图标（eye-slash）
        iconPath = QStringLiteral(":/icons/eye-slash.svg");
    }

    QIcon icon(iconPath);
    m_echoToggleButton->setIcon(icon);
    m_echoToggleButton->setIconSize(QSize(16, 16));
}

void QtETLineEdit::updateLayout()
{
    // 判断是否需要显示密码切换按钮
    bool shouldShowButton = m_echoModeToggleEnabled && 
                           (echoMode() == QLineEdit::Password || 
                            echoMode() == QLineEdit::PasswordEchoOnEdit);

    if (shouldShowButton) {
        m_echoToggleButton->show();
        updateButtonIcon();

        // 设置按钮位置（右侧）
        int buttonX = width() - BUTTON_SIZE - BUTTON_SPACING - 2;
        int buttonY = (height() - BUTTON_SIZE) / 2;
        m_echoToggleButton->move(buttonX, buttonY);

        // 设置文本边距，为按钮留出空间
        setTextMargins(0, 0, BUTTON_SIZE + BUTTON_SPACING, 0);
    } else {
        m_echoToggleButton->hide();
        // 恢复默认文本边距
        setTextMargins(0, 0, 0, 0);
    }
}

qreal QtETLineEdit::borderOpacity() const
{
    return m_borderOpacity;
}

void QtETLineEdit::setBorderOpacity(qreal opacity)
{
    if (!qFuzzyCompare(m_borderOpacity, opacity)) {
        m_borderOpacity = qBound(0.0, opacity, 1.0);
        QWidget::update();
    }
}

void QtETLineEdit::setEchoModeToggleEnabled(bool enabled)
{
    if (m_echoModeToggleEnabled != enabled) {
        m_echoModeToggleEnabled = enabled;
        updateLayout();
    }
}

bool QtETLineEdit::isEchoModeToggleEnabled() const
{
    return m_echoModeToggleEnabled;
}

void QtETLineEdit::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制控件边框和背景
    QRect borderRect = rect().adjusted(1, 1, -1, -1);

    // 确定边框颜色
    QColor borderColor;
    if (m_hasFocus) {
        // 聚焦状态使用聚焦边框颜色
        borderColor = m_focusBorderColor;
    } else {
        // 非聚焦状态，混合普通和高亮颜色
        borderColor = QColor::fromRgbF(
            m_normalBorderColor.redF() * (1 - m_borderOpacity) + m_highlightBorderColor.redF() * m_borderOpacity,
            m_normalBorderColor.greenF() * (1 - m_borderOpacity) + m_highlightBorderColor.greenF() * m_borderOpacity,
            m_normalBorderColor.blueF() * (1 - m_borderOpacity) + m_highlightBorderColor.blueF() * m_borderOpacity,
            m_normalBorderColor.alphaF() * (1 - m_borderOpacity) + m_highlightBorderColor.alphaF() * m_borderOpacity
        );
    }

    // 绘制背景
    QPainterPath bgPath;
    bgPath.addRoundedRect(borderRect, BORDER_RADIUS, BORDER_RADIUS);
    painter.fillPath(bgPath, m_backgroundColor);

    // 绘制边框
    painter.setPen(QPen(borderColor, BORDER_WIDTH));
    painter.drawPath(bgPath);

    // 调用父类paintEvent绘制文本内容
    // 注意：需要先结束painter，因为父类paintEvent也会使用QPainter
    painter.end();

    QLineEdit::paintEvent(event);
}

void QtETLineEdit::enterEvent(QEnterEvent *event)
{
    QLineEdit::enterEvent(event);

    // 启动边框高亮动画（渐显）
    m_borderAnimation->stop();
    m_borderAnimation->setStartValue(m_borderOpacity);
    m_borderAnimation->setEndValue(1.0);
    m_borderAnimation->start();
}

void QtETLineEdit::leaveEvent(QEvent *event)
{
    QLineEdit::leaveEvent(event);

    // 启动边框淡出动画（渐隐）
    m_borderAnimation->stop();
    m_borderAnimation->setStartValue(m_borderOpacity);
    m_borderAnimation->setEndValue(0.0);
    m_borderAnimation->start();
}

void QtETLineEdit::focusInEvent(QFocusEvent *event)
{
    m_hasFocus = true;
    QWidget::update();  // 立即更新边框为聚焦色

    QLineEdit::focusInEvent(event);
}

void QtETLineEdit::focusOutEvent(QFocusEvent *event)
{
    m_hasFocus = false;
    QWidget::update();  // 恢复普通/悬停状态

    QLineEdit::focusOutEvent(event);
}

void QtETLineEdit::resizeEvent(QResizeEvent *event)
{
    QLineEdit::resizeEvent(event);
    updateLayout();
}
