/**
 * @file AppState.h
 * @brief 全局应用状态 ViewModel
 *
 * 管理当前页面导航、主题模式、窗口状态和全局消息。
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
 *   - themeMode: 主题模式（System=0 / Light=1 / Dark=2）
 *   - isDarkMode / isLightMode: 派生的实际配色状态
 *   - errorOccurred: 全局错误信号（QML 层统一展示错误信息）
 *
 * 该对象在 QML 中通过 "AppState" 全局 id 访问。
 */
class AppState : public QObject {
    Q_OBJECT

    /// 当前页面标识（"network" / "settings"），QML 中用于控制 PageContainer 的 currentPage
    Q_PROPERTY(QString currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged FINAL)

    /// 主题模式：0=System（跟随系统）, 1=Light, 2=Dark
    Q_PROPERTY(int themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged FINAL)

    /// 当前是否为深色主题（由 themeMode 和系统配色综合计算得出）
    Q_PROPERTY(bool isDarkMode READ isDarkMode NOTIFY isDarkModeChanged FINAL)

    /// 当前是否为浅色主题（始终为 !isDarkMode）
    Q_PROPERTY(bool isLightMode READ isLightMode NOTIFY isLightModeChanged FINAL)

    /// 用户主目录路径（只读常量，用于 QML 文件选择器的起始目录）
    Q_PROPERTY(QString homeDirectory READ homeDirectory CONSTANT FINAL)

public:
    /// 主题模式枚举，与 QML 中的 int 值对应
    enum ThemeMode {
        System = 0, ///< 跟随系统配色方案
        Light,      ///< 强制浅色
        Dark        ///< 强制深色
    };
    Q_ENUM(ThemeMode)

    explicit AppState(QObject *parent = nullptr);

    QString currentPage() const;
    void setCurrentPage(const QString &page);

    int themeMode() const;
    void setThemeMode(int mode);

    bool isDarkMode() const;
    bool isLightMode() const;

    /// 向 QML 层推送全局错误消息（供 QML 中的错误弹窗 / Toast 消费）
    Q_INVOKABLE void showError(const QString &message);

    QString homeDirectory() const;

signals:
    /// 当前页面切换时发射，触发页面切换动画（PageContainer 的 Transition）
    void currentPageChanged();
    /// themeMode 属性值变更时发射
    void themeModeChanged();
    /// 实际深色/浅色状态切换时发射（由 updateColorScheme 内部计算触发）
    void isDarkModeChanged();
    /// 与 isDarkModeChanged 同时发射，通知浅色状态变更
    void isLightModeChanged();
    /// 全局错误发生，QML 可监听此信号展示错误提示
    void errorOccurred(const QString &message);

private:
    /**
     * @brief 根据当前 themeMode 和系统配色计算实际 isDarkMode
     *
     * 计算逻辑：
     *   - Light 模式: isDarkMode = false，并强制 styleHints 为 Light
     *   - Dark 模式:  isDarkMode = true， 并强制 styleHints 为 Dark
     *   - System 模式: 读取系统当前配色，不需要 force——
     *     但注意：从强制模式切回 System 时，需要 reset styleHints 为 Unknown
     */
    void updateColorScheme();

    QString m_currentPage = QStringLiteral("network"); ///< 当前页面标识
    int m_themeMode = System;                   ///< 主题模式，默认跟随系统
    bool m_isDarkMode = false;                  ///< 实际深色状态缓存
};
