/**
 * @file tst_favorite_node_import_export_service.cpp
 * @brief 收藏节点批量导入导出服务单元测试。
 */
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include <memory>

#include "core/application/favorite/FavoriteNodeImportExportService.h"
#include "core/repository/DatabaseConnection.h"
#include "core/repository/FavoriteNodeRepository.h"

class TestFavoriteNodeImportExportService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_tempDir.isValid());
        m_db = std::make_unique<DatabaseConnection>(QDir(m_tempDir.path()).filePath(QStringLiteral("favorite_service.db")));
        QVERIFY(m_db->open());
        m_repository = new FavoriteNodeRepository(m_db->database(), this);
        m_service = new FavoriteNodeImportExportService(m_repository, this);
    }

    void init()
    {
        QVERIFY(m_repository->clear());
        m_service->setImportUrlTimeoutMs(1000);
    }

    /// 测试目标：文件导入会跳过重复 URI，并将缺失 display_name 的节点命名为 UNKNOW
    void importFromFile_importsValidNodesAndSkipsDuplicates()
    {
        QVERIFY(m_repository->add(QStringLiteral("已有"), QStringLiteral("tcp://dup.example.com:11010")).has_value());

        const QString path = QDir(m_tempDir.path()).filePath(QStringLiteral("nodes-import.json"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("["
                   "{\"uri\":\"tcp://new.example.com:11010\",\"display_name\":\"新节点\",\"publicKey\":\"key-a\"},"
                   "{\"uri\":\"tcp://unnamed.example.com:11010\"},"
                   "{\"uri\":\"tcp://dup.example.com:11010\",\"display_name\":\"重复\"},"
                   "{\"uri\":\"\",\"display_name\":\"空地址\"}"
                   "]");
        file.close();

        QSignalSpy completedSpy(m_service, &FavoriteNodeImportExportService::importCompleted);

        QVERIFY(m_service->importFromFile(QUrl::fromLocalFile(path)));

        QCOMPARE(completedSpy.count(), 1);
        QCOMPARE(completedSpy.first().at(0).toInt(), 2);
        QCOMPARE(completedSpy.first().at(1).toInt(), 2);
        QCOMPARE(m_repository->loadAll().size(), 3);

        bool foundDefaultName = false;
        for (const FavoriteNode &node : m_repository->loadAll()) {
            if (node.uri == QStringLiteral("tcp://unnamed.example.com:11010")) {
                foundDefaultName = true;
                QCOMPARE(node.name, QStringLiteral("UNKNOW"));
            }
        }
        QVERIFY(foundDefaultName);
    }

    /// 测试目标：批量导出使用 uri / display_name / publicKey 字段格式
    void exportToFile_writesPublicServerCompatibleJson()
    {
        FavoriteNode node;
        node.name = QStringLiteral("导出节点");
        node.uri = QStringLiteral("tcp://export.example.com:11010");
        node.publicKey = QStringLiteral("key-export");
        const QString path = QDir(m_tempDir.path()).filePath(QStringLiteral("nodes-export.json"));
        QSignalSpy completedSpy(m_service, &FavoriteNodeImportExportService::exportCompleted);

        QVERIFY(m_service->exportToFile(QUrl::fromLocalFile(path), {node}));

        QCOMPARE(completedSpy.count(), 1);
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QVERIFY(doc.isArray());
        QCOMPARE(doc.array().size(), 1);
        const QJsonObject obj = doc.array().first().toObject();
        QCOMPARE(obj.value(QStringLiteral("uri")).toString(), QStringLiteral("tcp://export.example.com:11010"));
        QCOMPARE(obj.value(QStringLiteral("display_name")).toString(), QStringLiteral("导出节点"));
        QCOMPARE(obj.value(QStringLiteral("publicKey")).toString(), QStringLiteral("key-export"));
        QVERIFY(!obj.contains(QStringLiteral("contributor")));
    }

    /// 测试目标：URL 导入拒绝非 http/https 协议
    void importFromUrl_rejectsUnsupportedScheme()
    {
        QSignalSpy errorSpy(m_service, &FavoriteNodeImportExportService::operationFailed);

        m_service->importFromUrl(QUrl(QStringLiteral("ftp://example.com/nodes.json")));

        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().first().toString(), QStringLiteral("仅支持 http/https 节点导入地址"));
    }

    /// 测试目标：URL 返回合法 JSON 时导入成功，并沿用重复跳过和 UNKNOW 默认名称规则
    void importFromUrl_importsValidJsonResponse()
    {
        QVERIFY(m_repository->add(QStringLiteral("已有"), QStringLiteral("tcp://dup-url.example.com:11010")).has_value());
        QTcpServer server;
        const QUrl url = serveHttpOnce(&server,
            "["
            "{\"uri\":\"tcp://url.example.com:11010\",\"display_name\":\"URL节点\",\"publicKey\":\"key-url\"},"
            "{\"uri\":\"tcp://url-unnamed.example.com:11010\"},"
            "{\"uri\":\"tcp://dup-url.example.com:11010\",\"display_name\":\"重复\"}"
            "]");
        QVERIFY(url.isValid());
        QSignalSpy completedSpy(m_service, &FavoriteNodeImportExportService::importCompleted);

        m_service->importFromUrl(url);

        QVERIFY(completedSpy.wait(1000));
        QCOMPARE(completedSpy.first().at(0).toInt(), 2);
        QCOMPARE(completedSpy.first().at(1).toInt(), 1);
        QCOMPARE(m_repository->loadAll().size(), 3);
    }

    /// 测试目标：URL 返回非法 JSON 时触发错误
    void importFromUrl_reportsInvalidJsonResponse()
    {
        QTcpServer server;
        const QUrl url = serveHttpOnce(&server, "not-json");
        QVERIFY(url.isValid());
        QSignalSpy errorSpy(m_service, &FavoriteNodeImportExportService::operationFailed);

        m_service->importFromUrl(url);

        QVERIFY(errorSpy.wait(1000));
        QVERIFY(errorSpy.first().first().toString().contains(QStringLiteral("JSON 格式错误")));
    }

    /// 测试目标：URL 返回非 2xx HTTP 状态码时触发错误
    void importFromUrl_reportsHttpErrorStatus()
    {
        QTcpServer server;
        const QUrl url = serveHttpOnce(&server, "[]", 404, "Not Found");
        QVERIFY(url.isValid());
        QSignalSpy errorSpy(m_service, &FavoriteNodeImportExportService::operationFailed);

        m_service->importFromUrl(url);

        QVERIFY(errorSpy.wait(1000));
        QVERIFY(errorSpy.first().first().toString().contains(QStringLiteral("HTTP 状态码 404")));
    }

    /// 测试目标：URL 请求超过显式超时时间时触发超时错误
    void importFromUrl_reportsTimeout()
    {
        QTcpServer server;
        const QUrl url = serveHangingHttpOnce(&server);
        QVERIFY(url.isValid());
        m_service->setImportUrlTimeoutMs(50);
        QSignalSpy errorSpy(m_service, &FavoriteNodeImportExportService::operationFailed);

        m_service->importFromUrl(url);

        QVERIFY(errorSpy.wait(1000));
        QCOMPARE(errorSpy.first().first().toString(), QStringLiteral("节点导入失败：请求超时"));
    }

private:
    QUrl serveHttpOnce(QTcpServer *server, const QByteArray &body, int statusCode = 200,
                       const QByteArray &reason = "OK")
    {
        if (!server->listen(QHostAddress::LocalHost, 0))
            return {};
        connect(server, &QTcpServer::newConnection, server, [server, body, statusCode, reason]() {
            QTcpSocket *socket = server->nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket, [socket, body, statusCode, reason]() {
                socket->readAll();
                const QByteArray response = QByteArray("HTTP/1.1 ") + QByteArray::number(statusCode) + " " + reason + "\r\n"
                                            + "Content-Type: application/json\r\n"
                                            + "Content-Length: " + QByteArray::number(body.size()) + "\r\n"
                                            + "Connection: close\r\n\r\n"
                                            + body;
                socket->write(response);
                socket->disconnectFromHost();
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
        });
        return QUrl(QStringLiteral("http://127.0.0.1:%1/nodes.json").arg(server->serverPort()));
    }

    QUrl serveHangingHttpOnce(QTcpServer *server)
    {
        if (!server->listen(QHostAddress::LocalHost, 0))
            return {};
        connect(server, &QTcpServer::newConnection, server, [server]() {
            QTcpSocket *socket = server->nextPendingConnection();
            socket->setParent(server);
            connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
                socket->readAll();
            });
        });
        return QUrl(QStringLiteral("http://127.0.0.1:%1/nodes.json").arg(server->serverPort()));
    }

    QTemporaryDir m_tempDir;
    std::unique_ptr<DatabaseConnection> m_db;
    FavoriteNodeRepository *m_repository = nullptr;
    FavoriteNodeImportExportService *m_service = nullptr;
};

QTEST_MAIN(TestFavoriteNodeImportExportService)
#include "tst_favorite_node_import_export_service.moc"
