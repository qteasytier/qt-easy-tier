#ifndef QTETTABWIDGET_H
#define QTETTABWIDGET_H

#include <QTabWidget>
#include <QTabBar>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QVector>

class QtETTabBar;

/// @brief 自定义 TabWidget 控件
class QtETTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit QtETTabWidget(QWidget *parent = nullptr);
    ~QtETTabWidget() override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void init();
    void updateColorScheme();

    QtETTabBar *m_tabBar = nullptr;
    QColor m_backgroundColor;
    QColor m_tabBarBackgroundColor;
    QColor m_borderColor;
};

/// @brief 自定义 TabBar 控件
class QtETTabBar : public QTabBar
{
    Q_OBJECT
    Q_PROPERTY(qreal indicatorWidth READ indicatorWidth WRITE setIndicatorWidth)
    Q_PROPERTY(qreal indicatorX READ indicatorX WRITE setIndicatorX)
    Q_PROPERTY(qreal hoverOpacity READ hoverOpacity WRITE setHoverOpacity)

public:
    explicit QtETTabBar(QWidget *parent = nullptr);
    ~QtETTabBar() override;

    [[nodiscard]] qreal indicatorWidth() const;
    void setIndicatorWidth(qreal width);
    [[nodiscard]] qreal indicatorX() const;
    void setIndicatorX(qreal x);
    [[nodiscard]] qreal hoverOpacity() const;
    void setHoverOpacity(qreal opacity);
    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;

private:
    void init();
    void updateColorScheme();
    void recalculateTabWidths();
    [[nodiscard]] QRect calculateTabRect(int index) const;
    void animateIndicatorTo(int index);
    [[nodiscard]] int getTabTextWidth(int index) const;

private:
    QColor m_textColor;
    QColor m_textInactiveColor;
    QColor m_indicatorColor;
    QColor m_hoverBackgroundColor;
    QColor m_selectedBackgroundColor;

    QPropertyAnimation *m_indicatorWidthAnim = nullptr;
    QPropertyAnimation *m_indicatorXAnim = nullptr;
    QParallelAnimationGroup *m_indicatorAnimGroup = nullptr;

    qreal m_indicatorWidth = 0;
    qreal m_indicatorX = 0;
    qreal m_hoverOpacity = 0.0;
    int m_hoveredTabIndex = -1;
    int m_previousIndex = -1;
    bool m_isInitialShow = true;

    QVector<int> m_tabTextWidths;
    QVector<int> m_tabCumulativeWidths;

    static constexpr int TAB_HEIGHT = 36;
    static constexpr int TAB_LEFT_PADDING = 16;
    static constexpr int TAB_RIGHT_PADDING = 16;
    static constexpr int INDICATOR_HEIGHT = 3;
    static constexpr int BORDER_RADIUS = 4;
    static constexpr int ANIMATION_DURATION = 200;
};

#endif // QTETTABWIDGET_H
