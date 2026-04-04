#ifndef QTETCHECKBTN_H
#define QTETCHECKBTN_H

#include <QCheckBox>
#include <QPropertyAnimation>

/// @brief 自定义开关按钮控件
/// 左边显示文字，右边显示圆角开关
/// 支持过渡动效，下方可显示高亮提示文字
class QtETCheckBtn : public QCheckBox
{
    Q_OBJECT
    Q_PROPERTY(qreal sliderPosition READ sliderPosition WRITE setSliderPosition)
    Q_PROPERTY(qreal borderOpacity READ borderOpacity WRITE setBorderOpacity)
    Q_PROPERTY(QString tipText READ tipText WRITE setTipText)
    Q_PROPERTY(QString briefTip READ briefTip WRITE setBriefTip)
    Q_PROPERTY(int tipFontSize READ tipFontSize WRITE setTipFontSize)
    Q_PROPERTY(QColor activeColor READ activeColor WRITE setActiveColor)
    Q_PROPERTY(QColor inactiveColor READ inactiveColor WRITE setInactiveColor)

public:
    explicit QtETCheckBtn(QWidget *parent = nullptr);
    explicit QtETCheckBtn(const QString &text, QWidget *parent = nullptr);
    ~QtETCheckBtn() override;

    /// @brief 获取提示文字
    [[nodiscard]] QString tipText() const;
    /// @brief 设置提示文字
    void setTipText(const QString &text);

    /// @brief 获取简要提示文字（显示在控件下方）
    [[nodiscard]] QString briefTip() const;
    /// @brief 设置简要提示文字（显示在控件下方）
    void setBriefTip(const QString &text);

    /// @brief 获取提示文字大小
    [[nodiscard]] int tipFontSize() const;
    /// @brief 设置提示文字大小
    void setTipFontSize(int size);

    /// @brief 获取激活颜色
    [[nodiscard]] QColor activeColor() const;
    /// @brief 设置激活颜色
    void setActiveColor(const QColor &color);

    /// @brief 获取非激活颜色
    [[nodiscard]] QColor inactiveColor() const;
    /// @brief 设置非激活颜色
    void setInactiveColor(const QColor &color);

    /// @brief 获取滑块位置（用于动画）
    [[nodiscard]] qreal sliderPosition() const;
    /// @brief 设置滑块位置（用于动画）
    void setSliderPosition(qreal pos);

    /// @brief 获取边框不透明度（用于动画）
    [[nodiscard]] qreal borderOpacity() const;
    /// @brief 设置边框不透明度（用于动画）
    void setBorderOpacity(qreal opacity);

    /// @brief 获取是否启用无边框模式
    [[nodiscard]] bool isBorderless() const;
    /// @brief 设置是否启用无边框模式
    /// @param borderless true 启用无边框模式，边框和背景透明，边距为3px
    void setBorderless(bool borderless);

    /// @brief 获取推荐尺寸
    [[nodiscard]] QSize sizeHint() const override;
    /// @brief 获取最小尺寸
    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool event(QEvent *event) override;

private:
    /// @brief 初始化控件
    void init();
    /// @brief 更新颜色方案（根据深色/浅色模式）
    void updateColorScheme();
    /// @brief 计算开关区域
    [[nodiscard]] QRect calculateSwitchRect() const;
    /// @brief 计算滑块区域
    [[nodiscard]] QRectF calculateSliderRect() const;

private:
    QString m_tipText;              ///< 提示文字
    QString m_briefTip;             ///< 简要提示文字（显示在控件下方）
    int m_tipFontSize;              ///< 提示文字大小
    QColor m_activeColor;           ///< 激活状态颜色
    QColor m_inactiveColor;         ///< 非激活状态颜色
    QColor m_sliderColor;           ///< 滑块颜色
    QColor m_tipHighlightColor;     ///< 提示文字高亮颜色

    QPropertyAnimation *m_animation;        ///< 滑块动画
    QPropertyAnimation *m_borderAnimation;  ///< 边框高亮动画
    qreal m_sliderPosition;         ///< 滑块位置 (0.0 ~ 1.0)
    qreal m_borderOpacity;          ///< 边框高亮不透明度 (0.0 ~ 1.0)
    bool m_pressedOnSwitch;         ///< 按下时是否在开关区域
    bool m_borderless;              ///< 无边框模式标志

    // 尺寸常量
    static constexpr int SWITCH_WIDTH = 40;     ///< 开关宽度
    static constexpr int SWITCH_HEIGHT = 22;    ///< 开关高度
    static constexpr int SLIDER_MARGIN = 2;     ///< 滑块边距
    static constexpr int BORDER_RADIUS = 11;    ///< 边框圆角
    static constexpr int TEXT_SWITCH_SPACING = 8; ///< 文字与开关间距
    static constexpr int TIP_TEXT_SPACING = 8;  ///< 提示文字与控件间距
    static constexpr int ANIMATION_DURATION = 150; ///< 动画时长(ms)
    static constexpr int BORDER_ANIMATION_DURATION = 200; ///< 边框动画时长(ms)
};

#endif // QTETCHECKBTN_H
