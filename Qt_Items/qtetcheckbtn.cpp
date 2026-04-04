#include "qtetcheckbtn.h"

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleHints>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QFontMetrics>
#include <QEvent>

QtETCheckBtn::QtETCheckBtn(QWidget *parent)
    : QCheckBox(parent)
    , m_tipFontSize(9)
    , m_sliderPosition(0.0)
    , m_borderOpacity(0.0)
    , m_pressedOnSwitch(false)
    , m_borderless(false)
{
    init();
}

QtETCheckBtn::QtETCheckBtn(const QString &text, QWidget *parent)
    : QCheckBox(text, parent)
    , m_tipFontSize(9)
    , m_sliderPosition(0.0)
    , m_borderOpacity(0.0)
    , m_pressedOnSwitch(false)
    , m_borderless(false)
{
    init();
}

QtETCheckBtn::~QtETCheckBtn()
{
    if (m_animation) {
        m_animation->stop();
    }
    if (m_borderAnimation) {
        m_borderAnimation->stop();
    }
}

void QtETCheckBtn::init()
{
    // 初始化滑块动画
    m_animation = new QPropertyAnimation(this, "sliderPosition", this);
    m_animation->setDuration(ANIMATION_DURATION);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);

    // 初始化边框高亮动画
    m_borderAnimation = new QPropertyAnimation(this, "borderOpacity", this);
    m_borderAnimation->setDuration(BORDER_ANIMATION_DURATION);
    m_borderAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // 初始化颜色
    updateColorScheme();

    // 监听系统主题变化
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        updateColorScheme();
        update();
    });

    // 监听选中状态变化
    connect(this, &QCheckBox::toggled, this, [this](bool checked) {
        m_animation->stop();
        m_animation->setStartValue(m_sliderPosition);
        m_animation->setEndValue(checked ? 1.0 : 0.0);
        m_animation->start();
    });

    // 初始化滑块位置
    m_sliderPosition = isChecked() ? 1.0 : 0.0;
}

void QtETCheckBtn::updateColorScheme()
{
    const bool isDark = (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark);

    if (isDark) {
        // 深色模式
        m_activeColor = QColor("#66ccff");      // 蓝色激活
        m_inactiveColor = QColor(80, 80, 80);      // 深灰非激活
        m_sliderColor = QColor(255, 255, 255);     // 白色滑块
        m_tipHighlightColor = QColor("#66ccff"); // 蓝色高亮
    } else {
        // 浅色模式
        m_activeColor = QColor("#66ccff");      // 蓝色激活
        m_inactiveColor = QColor(200, 200, 200);   // 浅灰非激活
        m_sliderColor = QColor(255, 255, 255);     // 白色滑块
        m_tipHighlightColor = QColor("#66ccff"); // 高亮蓝
    }

    update();
}

QString QtETCheckBtn::tipText() const
{
    return m_tipText;
}

void QtETCheckBtn::setTipText(const QString &text)
{
    if (m_tipText != text) {
        m_tipText = text;
        updateGeometry();
        update();
    }
}

QString QtETCheckBtn::briefTip() const
{
    return m_briefTip;
}

void QtETCheckBtn::setBriefTip(const QString &text)
{
    if (m_briefTip != text) {
        m_briefTip = text;
        updateGeometry();
        update();
    }
}

int QtETCheckBtn::tipFontSize() const
{
    return m_tipFontSize;
}

void QtETCheckBtn::setTipFontSize(int size)
{
    if (size > 0 && m_tipFontSize != size) {
        m_tipFontSize = size;
        updateGeometry();
        update();
    }
}

QColor QtETCheckBtn::activeColor() const
{
    return m_activeColor;
}

void QtETCheckBtn::setActiveColor(const QColor &color)
{
    if (m_activeColor != color) {
        m_activeColor = color;
        update();
    }
}

QColor QtETCheckBtn::inactiveColor() const
{
    return m_inactiveColor;
}

void QtETCheckBtn::setInactiveColor(const QColor &color)
{
    if (m_inactiveColor != color) {
        m_inactiveColor = color;
        update();
    }
}

qreal QtETCheckBtn::sliderPosition() const
{
    return m_sliderPosition;
}

void QtETCheckBtn::setSliderPosition(qreal pos)
{
    if (!qFuzzyCompare(m_sliderPosition, pos)) {
        m_sliderPosition = qBound(0.0, pos, 1.0);
        update();
    }
}

qreal QtETCheckBtn::borderOpacity() const
{
    return m_borderOpacity;
}

void QtETCheckBtn::setBorderOpacity(qreal opacity)
{
    if (!qFuzzyCompare(m_borderOpacity, opacity)) {
        m_borderOpacity = qBound(0.0, opacity, 1.0);
        update();
    }
}

bool QtETCheckBtn::isBorderless() const
{
    return m_borderless;
}

