/**
 * @file tst_favorite_node_viewmodel.cpp
 * @brief 收藏节点 ViewModel 单元测试。
 */
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include <memory>

#include "core/application/favorite/FavoriteNodeImportExportService.h"
#include "core/repository/DatabaseConnection.h"
#include "core/repository/FavoriteNodeRepository.h"
#include "core/viewmodel/FavoriteNodeViewModel.h"

class TestFavoriteNodeViewModel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_tempDir.isValid());
        m_db = std::make_unique<DatabaseConnection>(QDir(m_tempDir.path()).filePath(QStringLiteral("favorite_nodes.db")));
        QVERIFY(m_db->open());
        m_repository = new FavoriteNodeRepository(m_db->database(), this);
        m_service = new FavoriteNodeImportExportService(m_repository, this);
        m_model = new FavoriteNodeViewModel(m_repository, m_service, this);
    }

    void init()
    {
        QVERIFY(m_model->clearAll());
    }

    /// 测试目标：service 导入完成后 ViewModel 刷新模型并转发导入完成信号
    void serviceImportCompleted_refreshesModelAndForwardsSignal()
    {
        const QString path = QDir(m_tempDir.path()).filePath(QStringLiteral("nodes-viewmodel-import.json"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("["
                   "{\"uri\":\"tcp://new.example.com:11010\",\"display_name\":\"新节点\",\"publicKey\":\"key-a\"}"
                   "]");
        file.close();

        QSignalSpy completedSpy(m_model, &FavoriteNodeViewModel::importCompleted);

        QVERIFY(m_model->importNodesFromFile(QUrl::fromLocalFile(path).toString()));

        QCOMPARE(completedSpy.count(), 1);
        QCOMPARE(completedSpy.first().at(0).toInt(), 1);
        QCOMPARE(completedSpy.first().at(1).toInt(), 0);
        QCOMPARE(m_model->count(), 1);
        QCOMPARE(m_model->data(m_model->index(0, 0), FavoriteNodeViewModel::NameRole).toString(), QStringLiteral("新节点"));
    }

    /// 测试目标：service 错误会由 ViewModel 转发为 errorOccurred
    void serviceOperationFailed_forwardsError()
    {
        QSignalSpy errorSpy(m_model, &FavoriteNodeViewModel::errorOccurred);

        QVERIFY(!m_model->importNodesFromFile(QUrl::fromLocalFile(QDir(m_tempDir.path()).filePath(QStringLiteral("missing.json"))).toString()));

        QCOMPARE(errorSpy.count(), 1);
        QVERIFY(errorSpy.first().first().toString().contains(QStringLiteral("无法打开节点文件")));
    }

    /// 测试目标：导出完成信号由 service 转发到 ViewModel
    void exportNodesToFile_forwardsExportCompleted()
    {
        QVERIFY(m_model->addNode(QStringLiteral("导出节点"), QStringLiteral("tcp://export.example.com:11010"), QStringLiteral("key-export")));
        const QString path = QDir(m_tempDir.path()).filePath(QStringLiteral("nodes-export.json"));
        QSignalSpy completedSpy(m_model, &FavoriteNodeViewModel::exportCompleted);

        QVERIFY(m_model->exportNodesToFile(QUrl::fromLocalFile(path).toString()));

        QCOMPARE(completedSpy.count(), 1);
        QVERIFY(QFile::exists(path));
    }

    /// 测试目标：单条新增、删除仍由 ViewModel 刷新模型
    void addAndRemoveNode_updatesModel()
    {
        QVERIFY(m_model->addNode(QStringLiteral("节点"), QStringLiteral("tcp://node.example.com:11010"), QStringLiteral("key")));
        QCOMPARE(m_model->count(), 1);
        const qint64 id = m_model->data(m_model->index(0, 0), FavoriteNodeViewModel::IdRole).toLongLong();
        QVERIFY(m_model->removeNode(id));
        QCOMPARE(m_model->count(), 0);
    }

private:
    QTemporaryDir m_tempDir;
    std::unique_ptr<DatabaseConnection> m_db;
    FavoriteNodeRepository *m_repository = nullptr;
    FavoriteNodeImportExportService *m_service = nullptr;
    FavoriteNodeViewModel *m_model = nullptr;
};

QTEST_MAIN(TestFavoriteNodeViewModel)
#include "tst_favorite_node_viewmodel.moc"
