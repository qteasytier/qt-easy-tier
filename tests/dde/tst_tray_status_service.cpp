/** @file tst_tray_status_service.cpp @brief 托盘状态服务测试 */
#include "dde_tray_plugin/TrayStatusService.h"
#include "dde_tray_plugin/TrayStatusTypes.h"
#include "sqlite_repository/DatabaseConnection.h"
#include "sqlite_repository/NetworkConfigRepository.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

class TrayStatusServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void countsOnlineNodesIncludingLocalNode();
    void ignoresEventsAndMalformedOtherInstance();
    void resolvesDatabasePathMatchingMainApp();
    void showsNoInstancesWhenDaemonNotConnected();
};

void TrayStatusServiceTest::countsOnlineNodesIncludingLocalNode()
{
    const QJsonObject network{
        {QStringLiteral("my_node_info"), QJsonObject{{QStringLiteral("peer_id"), 1}}},
        {QStringLiteral("routes"), QJsonArray{
            QJsonObject{{QStringLiteral("peer_id"), 1}},
            QJsonObject{{QStringLiteral("peer_id"), 2}},
            QJsonObject{{QStringLiteral("peer_id"), 3}}
        }},
        {QStringLiteral("events"), QJsonArray{QStringLiteral("not parsed")}}
    };
    const QString encoded = QString::fromLatin1(
        QJsonDocument(network).toJson(QJsonDocument::Compact).toBase64());
    const QJsonObject result{{QStringLiteral("instances"), QJsonArray{
        QJsonObject{{QStringLiteral("key"), QStringLiteral("local")},
                    {QStringLiteral("value"), encoded}}
    }}};

    const auto counts = TrayStatusService::parseNodeCounts(result);
    QCOMPARE(counts.value(QStringLiteral("local")), 3);
}

void TrayStatusServiceTest::ignoresEventsAndMalformedOtherInstance()
{
    const QJsonObject network{
        {QStringLiteral("my_node_info"), QJsonObject{{QStringLiteral("peer_id"), 9}}},
        {QStringLiteral("routes"), QJsonArray{QJsonObject{{QStringLiteral("peer_id"), 9}}}},
        {QStringLiteral("events"), QJsonArray{QJsonObject{{QStringLiteral("invalid"), true}}}}
    };
    const QString encoded = QString::fromLatin1(
        QJsonDocument(network).toJson(QJsonDocument::Compact).toBase64());
    const QJsonObject result{{QStringLiteral("instances"), QJsonArray{
        QJsonObject{{QStringLiteral("key"), QStringLiteral("valid")},
                    {QStringLiteral("value"), encoded}},
        QJsonObject{{QStringLiteral("key"), QStringLiteral("broken")},
                    {QStringLiteral("value"), QStringLiteral("broken")}}
    }}};

    const auto counts = TrayStatusService::parseNodeCounts(result);
    QCOMPARE(counts.value(QStringLiteral("valid")), 1);
    QVERIFY(!counts.contains(QStringLiteral("broken")));
}

void TrayStatusServiceTest::resolvesDatabasePathMatchingMainApp()
{
    // 显式路径原样返回
    QCOMPARE(TrayStatusService::resolveDatabasePath(QStringLiteral("/tmp/x.db")),
             QStringLiteral("/tmp/x.db"));

    // 空路径时补齐应用标识，指向与主应用一致的默认数据库
    const QString path = TrayStatusService::resolveDatabasePath(QString());
    QVERIFY(path.endsWith(QStringLiteral("/qteasytier/QtEasyTier/qteasytier.db")));
    QCOMPARE(QCoreApplication::organizationName(), QStringLiteral("qteasytier"));
    QCOMPARE(QCoreApplication::applicationName(), QStringLiteral("QtEasyTier"));
}

void TrayStatusServiceTest::showsNoInstancesWhenDaemonNotConnected()
{
    // daemon 未连接（无心跳）时，即使数据库中有本地配置也不显示任何实例，
    // 避免用户误点击
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dbPath = tempDir.filePath(QStringLiteral("qteasytier.db"));
    DatabaseConnection db(dbPath);
    QVERIFY(db.open());
    NetworkConfigRepository repo(db.database());
    NetworkConf config;
    config.setInstanceName(QStringLiteral("local-conf"));
    config.displayName = QStringLiteral("本地配置");
    QVERIFY(repo.save(config));

    TrayStatusService service(dbPath);
    QVERIFY(service.snapshot().instances.isEmpty());
}

QTEST_MAIN(TrayStatusServiceTest)
#include "tst_tray_status_service.moc"
