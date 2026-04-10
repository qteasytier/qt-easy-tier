#ifndef QTETLINEEDIT_H
#define QTETLINEEDIT_H

#include <QLineEdit>
#include <QPropertyAnimation>

class QPushButton;  // 前向声明

/// @brief 自定义行编辑控件
/// 完全自定义绘制边框和背景，不使用 QSS
/// 文本输入使用 QLineEdit 原生功能（仅用于输入处理），通过 QPalette 设置颜色
/// 支持悬停边框高亮动画效果、聚焦状态高亮、密码显示切换
class QtETLineEdit : public QLineEdit
{
    Q_OBJECT
    Q_PROPERTY(qreal borderOpacity READ borderOpacity WRITE setBorderOpacity)

public:
    explicit QtETLineEdit(QWidget *parent = nullptr);
    ~QtETLineEdit() override;

    /// @brief 获取边框不透明度（用于动画）
    [[nodiscard]] qreal borderOpacity() const;
    /// @brief 设置边框不透明度（用于动画）
    void setBorderOpacity(qreal opacity);

    /// @brief 设置是否启用密码模式切换按钮
    void setEchoModeToggleEnabled(bool enabled);
    /// @brief 获取是否启用密码模式切换按钮
    [[nodiscard]] bool isEchoModeToggleEnabled() const;

    /// @brief 获取推荐尺寸
    [[nodiscard]] QSize sizeHint() const override;
    /// @brief 获取最小尺寸
    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    /// @brief 初始化控件
    void init();
    /// @brief 更新颜色方案（根据深色/浅色模式）
    void updateColorScheme();
    /// @brief 更新密码切换按钮图标
    void updateButtonIcon();
    /// @brief 更新布局（按钮位置、文本边距）
    void updateLayout();

private:
    QPropertyAnimation *m_borderAnimation;  ///< 边框高亮动画
    qreal m_borderOpacity;                  ///< 边框高亮不透明度 (0.0 ~ 1.0)
    bool m_hasFocus;                        ///< 聚焦状态标志
    bool m_echoModeToggleEnabled;           ///< 密码切换按钮启用标志
    QPushButton *m_echoToggleButton;        ///< 密码切换按钮

    QColor m_backgroundColor;               ///< 背景颜色
    QColor m_normalBorderColor;             ///< 普通边框颜色
    QColor m_highlightBorderColor;          ///< 高亮边框颜色
    QColor m_focusBorderColor;              ///< 聚焦边框颜色
    QColor m_textColor;                     ///< 文字颜色

    // 尺寸常量
    static constexpr int BORDER_RADIUS = 5;         ///< 边框圆角
    static constexpr int BORDER_WIDTH = 1;          ///< 边框宽度
    static constexpr int CONTENT_MARGIN_V = 7;      ///< 内容垂直边距（上下）
    static constexpr int CONTENT_MARGIN_H = 4;      ///< 内容水平边距（左右）
    static constexpr int BUTTON_SIZE = 20;          ///< 密码切换按钮大小
    static constexpr int BUTTON_SPACING = 4;        ///< 按钮与文本间距
    static constexpr int BORDER_ANIMATION_DURATION = 200;  ///< 边框动画时长(ms)
};

#endif // QTETLINEEDIT_H