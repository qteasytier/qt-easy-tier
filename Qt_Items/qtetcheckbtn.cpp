#include "qtetcheckbtn.h"

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QStyleHints>
#include <QMouseEvent>
#include <QFontMetrics>

QtETCheckBtn::QtETCheckBtn(QWidget *parent)
    : QCheckBox(parent)
    , m_tipFontSize(9)
    , m_sliderPosition(0.0)
    , m_pressedOnSwitch(false)
{
    init();
}

QtETCheckBtn::QtETCheckBtn(const QString &text, QWidget *parent)
    : QCheckBox(text, parent)
    , m_tipFontSize(9)
    , m_sliderPosition(0.0)
    , m_pressedOnSwitch(false)
{
    init();
}

QtETCheckBtn::~QtETCheckBtn()
{
    if (m_animation) {
        m_animation->stop();
    }
}

void QtETCheckBtn::init()
{
    // 初始化动画
    m_animation = new QPropertyAnimation(this, "sliderPosition", this);
    m_animation->setDuration(ANIMATION_DURATION);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);

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

QSize QtETCheckBtn::sizeHint() const
{
    QFontMetrics fm(font());
    int textWidth = fm.horizontalAdvance(text());
    int textHeight = fm.height();

    int width = textWidth + TEXT_SWITCH_SPACING + SWITCH_WIDTH;
    int height = qMax(textHeight, SWITCH_HEIGHT);

    // 如果有提示文字，增加高度
    if (!m_tipText.isEmpty()) {
        QFont tipFont = font();
        tipFont.setPointSize(m_tipFontSize);
        QFontMetrics tipFm(tipFont);
        height += TIP_TEXT_SPACING + tipFm.height();
    }

    // 添加边距
    return QSize(width + 8, height + 4);
}

QSize QtETCheckBtn::minimumSizeHint() const
{
    return sizeHint();
}

QRect QtETCheckBtn::calculateSwitchRect() const
{
    // 开关在右侧，垂直居中
    int switchX = width() - SWITCH_WIDTH - 4;
    int switchY = (height() - SWITCH_HEIGHT) / 2;

    // 如果有提示文字，开关需要在上方区域
    if (!m_tipText.isEmpty()) {
        QFontMetrics fm(font());
        int textHeight = fm.height();
        switchY = (qMax(textHeight, SWITCH_HEIGHT) - SWITCH_HEIGHT) / 2;
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

    QRect switchRect = calculateSwitchRect();
    QRectF sliderRect = calculateSliderRect();

    // 绘制文字（左对齐，垂直居中）
    QFontMetrics fm(font());
    int textHeight = fm.height();
    int textY;
    
    if (!m_tipText.isEmpty()) {
        textY = (qMax(textHeight, SWITCH_HEIGHT) - textHeight) / 2 + fm.ascent();
    } else {
        textY = (height() - textHeight) / 2 + fm.ascent();
    }

    painter.setFont(font());
    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(4, textY, text());

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

    // 绘制提示文字
    if (!m_tipText.isEmpty()) {
        QFont tipFont = font();
        tipFont.setPointSize(m_tipFontSize);
        QFontMetrics tipFm(tipFont);

        painter.setFont(tipFont);
        painter.setPen(m_tipHighlightColor);

        int tipY = qMax(textHeight, SWITCH_HEIGHT) + TIP_TEXT_SPACING + tipFm.ascent();
        painter.drawText(4, tipY, m_tipText);
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
