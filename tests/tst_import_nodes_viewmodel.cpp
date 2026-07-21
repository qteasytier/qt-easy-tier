/**
 * @file tst_import_nodes_viewmodel.cpp
 * @brief 导入节点 ViewModel 模块的单元测试。测试内容：收藏节点与公共节点合并加载、选中节点返回 URI 和公钥。
 */
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

#include "core/application/favorite/FavoriteNodeImportExportService.h"
#include "core/repository/DatabaseConnection.h"
#include "core/repository/FavoriteNodeRepository.h"
#include "core/viewmodel/FavoriteNodeViewModel.h"
#include "core/viewmodel/nodes/ImportNodesViewModel.h"

class TestImportNodesViewModel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_tempDir.isValid());
        m_db = std::make_unique<DatabaseConnection>(QDir(m_tempDir.path()).filePath(QStringLiteral("nodes.db")));
        QVERIFY(m_db->open());
        m_repository = new FavoriteNodeRepository(m_db->database(), this);
        m_service = new FavoriteNodeImportExportService(m_repository, this);
        m_favorites = new FavoriteNodeViewModel(m_repository, m_service, this);
    }

    void init()
    {
        QVERIFY(m_favorites);
        QVERIFY(m_favorites->clearAll());
    }

    /// 测试目标：验证 reload 将收藏节点和公共节点合并展示，且 roleNames 正确
    void reload_combinesFavoriteAndPublicNodes()
    {
        // 准备测试数据：添加一个收藏节点
        QVERIFY(m_favorites->addNode(QStringLiteral("收藏A"), QStringLiteral("tcp://favorite.example.com:11010"),
                                     QStringLiteral("favorite-key")));

        // 准备公共节点文件
        const QString publicPath = QDir(m_tempDir.path()).filePath(QStringLiteral("publicservers.json"));
        QFile file(publicPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("["
                   "{\"uri\":\"tcp://public.example.com:11010\",\"display_name\":\"bob\",\"publicKey\":\"public-key\"}"
                   "]");
        file.close();

        ImportNodesViewModel model(m_favorites, QUrl::fromLocalFile(publicPath));
        QSignalSpy countSpy(&model, &ImportNodesViewModel::countChanged);

        model.reload();

        // 检查合并后共 2 条节点
        QCOMPARE(model.count(), 2);
        QCOMPARE(countSpy.count(), 1);

        // 检查 roleNames 正确
        const QHash<int, QByteArray> roles = model.roleNames();
        QCOMPARE(roles.value(ImportNodesViewModel::NameRole), QByteArray("name"));
        QCOMPARE(roles.value(ImportNodesViewModel::UriRole), QByteArray("uri"));
        QCOMPARE(roles.value(ImportNodesViewModel::PublicKeyRole), QByteArray("publicKey"));
        QCOMPARE(roles.value(ImportNodesViewModel::CheckedRole), QByteArray("checked"));
        QCOMPARE(roles.value(ImportNodesViewModel::SectionRole), QByteArray("section"));

        // 检查收藏节点数据正确
        QCOMPARE(model.data(model.index(0, 0), ImportNodesViewModel::NameRole).toString(), QStringLiteral("收藏A"));
        QCOMPARE(model.data(model.index(0, 0), ImportNodesViewModel::UriRole).toString(), QStringLiteral("tcp://favorite.example.com:11010"));
        QCOMPARE(model.data(model.index(0, 0), ImportNodesViewModel::PublicKeyRole).toString(), QStringLiteral("favorite-key"));
        QCOMPARE(model.data(model.index(0, 0), ImportNodesViewModel::SectionRole).toString(), QStringLiteral("收藏节点"));

        // 检查公共节点数据正确
        QCOMPARE(model.data(model.index(1, 0), ImportNodesViewModel::NameRole).toString(), QStringLiteral("【公共节点】由bob提供"));
        QCOMPARE(model.data(model.index(1, 0), ImportNodesViewModel::UriRole).toString(), QStringLiteral("tcp://public.example.com:11010"));
        QCOMPARE(model.data(model.index(1, 0), ImportNodesViewModel::PublicKeyRole).toString(), QStringLiteral("public-key"));
        QCOMPARE(model.data(model.index(1, 0), ImportNodesViewModel::SectionRole).toString(), QStringLiteral("公共节点"));
    }

    /// 测试目标：验证 selectedNodes 返回选中节点的 URI 和公钥
    void selectedNodes_returnsCheckedUriAndPublicKey()
    {
        // 准备测试数据：添加收藏节点
        QVERIFY(m_favorites->addNode(QStringLiteral("收藏A"), QStringLiteral("tcp://favorite.example.com:11010"),
                                     QStringLiteral("favorite-key")));

        ImportNodesViewModel model(m_favorites, QUrl::fromLocalFile(QDir(m_tempDir.path()).filePath(QStringLiteral("missing.json"))));
        model.reload();

        // 初始无选中节点
        QCOMPARE(model.selectedNodes().size(), 0);
        model.setChecked(0, true);

        const QVariantList selected = model.selectedNodes();
        // 检查选中 1 条节点
        QCOMPARE(selected.size(), 1);
        const QVariantMap node = selected.first().toMap();
        // 检查返回的 URI 和公钥正确
        QCOMPARE(node.value(QStringLiteral("uri")).toString(), QStringLiteral("tcp://favorite.example.com:11010"));
        QCOMPARE(node.value(QStringLiteral("publicKey")).toString(), QStringLiteral("favorite-key"));
    }

private:
    QTemporaryDir m_tempDir;
    std::unique_ptr<DatabaseConnection> m_db;
    FavoriteNodeRepository *m_repository = nullptr;
    FavoriteNodeImportExportService *m_service = nullptr;
    FavoriteNodeViewModel *m_favorites = nullptr;
};

QTEST_MAIN(TestImportNodesViewModel)
#include "tst_import_nodes_viewmodel.moc"
