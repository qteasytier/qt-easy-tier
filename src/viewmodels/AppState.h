/**
 * @file AppState.h
 * @brief 全局应用状态 ViewModel
 *
 * 管理当前页面导航、窗口状态和全局消息。
 * 通过 qmlRegisterSingletonType 注入 QML 上下文。
 * 所有权归属于 QQmlApplicationEngine（非 QGuiApplication），
 * 防止 app 析构时对 engine 子对象调用 delete 导致 double free。
 */
#pragma once
#include <QObject>
#include <QString>

/**
 * @class AppState
 * @brief 全局应用状态管理器
 *
 * 集中管理以下全局状态：
 *   - currentPage: 当前激活的页面标识（"network" / "settings"）
 *   - errorOccurred: 全局错误信号（QML 层统一展示错误信息）
 *
 * 该对象在 QML 中通过 "AppState" 全局 id 访问。
 */
class AppState : public QObject {
    Q_OBJECT

    /// 当前页面标识（"network" / "settings"），QML 中用于控制 PageContainer 的 currentPage
    Q_PROPERTY(QString currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged FINAL)

    /// 用户主目录路径（只读常量，用于 QML 文件选择器的起始目录）
    Q_PROPERTY(QString homeDirectory READ homeDirectory CONSTANT FINAL)

public:
    explicit AppState(QObject *parent = nullptr);

    QString currentPage() const;
    void setCurrentPage(const QString &page);

    /// 向 QML 层推送全局错误消息（供 QML 中的错误弹窗 / Toast 消费）
    Q_INVOKABLE void showError(const QString &message);

    QString homeDirectory() const;

    signals:
    /// 当前页面切换时发射，触发页面切换动画（PageContainer 的 Transition）
    void currentPageChanged();
    /// 全局错误发生，QML 可监听此信号展示错误提示
    void errorOccurred(const QString &message);

private:
    QString m_currentPage = QStringLiteral("network"); ///< 当前页面标识
};
