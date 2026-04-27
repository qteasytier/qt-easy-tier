#ifndef QTETDRAWUTILS_H
#define QTETDRAWUTILS_H

#include <QColor>
#include <QPainter>
#include <QPainterPath>

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

#endif // QTETDRAWUTILS_H
