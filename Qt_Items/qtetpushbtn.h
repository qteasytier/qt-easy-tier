#ifndef QTETPUSHBTN_H
#define QTETPUSHBTN_H

#include <QPushButton>
#include <QPropertyAnimation>

/// @brief 自定义推送按钮控件
/// 支持悬停边框高亮动画效果，按下时边框变深，禁用态灰色显示
class QtETPushBtn : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(qreal borderOpacity READ borderOpacity WRITE setBorderOpacity)

public:
    explicit QtETPushBtn(QWidget *parent = nullptr);
    explicit QtETPushBtn(const QString &text, QWidget *parent = nullptr);
    ~QtETPushBtn() override;

    [[nodiscard]] qreal borderOpacity() const;
    void setBorderOpacity(qreal opacity);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void init();
    void updateColorScheme();
    void startBorderAnimation(qreal targetOpacity);

private:
    QPropertyAnimation *m_borderAnimation = nullptr;
    qreal m_borderOpacity = 0.0;
    bool m_isPressed = false;

    QColor m_backgroundColor;
    QColor m_normalBorderColor;
    QColor m_highlightBorderColor;
    QColor m_pressedBorderColor;
    QColor m_disabledTextColor;

    static constexpr int BORDER_RADIUS = 5;
    static constexpr int BORDER_WIDTH = 1;
    static constexpr int CONTENT_MARGIN = 7;
    static constexpr int BORDER_ANIMATION_DURATION = 200;
};

#endif // QTETPUSHBTN_H
