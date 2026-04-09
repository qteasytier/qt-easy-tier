#ifndef QTETTABWIDGET_H
#define QTETTABWIDGET_H

#include <QTabWidget>
#include <QTabBar>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

class QtETTabBar;

/// @brief 自定义 TabWidget 控件
/// 背景色与主页面一致，顶部 Tab 栏现代化设计
/// 支持深色/浅色模式自动切换，带平滑动画效果
class QtETTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit QtETTabWidget(QWidget *parent = nullptr);
    ~QtETTabWidget() override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    /// @brief 初始化控件
    void init();
    /// @brief 更新颜色方案（根据深色/浅色模式）
    void updateColorScheme();
    /// @brief 设置 TabBar 样式
    void setupTabBarStyle();

private:
    QtETTabBar *m_tabBar;           ///< 自定义 TabBar
    QColor m_backgroundColor;       ///< 背景颜色
    QColor m_tabBarBackgroundColor; ///< Tab 栏背景颜色（比主背景稍深）
    QColor m_borderColor;           ///< 边框颜色
};

/// @brief 自定义 TabBar 控件
/// 现代化设计，支持底部指示器、圆角背景、平滑动画
class QtETTabBar : public QTabBar
{
    Q_OBJECT
    Q_PROPERTY(qreal indicatorWidth READ indicatorWidth WRITE setIndicatorWidth)
    Q_PROPERTY(qreal indicatorX READ indicatorX WRITE setIndicatorX)
    Q_PROPERTY(qreal hoverOpacity READ hoverOpacity WRITE setHoverOpacity)

public:
    explicit QtETTabBar(QWidget *parent = nullptr);
    ~QtETTabBar() override;

    /// @brief 获取指示器宽度（用于动画）
    [[nodiscard]] qreal indicatorWidth() const;
    /// @brief 设置指示器宽度（用于动画）
    void setIndicatorWidth(qreal width);

    /// @brief 获取指示器 X 坐标（用于动画）
    [[nodiscard]] qreal indicatorX() const;
    /// @brief 设置指示器 X 坐标（用于动画）
    void setIndicatorX(qreal x);

    /// @brief 获取悬停不透明度（用于动画）
    [[nodiscard]] qreal hoverOpacity() const;
    /// @brief 设置悬停不透明度（用于动画）
    void setHoverOpacity(qreal opacity);

    /// @brief 获取推荐尺寸
    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool event(QEvent *event) override;

private:
    /// @brief 初始化控件
    void init();
    /// @brief 更新颜色方案（根据深色/浅色模式）
    void updateColorScheme();
    /// @brief 计算指定索引 Tab 的矩形区域
    [[nodiscard]] QRect calculateTabRect(int index) const;
    /// @brief 动画移动指示器到指定索引
    void animateIndicatorTo(int index);
    /// @brief 获取 Tab 文本所需宽度
    [[nodiscard]] int getTabTextWidth(int index) const;

private:
    // 颜色
    QColor m_textColor;                 ///< 文字颜色
    QColor m_textInactiveColor;         ///< 非活动文字颜色
    QColor m_indicatorColor;            ///< 指示器颜色
    QColor m_hoverBackgroundColor;      ///< 悬停背景颜色
    QColor m_selectedBackgroundColor;   ///< 选中背景颜色

    // 动画
    QPropertyAnimation *m_indicatorWidthAnim;    ///< 指示器宽度动画
    QPropertyAnimation *m_indicatorXAnim;        ///< 指示器位置动画
    QParallelAnimationGroup *m_indicatorAnimGroup; ///< 指示器动画组

    // 状态
    qreal m_indicatorWidth;             ///< 指示器宽度
    qreal m_indicatorX;                 ///< 指示器 X 坐标
    qreal m_hoverOpacity;               ///< 悬停不透明度
    int m_hoveredTabIndex;              ///< 当前悬停的 Tab 索引
    int m_previousIndex;                ///< 上一个选中的索引（用于动画）

    // 尺寸常量
    static constexpr int TAB_HEIGHT = 36;              ///< Tab 高度
    static constexpr int TAB_LEFT_PADDING = 16;        ///< Tab 左边距
    static constexpr int TAB_RIGHT_PADDING = 16;       ///< Tab 右边距
    static constexpr int TAB_SPACING = 0;              ///< Tab 间距
    static constexpr int INDICATOR_HEIGHT = 3;         ///< 指示器高度
    static constexpr int INDICATOR_BOTTOM_MARGIN = 0;  ///< 指示器底部边距
    static constexpr int BORDER_RADIUS = 4;            ///< 圆角半径
    static constexpr int ANIMATION_DURATION = 200;     ///< 动画时长(ms)
};

#endif // QTETTABWIDGET_H
