#include <QTest>
#include <QWindow>

#include "core/application/runtime/ConfigRunState.h"
#include "core/service/DaemonClient.h"
#include "core/system_tray/SystemTrayManager.h"

class TestSystemTrayManager : public QObject {
    Q_OBJECT

private slots:
    void disconnectedDaemonUsesRedIconAndClearsRunningConfigs()
    {
        SystemTrayManager manager;
        manager.setDaemonConnectionState(DaemonClient::ConnectionState::Connected);
        manager.setConfigRunState(QStringLiteral("net-a"), ConfigRunState::Running);
        QCOMPARE(manager.runningConfigCount(), 1);
        QCOMPARE(manager.currentIconPath(), QStringLiteral(":/icons/qtet-green.png"));

        manager.setDaemonConnectionState(DaemonClient::ConnectionState::Disconnected);

        QCOMPARE(manager.runningConfigCount(), 0);
        QCOMPARE(manager.daemonStatusText(), QStringLiteral("未连接"));
        QCOMPARE(manager.daemonStatusMenuText(), QStringLiteral("🔴 后端状态：未连接"));
        QCOMPARE(manager.networkCountMenuText(), QStringLiteral("🟠 网络连接：0 个"));
        QCOMPARE(manager.currentIconPath(), QStringLiteral(":/icons/qtet-red.png"));
    }

    void connectedDaemonWithoutRunningConfigsUsesDefaultIcon()
    {
        SystemTrayManager manager;
        manager.setDaemonConnectionState(DaemonClient::ConnectionState::Connected);

        QCOMPARE(manager.runningConfigCount(), 0);
        QCOMPARE(manager.daemonStatusText(), QStringLiteral("已连接"));
        QCOMPARE(manager.daemonStatusMenuText(), QStringLiteral("🟢 后端状态：已连接"));
        QCOMPARE(manager.networkCountMenuText(), QStringLiteral("🟠 网络连接：0 个"));
        QCOMPARE(manager.currentIconPath(), QStringLiteral(":/icons/qtet.png"));
    }

    void onlyRunningStateIsCountedAsNetworkConnection()
    {
        SystemTrayManager manager;
        manager.setDaemonConnectionState(DaemonClient::ConnectionState::Connected);

        manager.setConfigRunState(QStringLiteral("starting"), ConfigRunState::Starting);
        manager.setConfigRunState(QStringLiteral("stopping"), ConfigRunState::Stopping);
        manager.setConfigRunState(QStringLiteral("running"), ConfigRunState::Running);

        QCOMPARE(manager.runningConfigCount(), 1);
        QCOMPARE(manager.networkCountMenuText(), QStringLiteral("🟢 网络连接：1 个"));
        QCOMPARE(manager.currentIconPath(), QStringLiteral(":/icons/qtet-green.png"));

        manager.setConfigRunState(QStringLiteral("running"), ConfigRunState::Stopped);
        QCOMPARE(manager.runningConfigCount(), 0);
        QCOMPARE(manager.networkCountMenuText(), QStringLiteral("🟠 网络连接：0 个"));
        QCOMPARE(manager.currentIconPath(), QStringLiteral(":/icons/qtet.png"));
    }

    void connectingDaemonUsesRedIconEvenWithRunningConfigs()
    {
        SystemTrayManager manager;
        manager.setDaemonConnectionState(DaemonClient::ConnectionState::Connected);
        manager.setConfigRunState(QStringLiteral("net-a"), ConfigRunState::Running);

        manager.setDaemonConnectionState(DaemonClient::ConnectionState::Connecting);

        QCOMPARE(manager.daemonStatusText(), QStringLiteral("连接中"));
        QCOMPARE(manager.daemonStatusMenuText(), QStringLiteral("🟠 后端状态：连接中"));
        QCOMPARE(manager.networkCountMenuText(), QStringLiteral("🟠 网络连接：0 个"));
        QCOMPARE(manager.currentIconPath(), QStringLiteral(":/icons/qtet-red.png"));
    }

    void destroyedMainWindowDoesNotCrashTrayManagerDestruction()
    {
        auto *manager = new SystemTrayManager;
        auto *window = new QWindow;
        manager->setMainWindow(window);

        delete window;
        delete manager;
    }

    void showAfterMainWindowDestroyedIsSafe()
    {
        SystemTrayManager manager;
        auto *window = new QWindow;
        manager.setMainWindow(window);

        delete window;
        manager.showMainWindowFromTray();
    }
};

QTEST_MAIN(TestSystemTrayManager)
#include "tst_system_tray_manager.moc"
