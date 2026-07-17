/**
 * @file main.cpp
 * @brief 应用程序入口
 *
 * 按顺序完成初始化：
 * 1. 创建 QApplication，设置组织名和应用名
 * 2. 打开 SQLite 数据库并建表/迁移
 * 3. 创建 QQmlApplicationEngine
 * 4. 装配所有服务对象（AppServices）并以引擎为父级
 * 5. 将所有服务对象注册为 QML 单例
 * 6. 加载 QML 主界面
 * 7. 进入事件循环
 */
#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QUrl>
#include <QWindow>

#include "app/AppLaunchManager.h"
#include "app/AppServices.h"
#include "app/QmlSingletonRegistrar.h"
#include "core/repository/DatabaseConnection.h"
#include "core/system_tray/SystemTrayManager.h"
#include "core/util/LogHelper.h"

int main(int argc, char *argv[])
{

// Windows下使用稍微没那么难看的 Universal 风格
#ifdef Q_OS_WIN
    QQuickStyle::setStyle(QStringLiteral("Universal"));
#endif

    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("qteasytier"));
    app.setApplicationName(QStringLiteral("QtEasyTier"));
    app.setWindowIcon(QIcon(":/icons/qtet.png"));

    AppLaunchManager launchManager;
    if (launchManager.tryConnectToExistingInstance())
        return 0;

    if (!launchManager.listenForSingleInstance()) {
        LogHelper::logError(QStringLiteral("单实例监听启动失败"));
        return 1;
    }

    const bool autoStartLaunch = AppLaunchManager::isAutoStartLaunch(QApplication::arguments());

    // 打开默认路径的 SQLite 数据库（自动建表/迁移）
    DatabaseConnection db(DatabaseConnection::defaultDatabasePath());
    if (!db.open()) {
        LogHelper::logError(QStringLiteral("数据库打开失败"));
        return 1;
    }

    // 创建 QML 引擎，作为所有服务对象的父级（管理生命周期）
    QQmlApplicationEngine engine;
    AppServices services(db.database(), &engine);
    // 将所有 C++ 服务对象注册为 QML 单例，供 QML 界面绑定
    registerQmlSingletons(engine, services);

    // 从内嵌 QRC 加载主 QML 界面，避免依赖程序目录中的本地 QML 文件。
    engine.load(QUrl(QStringLiteral("qrc:/QtEasyTier/Main.qml")));

    if (engine.rootObjects().isEmpty()) {
        LogHelper::logError(QStringLiteral("QML 主界面加载失败"));
        return 1;
    }

    auto *mainWindow = qobject_cast<QWindow *>(engine.rootObjects().first());
    if (!mainWindow) {
        LogHelper::logError(QStringLiteral("QML 根对象不是窗口，无法绑定系统托盘"));
        return 1;
    }

    services.systemTrayManager()->setMainWindow(mainWindow);
    QObject::connect(&launchManager, &AppLaunchManager::activationRequested,
                     services.systemTrayManager(), &SystemTrayManager::showMainWindowFromTray);
    // 只有系统托盘可用时才保持后台运行；否则沿用 Qt 默认的关闭即退出行为。
    const bool trayAvailable = services.systemTrayManager()->isAvailable();
    QApplication::setQuitOnLastWindowClosed(!trayAvailable);
    // 普通启动显示主窗口；自启动且托盘可用时保持隐藏，避免登录后窗口闪现。
    if (!autoStartLaunch || !trayAvailable)
        services.systemTrayManager()->showMainWindowFromTray();
    return QApplication::exec();
}
