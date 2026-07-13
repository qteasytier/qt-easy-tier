/**
 * @file FontHelper.cpp
 * @brief FontHelper 实现
 */
#include "FontHelper.h"

#include <QFontDatabase>

/**
 * @brief 构造函数
 * @param parent 父对象指针
 *
 */
FontHelper::FontHelper(QObject *parent)
    : QObject(parent)
{
}

/**
 * @brief 返回系统最小可读字体
 *
 * 使用 QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont)
 * 获取桌面环境的"最小可读字体"设置。
 * 这确保应用在不同桌面环境（KDE、GNOME 等）和不同显示器
 * 分辨率下都能使用系统预设的合适字号。
 */
QFont FontHelper::smallFont() const
{
    return QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont);
}
