#include "qtetdrawutils.h"

#include <QFontMetrics>

QColor QtETDrawUtils::blendColors(const QColor &from, const QColor &to, qreal opacity)
{
    return QColor::fromRgbF(
        from.redF() * (1.0 - opacity) + to.redF() * opacity,
        from.greenF() * (1.0 - opacity) + to.greenF() * opacity,
        from.blueF() * (1.0 - opacity) + to.blueF() * opacity,
        from.alphaF() * (1.0 - opacity) + to.alphaF() * opacity
    );
}

void QtETDrawUtils::drawRoundedRect(QPainter *painter, const QRect &rect, int radius,
                                     const QColor &fillColor, const QColor &borderColor,
                                     int borderWidth)
{
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    painter->fillPath(path, fillColor);
    painter->setPen(QPen(borderColor, borderWidth));
    painter->drawPath(path);
}

void QtETDrawUtils::drawRoundedFill(QPainter *painter, const QRect &rect, int radius,
                                     const QColor &fillColor)
{
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    painter->fillPath(path, fillColor);
}

void QtETDrawUtils::drawBottomIndicator(QPainter *painter, qreal x, qreal y,
                                         qreal width, qreal height, const QColor &color)
{
    QPainterPath path;
    path.addRoundedRect(QRectF(x, y, width, height), height / 2.0, height / 2.0);
    painter->fillPath(path, color);
}

void QtETDrawUtils::drawLabel(QPainter *painter, int x, int y,
                               const QString &text, const QColor &bgColor,
                               const QColor &textColor, int fontSize,
                               int hPadding, int vPadding, int radius)
{
    QFont oldFont = painter->font();
    QFont labelFont = oldFont;
    labelFont.setPointSize(fontSize);
    labelFont.setWeight(QFont::Bold);
    painter->setFont(labelFont);

    QFontMetrics fm(labelFont);
    int textWidth = fm.horizontalAdvance(text);
    int labelWidth = textWidth + 2 * hPadding;
    int labelHeight = fm.height() + 2 * vPadding;

    QPainterPath path;
    path.addRoundedRect(x, y, labelWidth, labelHeight, radius, radius);
    painter->fillPath(path, bgColor);

    painter->setPen(textColor);
    int textX = x + (labelWidth - textWidth) / 2;
    int textY = y + vPadding + fm.ascent();
    painter->drawText(textX, textY, text);

    painter->setFont(oldFont);
}

QSize QtETDrawUtils::labelSize(const QString &text, int fontSize,
                                int hPadding, int vPadding)
{
    QFont font;
    font.setPointSize(fontSize);
    font.setWeight(QFont::Bold);
    QFontMetrics fm(font);
    return QSize(fm.horizontalAdvance(text) + 2 * hPadding, fm.height() + 2 * vPadding);
}

void QtETDrawUtils::safeEndPainter(QPainter *painter)
{
    if (painter->isActive()) {
        painter->end();
    }
}
