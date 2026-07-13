/**
 * @file tst_public_server_provider.cpp
 * @brief 公共服务器提供者模块的单元测试。测试内容：加载有效公共服务器条目、非法 JSON 返回空列表。
 */
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "core/application/nodes/PublicServerProvider.h"

class TestPublicServerProvider : public QObject {
    Q_OBJECT

private slots:
    /// 测试目标：验证 loadPublicServers 仅返回 uri 非空的合法条目
    void loadPublicServers_returnsValidEntriesOnly()
    {
        // 准备测试数据：写入包含有效和无效条目的 JSON 文件
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString path = QDir(tempDir.path()).filePath(QStringLiteral("publicservers.json"));

        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("["
                   "{\"uri\":\"tcp://valid.example.com:11010\",\"contributor\":\"alice\",\"publicKey\":\"key-a\"},"
                   "{\"uri\":\"\",\"contributor\":\"ignored\",\"publicKey\":\"key-b\"},"
                   "{\"contributor\":\"missing-uri\",\"publicKey\":\"key-c\"}"
                   "]");
        file.close();

        PublicServerProvider provider(path);
        const QVariantList servers = provider.loadPublicServers();

        // 检查仅返回 1 条有效条目
        QCOMPARE(servers.size(), 1);
        const QVariantMap server = servers.first().toMap();
        // 检查返回的条目字段正确
        QCOMPARE(server.value(QStringLiteral("uri")).toString(), QStringLiteral("tcp://valid.example.com:11010"));
        QCOMPARE(server.value(QStringLiteral("contributor")).toString(), QStringLiteral("alice"));
        QCOMPARE(server.value(QStringLiteral("publicKey")).toString(), QStringLiteral("key-a"));
    }

    /// 测试目标：验证非法 JSON 文件时 loadPublicServers 返回空列表
    void loadPublicServers_returnsEmptyListForInvalidJson()
    {
        // 准备测试数据：写入非法 JSON 内容
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString path = QDir(tempDir.path()).filePath(QStringLiteral("publicservers.json"));

        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("not-json");
        file.close();

        PublicServerProvider provider(path);
        // 检查非法 JSON 返回空列表
        QVERIFY(provider.loadPublicServers().isEmpty());
    }
};

QTEST_MAIN(TestPublicServerProvider)
#include "tst_public_server_provider.moc"