void QtETCheckBtn::setBorderless(bool borderless)
{
    if (m_borderless != borderless) {
        m_borderless = borderless;
        updateGeometry();
        update();
    }
}

QSize QtETCheckBtn::sizeHint() const
{
    QFontMetrics fm(font());
    int textWidth = fm.horizontalAdvance(text());
    int textHeight = fm.height();

    // 根据无边框模式设置不同的边距
    int margin = m_borderless ? 3 : 10;

    // 内容区域宽度：左边距 + 文字 + 间距 + 开关 + 右边距
    int width = textWidth + TEXT_SWITCH_SPACING + SWITCH_WIDTH + margin * 2;

    // 计算高度
    int height;
    if (!m_briefTip.isEmpty()) {
        // 有 briefTip 时
        QFont tipFont = font();
        tipFont.setPointSize(m_tipFontSize);
        QFontMetrics tipFm(tipFont);
        // 上边距 + 内容高度 + 间距 + tip高度 + 下边距
        height = margin + qMax(textHeight, SWITCH_HEIGHT) + TIP_TEXT_SPACING + tipFm.height() + margin;
    } else {
        // 无 briefTip 时，上下边距 + 内容高度
        height = margin * 2 + qMax(textHeight, SWITCH_HEIGHT);
    }

    return QSize(width, height);
}

QSize QtETCheckBtn::minimumSizeHint() const
{
    return sizeHint();
}

QRect QtETCheckBtn::calculateSwitchRect() const
{
    // 根据无边框模式设置不同的边距
    int margin = m_borderless ? 3 : 10;

    // 开关在右侧
    int switchX = width() - SWITCH_WIDTH - margin;
    int switchY;

    if (!m_briefTip.isEmpty()) {
        // 有 briefTip 时，开关与文字在内容区域垂直居中
        int contentHeight = qMax(fontMetrics().height(), SWITCH_HEIGHT);
        switchY = margin + (contentHeight - SWITCH_HEIGHT) / 2;
    } else {
        // 无 briefTip 时垂直居中
        switchY = (height() - SWITCH_HEIGHT) / 2;
    }

    return QRect(switchX, switchY, SWITCH_WIDTH, SWITCH_HEIGHT);
}

QRectF QtETCheckBtn::calculateSliderRect() const
{
    QRect switchRect = calculateSwitchRect();
    qreal sliderSize = switchRect.height() - 2 * SLIDER_MARGIN;

    // 计算滑块位置：从左边滑动到右边
    qreal sliderX = switchRect.x() + SLIDER_MARGIN + 
                    m_sliderPosition * (switchRect.width() - 2 * SLIDER_MARGIN - sliderSize);
    qreal sliderY = switchRect.y() + SLIDER_MARGIN;

    return QRectF(sliderX, sliderY, sliderSize, sliderSize);
}

