/**
 * @file AppState.cpp
 * @brief AppState 实现
 */
#include "AppState.h"
#include <QStandardPaths>

AppState::AppState(QObject *parent)
    : QObject(parent)
{
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
