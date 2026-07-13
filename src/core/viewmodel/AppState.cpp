/**
 * @file AppState.cpp
 * @brief AppState 实现
 */
#include "AppState.h"
#include <QGuiApplication>
#include <QStyleHints>
#include <QStandardPaths>

AppState::AppState(QObject *parent)
    : QObject(parent)
{
    // 初始化时立即计算当前配色方案
    updateColorScheme();

    // 监听系统配色变化（用户切换系统的 Light/Dark 模式时触发）
    // 仅在 themeMode == System 时实际生效
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, &AppState::updateColorScheme);
}

QString AppState::currentPage() const { return m_currentPage; }

void AppState::setCurrentPage(const QString &page)
{
    // 值相同时跳过，避免不必要的信号发射和 QML 属性绑定重计算
    if (m_currentPage == page)
        return;
    m_currentPage = page;
    emit currentPageChanged();
}

int AppState::themeMode() const { return m_themeMode; }

void AppState::setThemeMode(int mode)
{
    // 值未变化则跳过
    if (m_themeMode == mode)
        return;
    m_themeMode = mode;

    // 立即更新实际配色状态，内部会判断 isDarkMode 是否真正变化
    updateColorScheme();

    // 通知 QML 属性绑定更新
    emit themeModeChanged();
}

bool AppState::isDarkMode() const { return m_isDarkMode; }
bool AppState::isLightMode() const { return !m_isDarkMode; }

void AppState::showError(const QString &message)
{
    // 错误消息的信号转发：C++ 层和 QML 层都可能调用此方法
    emit errorOccurred(message);
}

QString AppState::homeDirectory() const
{
    // 返回当前用户的主目录路径，用于 QML 中 FileDialog 等组件的默认打开位置
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
}

void AppState::updateColorScheme()
{
    bool dark;
    Qt::ColorScheme targetScheme;

    // 步骤 1：根据当前 themeMode 确定目标配色和强制方案
    switch (m_themeMode) {
    case Light:
        dark = false;
        targetScheme = Qt::ColorScheme::Light;
        break;
    case Dark:
        dark = true;
        targetScheme = Qt::ColorScheme::Dark;
        break;
    default: // System: 读取系统当前配色方案
        dark = (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark);
        targetScheme = Qt::ColorScheme::Unknown;
        break;
    }

    // 步骤 2：强制或重置应用级配色方案
    // QML 基础控件（Switch、RadioButton 等）优先读取 styleHints 的 ColorScheme
    // 因此需要显式设置才能让控件跟随自定义模式
    if (m_themeMode != System) {
        // Light/Dark 模式：强制设置对应配色方案
        QGuiApplication::styleHints()->setColorScheme(targetScheme);
    } else {
        // System 模式：重置为 Unknown，让控件跟随系统
        QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Unknown);
    }

    // 步骤 3：仅在实际状态变化时发射信号，避免无效的 QML 属性绑定更新
    if (m_isDarkMode == dark)
        return;
    m_isDarkMode = dark;
    emit isDarkModeChanged();
    emit isLightModeChanged();
}