void QtETCheckBtn::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 非无边框模式时绘制控件边框和背景
    if (!m_borderless) {
        // 绘制控件边框（类似 QPushButton 样式）
        QRect borderRect = rect().adjusted(1, 1, -1, -1);
        constexpr int borderRadius = 5;

        // 获取边框颜色
        QColor borderColor;
        QColor backgroundColor = palette().color(QPalette::Button);

        // 判断是否为暗色模式
        const bool isDark = (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark);

        // 基础边框颜色（非悬停状态）
        QColor normalBorderColor;
        if (isDark) {
            normalBorderColor = palette().color(QPalette::Light);
            // 如果颜色太深，使用更浅的颜色
            if (normalBorderColor.lightnessF() < 0.3) {
                normalBorderColor = QColor(100, 100, 100);
            }
        } else {
            normalBorderColor = palette().color(QPalette::Mid);
            // 如果颜色太浅，使用更深的颜色
            if (normalBorderColor.lightnessF() > 0.9) {
                normalBorderColor = palette().color(QPalette::Dark);
            }
        }

        // 高亮边框颜色
        QColor highlightBorderColor = palette().color(QPalette::Highlight);

        // 根据 m_borderOpacity 混合颜色
        borderColor = QColor::fromRgbF(
            normalBorderColor.redF() * (1 - m_borderOpacity) + highlightBorderColor.redF() * m_borderOpacity,
            normalBorderColor.greenF() * (1 - m_borderOpacity) + highlightBorderColor.greenF() * m_borderOpacity,
            normalBorderColor.blueF() * (1 - m_borderOpacity) + highlightBorderColor.blueF() * m_borderOpacity,
            normalBorderColor.alphaF() * (1 - m_borderOpacity) + highlightBorderColor.alphaF() * m_borderOpacity
        );

        // 绘制背景
        QPainterPath bgPath;
        bgPath.addRoundedRect(borderRect, borderRadius, borderRadius);
        painter.fillPath(bgPath, backgroundColor);

        // 绘制边框
        painter.setPen(QPen(borderColor, 1));
        painter.drawPath(bgPath);
    }

    QRect switchRect = calculateSwitchRect();
    QRectF sliderRect = calculateSliderRect();

    // 绘制文字（左对齐）
    QFontMetrics fm(font());
    int textHeight = fm.height();
    int textY;

    // 根据无边框模式设置不同的边距
    int contentMargin = m_borderless ? 3 : 10;

    if (!m_briefTip.isEmpty()) {
        // 有 briefTip 时，文字与开关在内容区域垂直居中
        // 内容区域顶部 = contentMargin
        // 内容区域高度 = qMax(textHeight, SWITCH_HEIGHT)
        int contentHeight = qMax(textHeight, SWITCH_HEIGHT);
        textY = contentMargin + (contentHeight - textHeight) / 2 + fm.ascent();
    } else {
        textY = (height() - textHeight) / 2 + fm.ascent();
    }

    painter.setFont(font());
    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(contentMargin, textY, text());

    // 绘制开关背景（带圆角）
    QPainterPath switchPath;
    switchPath.addRoundedRect(switchRect, BORDER_RADIUS, BORDER_RADIUS);
    
    // 背景颜色插值
    QColor bgColor = QColor::fromHsvF(
        m_inactiveColor.toHsv().hueF(),
        m_inactiveColor.toHsv().saturationF() * (1 - m_sliderPosition) + m_activeColor.toHsv().saturationF() * m_sliderPosition,
        m_inactiveColor.toHsv().valueF() * (1 - m_sliderPosition) + m_activeColor.toHsv().valueF() * m_sliderPosition
    );

    // 简化：直接根据滑块位置选择颜色
    if (m_sliderPosition > 0.5) {
        bgColor = m_activeColor;
    } else {
        bgColor = m_inactiveColor;
    }
    
    painter.fillPath(switchPath, bgColor);

    // 绘制滑块（圆形）
    QPainterPath sliderPath;
    sliderPath.addEllipse(sliderRect);
    painter.fillPath(sliderPath, m_sliderColor);

    // 绘制 briefTip 文字（高亮显示在下方，单行显示）
    if (!m_briefTip.isEmpty()) {
        QFont tipFont = font();
        tipFont.setPointSize(m_tipFontSize);
        QFontMetrics tipFm(tipFont);

        painter.setFont(tipFont);
        painter.setPen(m_tipHighlightColor);

        // briefTip 位置：上边距 + 内容高度 + 间距
        int contentHeight = qMax(fm.height(), SWITCH_HEIGHT);
        int tipY = contentMargin + contentHeight + TIP_TEXT_SPACING + tipFm.ascent();
        painter.drawText(contentMargin, tipY, m_briefTip);
    }
}

void QtETCheckBtn::resizeEvent(QResizeEvent *event)
{
    QCheckBox::resizeEvent(event);
    update();
}

void QtETCheckBtn::mousePressEvent(QMouseEvent *event)
{
    // 记录按下时是否在开关区域
    QRect switchRect = calculateSwitchRect();
    m_pressedOnSwitch = switchRect.contains(event->pos());
    
    if (m_pressedOnSwitch) {
        // 接受事件，阻止传播
        event->accept();
    } else {
        // 不在开关区域，忽略事件
        event->ignore();
    }
}

void QtETCheckBtn::mouseReleaseEvent(QMouseEvent *event)
{
    // 只有按下时在开关区域，且释放时也在开关区域才切换状态
    QRect switchRect = calculateSwitchRect();
    
    if (m_pressedOnSwitch && switchRect.contains(event->pos())) {
        // 切换状态
        setChecked(!isChecked());
        event->accept();
    } else {
        event->ignore();
    }
    
    m_pressedOnSwitch = false;
}

void QtETCheckBtn::enterEvent(QEnterEvent *event)
{
    QCheckBox::enterEvent(event);
    
    // 启动边框高亮动画（渐显）
    m_borderAnimation->stop();
    m_borderAnimation->setStartValue(m_borderOpacity);
    m_borderAnimation->setEndValue(1.0);
    m_borderAnimation->start();
}

void QtETCheckBtn::leaveEvent(QEvent *event)
{
    QCheckBox::leaveEvent(event);
    
    // 启动边框淡出动画（渐隐）
    m_borderAnimation->stop();
    m_borderAnimation->setStartValue(m_borderOpacity);
    m_borderAnimation->setEndValue(0.0);
    m_borderAnimation->start();
}

bool QtETCheckBtn::event(QEvent *event)
{
    // 监听 briefTip 变化，更新布局
    if (event->type() == QEvent::DynamicPropertyChange) {
        updateGeometry();
        update();
    }
    
    return QCheckBox::event(event);
}
