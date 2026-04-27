#include "qtettheme.h"

#include <QApplication>
#include <QStyleHints>
#include <QPalette>

QtETTheme::QtETTheme(QObject *parent)
    : QObject(parent)
{
}

QtETTheme *QtETTheme::instance()
{
    static QtETTheme theme;
    return &theme;
}

bool QtETTheme::isDarkMode() const
{
    return qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

QColor QtETTheme::backgroundColor() const
{
    return isDarkMode() ? QColor(35, 35, 35) : QApplication::palette().color(QPalette::Window);
}

QColor QtETTheme::widgetBackgroundColor() const
{
    return isDarkMode() ? QColor(45, 45, 45) : QColor(240, 240, 240);
}

QColor QtETTheme::selectedBackgroundColor() const
{
    return isDarkMode() ? QColor(70, 70, 70, 50) : QColor(102, 204, 255, 30);
}

QColor QtETTheme::normalBorderColor() const
{
    return isDarkMode() ? QColor(70, 70, 70) : QColor(180, 180, 180);
}

QColor QtETTheme::highlightBorderColor() const
{
    return AccentColor;
}

QColor QtETTheme::pressedBorderColor() const
{
    return AccentColorDark;
}

QColor QtETTheme::focusBorderColor() const
{
    return AccentColor;
}

QColor QtETTheme::textColor() const
{
    return isDarkMode() ? QColor(220, 220, 220) : QColor(30, 30, 30);
}

QColor QtETTheme::textInactiveColor() const
{
    return isDarkMode() ? QColor(140, 140, 140) : QColor(130, 130, 130);
}

QColor QtETTheme::selectedTextColor() const
{
    return isDarkMode() ? QColor(220, 220, 220) : QColor(0, 0, 0);
}

QColor QtETTheme::switchActiveColor() const
{
    return AccentColor;
}

QColor QtETTheme::switchInactiveColor() const
{
    return isDarkMode() ? QColor(80, 80, 80) : QColor(200, 200, 200);
}

QColor QtETTheme::switchSliderColor() const
{
    return Qt::white;
}

QColor QtETTheme::hoverBackgroundColor() const
{
    return isDarkMode() ? QColor(60, 60, 60, 120) : QColor(0, 0, 0, 25);
}

QColor QtETTheme::hoverFillColor(const QColor &base) const
{
    return QColor::fromRgbF(base.redF(), base.greenF(), base.blueF(), 0.15);
}
