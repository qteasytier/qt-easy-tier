#ifndef QTETLINEEDIT_H
#define QTETLINEEDIT_H

#include <QLineEdit>
#include <QPropertyAnimation>
#include <QAction>

/// @brief 自定义行编辑控件
/// 完全自定义绘制边框和背景，支持悬停/聚焦高亮、密码显示切换
class QtETLineEdit : public QLineEdit
{
    Q_OBJECT
    Q_PROPERTY(qreal borderOpacity READ borderOpacity WRITE setBorderOpacity)

public:
    explicit QtETLineEdit(QWidget *parent = nullptr);
    ~QtETLineEdit() override;

    [[nodiscard]] qreal borderOpacity() const;
    void setBorderOpacity(qreal opacity);

    void setEchoModeToggleEnabled(bool enabled);
    [[nodiscard]] bool isEchoModeToggleEnabled() const;

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void init();
    void updateColorScheme();
    void updateButtonIcon();
    void updateLayout();
    void startBorderAnimation(qreal targetOpacity);

private:
    QPropertyAnimation *m_borderAnimation = nullptr;
    qreal m_borderOpacity = 0.0;
    bool m_hasFocus = false;
    bool m_echoModeToggleEnabled = true;
    QAction *m_echoToggleAction = nullptr;

    QColor m_backgroundColor;
    QColor m_normalBorderColor;
    QColor m_highlightBorderColor;
    QColor m_focusBorderColor;
    QColor m_textColor;

    static constexpr int BORDER_RADIUS = 5;
    static constexpr int BORDER_WIDTH = 1;
    static constexpr int CONTENT_MARGIN_V = 7;
    static constexpr int CONTENT_MARGIN_H = 4;
    static constexpr int BUTTON_SIZE = 20;
    static constexpr int BUTTON_SPACING = 4;
    static constexpr int BORDER_ANIMATION_DURATION = 200;
};

#endif // QTETLINEEDIT_H
