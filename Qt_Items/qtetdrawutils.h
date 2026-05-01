#ifndef QTETDRAWUTILS_H
#define QTETDRAWUTILS_H

#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QObject>

class QAbstractScrollArea;
class QPropertyAnimation;

/// @brief 公共绘制工具类
/// 提取各控件中重复的绘制逻辑
class QtETDrawUtils
{
public:
    QtETDrawUtils() = delete;

    /// @brief 混合两种颜色，根据不透明度插值
    static QColor blendColors(const QColor &from, const QColor &to, qreal opacity);

    /// @brief 绘制圆角矩形背景
    static void drawRoundedRect(QPainter *painter, const QRect &rect, int radius,
                                 const QColor &fillColor, const QColor &borderColor,
                                 int borderWidth = 1);

    /// @brief 绘制带圆角的填充区域（无边框）
    static void drawRoundedFill(QPainter *painter, const QRect &rect, int radius,
                                 const QColor &fillColor);

    /// @brief 绘制底部指示器（圆角矩形）
    static void drawBottomIndicator(QPainter *painter, qreal x, qreal y,
                                     qreal width, qreal height, const QColor &color);

    /// @brief 绘制标签（带圆角背景的文本标签）
    static void drawLabel(QPainter *painter, int x, int y,
                           const QString &text, const QColor &bgColor,
                           const QColor &textColor, int fontSize,
                           int hPadding = 6, int vPadding = 2, int radius = 6);

    /// @brief 计算标签尺寸
    static QSize labelSize(const QString &text, int fontSize,
                            int hPadding = 6, int vPadding = 2);

    /// @brief 安全地结束 painter（避免双重 end 问题）
    static void safeEndPainter(QPainter *painter);
};

/// @brief 平滑滚动组件
/// 通过事件过滤器拦截 QAbstractScrollArea 的滚轮事件，使用 QPropertyAnimation 实现平滑滚动。
/// 支持 QScrollArea、QTextEdit 等所有 QAbstractScrollArea 子类。
class QtETSmoothScroll : public QObject
{
    Q_OBJECT

public:
    explicit QtETSmoothScroll(QAbstractScrollArea *scrollArea, QObject *parent = nullptr);

    /// @brief 便捷方法：创建并安装到目标滚动区域的 viewport
    static QtETSmoothScroll* install(QAbstractScrollArea *scrollArea,
                                     int duration = 150, int step = 80);

    void setDuration(int ms);
    void setScrollStep(int step);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QAbstractScrollArea *m_scrollArea;
    QPropertyAnimation *m_animation = nullptr;
    int m_targetValue = 0;
    int m_duration = 150;
    int m_scrollStep = 80;
};

#endif // QTETDRAWUTILS_H
