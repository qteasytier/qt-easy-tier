#ifndef QTETCOMBOBOX_H
#define QTETCOMBOBOX_H

#include <QComboBox>
#include <QPropertyAnimation>

class QListView;

/// @brief 自定义下拉选择框控件
/// 基于 QComboBox 派生，完全自绘（圆角边框、悬停动画、按下态、禁用态）
class QtETComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(qreal borderOpacity READ borderOpacity WRITE setBorderOpacity)

public:
    explicit QtETComboBox(QWidget *parent = nullptr);
    ~QtETComboBox() override;

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
    void showPopup() override;

private:
    void init();
    void updateColorScheme();
    void startBorderAnimation(qreal targetOpacity);
    void updatePopupTheme();

private:
    QPropertyAnimation *m_borderAnimation = nullptr;
    qreal m_borderOpacity = 0.0;
    bool m_isPressed = false;

    QColor m_backgroundColor;
    QColor m_normalBorderColor;
    QColor m_highlightBorderColor;
    QColor m_pressedBorderColor;
    QColor m_disabledTextColor;

    QListView *m_popupView = nullptr;

    static constexpr int BORDER_RADIUS = 5;
    static constexpr int BORDER_WIDTH = 1;
    static constexpr int CONTENT_MARGIN = 7;
    static constexpr int ARROW_SIZE = 10;
    static constexpr int ARROW_HEIGHT = 6;
    static constexpr int BORDER_ANIMATION_DURATION = 200;
};

#endif // QTETCOMBOBOX_H
