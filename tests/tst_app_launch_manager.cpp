#include <QTest>

#include "app/AppLaunchManager.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QSignalSpy>
#include <QUuid>

class TestAppLaunchManager : public QObject {
    Q_OBJECT

private slots:
    void detectsAutoStartArgument()
    {
        QVERIFY(AppLaunchManager::isAutoStartLaunch({QStringLiteral("QtEasyTier"), QStringLiteral("--autostart")}));
    }

    void returnsFalseWhenAutoStartArgumentIsMissing()
    {
        QVERIFY(!AppLaunchManager::isAutoStartLaunch({QStringLiteral("QtEasyTier")}));
    }

    void doesNotMatchSimilarArguments()
    {
        QVERIFY(!AppLaunchManager::isAutoStartLaunch({QStringLiteral("QtEasyTier"), QStringLiteral("--autostarted")}));
        QVERIFY(!AppLaunchManager::isAutoStartLaunch({QStringLiteral("QtEasyTier"), QStringLiteral("autostart")}));
    }

    void singleInstanceServerNameIsStable()
    {
        QCOMPARE(AppLaunchManager::singleInstanceServerName(), QStringLiteral("cn.qtet.qteasytier.sock"));
    }

    void returnsFalseWhenNoExistingInstanceIsListening()
    {
        AppLaunchManager manager;
        QVERIFY(!manager.tryConnectToExistingInstance(uniqueServerName(), 50));
    }

    void connectsToExistingInstance()
    {
        const QString serverName = uniqueServerName();
        QLocalServer server;
        QVERIFY(server.listen(serverName));

        AppLaunchManager manager;
        QVERIFY(manager.tryConnectToExistingInstance(serverName, 500));

        server.close();
        QLocalServer::removeServer(serverName);
    }

    void listenForSingleInstanceAcceptsActivationRequests()
    {
        const QString serverName = uniqueServerName();
        AppLaunchManager manager;
        QSignalSpy spy(&manager, &AppLaunchManager::activationRequested);

        QVERIFY(manager.listenForSingleInstance(serverName));
        QVERIFY(manager.isSingleInstanceListening());

        QLocalSocket socket;
        socket.connectToServer(serverName);
        QVERIFY(socket.waitForConnected(500));
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 500);

        QLocalServer::removeServer(serverName);
    }

private:
    static QString uniqueServerName()
    {
        return QStringLiteral("cn.qtet.qteasytier-test-%1.sock")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
};

QTEST_MAIN(TestAppLaunchManager)
#include "tst_app_launch_manager.moc"
