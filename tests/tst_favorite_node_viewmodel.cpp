/**
 * @file tst_favorite_node_viewmodel.cpp
 * @brief 收藏节点 ViewModel 批量导入导出单元测试。
 */
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

#include "core/repository/DatabaseConnection.h"
#include "core/viewmodel/FavoriteNodeViewModel.h"

class TestFavoriteNodeViewModel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_tempDir.isValid());
        m_db = std::make_unique<DatabaseConnection>(QDir(m_tempDir.path()).filePath(QStringLiteral("favorite_nodes.db")));
        QVERIFY(m_db->open());
        m_model = new FavoriteNodeViewModel(m_db->database(), this);
    }

    void init()
    {
        QVERIFY(m_model->clearAll());
    }

    /// 测试目标：批量导入会跳过重复 URI，并将缺失 display_name 的节点命名为 UNKNOW
    void importNodesFromFile_importsValidNodesAndSkipsDuplicates()
    {
        QVERIFY(m_model->addNode(QStringLiteral("已有"), QStringLiteral("tcp://dup.example.com:11010"), QString()));

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

        QSignalSpy completedSpy(m_model, &FavoriteNodeViewModel::importCompleted);

        QVERIFY(m_model->importNodesFromFile(QUrl::fromLocalFile(path).toString()));

        QCOMPARE(completedSpy.count(), 1);
        QCOMPARE(completedSpy.first().at(0).toInt(), 2);
        QCOMPARE(completedSpy.first().at(1).toInt(), 2);
        QCOMPARE(m_model->count(), 3);

        bool foundDefaultName = false;
        for (int row = 0; row < m_model->rowCount(); ++row) {
            const QModelIndex idx = m_model->index(row, 0);
            if (m_model->data(idx, FavoriteNodeViewModel::UriRole).toString() == QStringLiteral("tcp://unnamed.example.com:11010")) {
                foundDefaultName = true;
                QCOMPARE(m_model->data(idx, FavoriteNodeViewModel::NameRole).toString(), QStringLiteral("UNKNOW"));
            }
        }
        QVERIFY(foundDefaultName);
    }

    /// 测试目标：批量导出使用 uri / display_name / publicKey 字段格式
    void exportNodesToFile_writesPublicServerCompatibleJson()
    {
        QVERIFY(m_model->addNode(QStringLiteral("导出节点"), QStringLiteral("tcp://export.example.com:11010"), QStringLiteral("key-export")));

        const QString path = QDir(m_tempDir.path()).filePath(QStringLiteral("nodes-export.json"));
        QSignalSpy completedSpy(m_model, &FavoriteNodeViewModel::exportCompleted);

        QVERIFY(m_model->exportNodesToFile(QUrl::fromLocalFile(path).toString()));

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

private:
    QTemporaryDir m_tempDir;
    std::unique_ptr<DatabaseConnection> m_db;
    FavoriteNodeViewModel *m_model = nullptr;
};

QTEST_MAIN(TestFavoriteNodeViewModel)
#include "tst_favorite_node_viewmodel.moc"
