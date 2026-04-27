#ifndef QTETCHECKBTN_H
#define QTETCHECKBTN_H

#include <QCheckBox>
#include <QPropertyAnimation>

/// @brief 自定义开关按钮控件
/// 左边显示文字，右边显示圆角开关，支持过渡动效
class QtETCheckBtn : public QCheckBox
{
    Q_OBJECT
    Q_PROPERTY(qreal sliderPosition READ sliderPosition WRITE setSliderPosition)
    Q_PROPERTY(qreal borderOpacity READ borderOpacity WRITE setBorderOpacity)
    Q_PROPERTY(QString tipText READ tipText WRITE setTipText)
    Q_PROPERTY(QString briefTip READ briefTip WRITE setBriefTip)
    Q_PROPERTY(int tipFontSize READ tipFontSize WRITE setTipFontSize)

public:
    explicit QtETCheckBtn(QWidget *parent = nullptr);
    explicit QtETCheckBtn(const QString &text, QWidget *parent = nullptr);
    ~QtETCheckBtn() override;

    [[nodiscard]] QString tipText() const;
    void setTipText(const QString &text);

    [[nodiscard]] QString briefTip() const;
    void setBriefTip(const QString &text);

    [[nodiscard]] int tipFontSize() const;
    void setTipFontSize(int size);

    [[nodiscard]] qreal sliderPosition() const;
    void setSliderPosition(qreal pos);

    [[nodiscard]] qreal borderOpacity() const;
    void setBorderOpacity(qreal opacity);

    [[nodiscard]] bool isBorderless() const;
    void setBorderless(bool borderless);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

    void setChecked(bool checked);

public slots:
    void setCheckedSlot(bool checked);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void nextCheckState() override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void init();
    void updateColorScheme();
    [[nodiscard]] QRect calculateSwitchRect() const;
    [[nodiscard]] QRectF calculateSliderRect() const;
    void startSliderAnimation(qreal targetPos);
    void startBorderAnimation(qreal targetOpacity);

private:
    QString m_tipText;
    QString m_briefTip;
    int m_tipFontSize = 9;
    QColor m_activeColor;
    QColor m_inactiveColor;
    QColor m_sliderColor;
    QColor m_tipHighlightColor;

    QPropertyAnimation *m_animation = nullptr;
    QPropertyAnimation *m_borderAnimation = nullptr;
    qreal m_sliderPosition = 0.0;
    qreal m_borderOpacity = 0.0;
    bool m_pressedOnSwitch = false;
    bool m_borderless = false;
    bool m_isProgrammaticChange = false;

    static constexpr int SWITCH_WIDTH = 40;
    static constexpr int SWITCH_HEIGHT = 22;
    static constexpr int SLIDER_MARGIN = 2;
    static constexpr int BORDER_RADIUS = 11;
    static constexpr int TEXT_SWITCH_SPACING = 8;
    static constexpr int TIP_TEXT_SPACING = 8;
    static constexpr int ANIMATION_DURATION = 150;
    static constexpr int BORDER_ANIMATION_DURATION = 200;
};

#endif // QTETCHECKBTN_H
