#ifndef QTETPUSHBTN_H
#define QTETPUSHBTN_H

#include <QPushButton>
#include <QPropertyAnimation>

/// @brief 自定义推送按钮控件
/// 参考 QtETCheckBtn 的边框和背景样式
/// 支持悬停边框高亮动画效果，按下时边框变深
class QtETPushBtn : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(qreal borderOpacity READ borderOpacity WRITE setBorderOpacity)

public:
    explicit QtETPushBtn(QWidget *parent = nullptr);
    explicit QtETPushBtn(const QString &text, QWidget *parent = nullptr);
    ~QtETPushBtn() override;

    /// @brief 获取边框不透明度（用于动画）
    [[nodiscard]] qreal borderOpacity() const;
    /// @brief 设置边框不透明度（用于动画）
    void setBorderOpacity(qreal opacity);

    /// @brief 获取推荐尺寸
    [[nodiscard]] QSize sizeHint() const override;
    /// @brief 获取最小尺寸
    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    /// @brief 初始化控件
    void init();
    /// @brief 更新颜色方案（根据深色/浅色模式）
    void updateColorScheme();

private:
    QPropertyAnimation *m_borderAnimation;  ///< 边框高亮动画
    qreal m_borderOpacity;                  ///< 边框高亮不透明度 (0.0 ~ 1.0)
    bool m_isPressed;                       ///< 按下状态标志

    QColor m_backgroundColor;               ///< 背景颜色
    QColor m_normalBorderColor;             ///< 普通边框颜色
    QColor m_highlightBorderColor;          ///< 高亮边框颜色
    QColor m_pressedBorderColor;            ///< 按下边框颜色

    // 尺寸常量
    static constexpr int BORDER_RADIUS = 5;         ///< 边框圆角
    static constexpr int BORDER_WIDTH = 1;          ///< 边框宽度
    static constexpr int CONTENT_MARGIN = 7;        ///< 内容边距（上下左右）
    static constexpr int BORDER_ANIMATION_DURATION = 200;  ///< 边框动画时长(ms)
};

#endif // QTETPUSHBTN_H
