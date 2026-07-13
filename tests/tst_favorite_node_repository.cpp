/**
 * @file tst_favorite_node_repository.cpp
 * @brief 收藏节点仓库模块的单元测试。测试内容：新增节点与批量加载、更新节点（含公钥）、删除节点、清空全部、重复 URI 防重检查、新增节点时附加公钥。
 *
 * 覆盖 FavoriteNodeRepository 的所有 CRUD 操作：
 * - 新增节点 / 批量加载
 * - 更新节点（含公钥字段）
 * - 删除节点
 * - 清空全部
 * - 重复 URI 防重检查（含带 excludeId 的精确去重）
 * - 新增节点时附加公钥
 *
 * 每个测试使用独立随机命名临时数据库，确保测试完全隔离。
 */
#include <QTest>
#include <QDir>
#include <QUuid>
#include "core/repository/DatabaseConnection.h"
#include "core/repository/FavoriteNodeRepository.h"

class TestFavoriteNodeRepository : public QObject {
    Q_OBJECT

private slots:
    /// 测试目标: 验证新增节点后 loadAll() 返回正确的节点列表
    void addAndLoadNodes();
    /// 测试目标: 验证 update() 可更新节点名称、URI、公钥
    void updateNode();
    /// 测试目标: 验证 remove() 删除单个节点后 loadAll() 为空
    void removeNode();
    /// 测试目标: 验证 clear() 清空全部节点后 loadAll() 为空
    void clearAll();
    /// 测试目标: 验证重复 URI 写入被拒绝 + existsByUri() 各项行为
    void duplicateUri();
    /// 测试目标: 验证 add() 第三个参数传递公钥后正确持久化
    void addWithPublicKey();
};

/// 创建随机命名的临时 SQLite 数据库，测试完全隔离
static DatabaseConnection makeTempDb()
{
    const QString path = QDir::temp().filePath(
        QStringLiteral("qtet-test-%1.db")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    return DatabaseConnection(path);
}

/// 新增两个节点，断言 loadAll() 返回 2 条记录且字段正确
void TestFavoriteNodeRepository::addAndLoadNodes()
{
    // 准备测试数据：创建临时数据库
    auto conn = makeTempDb();
    QVERIFY(conn.open());

    FavoriteNodeRepository repo(conn.database());

    auto node1 = repo.add(QStringLiteral("节点A"), QStringLiteral("tcp://a.example.com:11010"));
    QVERIFY(node1.has_value());
    // 检查返回的节点 id 大于 0
    QVERIFY(node1->id > 0);
    QCOMPARE(node1->name, QStringLiteral("节点A"));
    QCOMPARE(node1->uri, QStringLiteral("tcp://a.example.com:11010"));

    auto node2 = repo.add(QStringLiteral("节点B"), QStringLiteral("tcp://b.example.com:11010"));
    QVERIFY(node2.has_value());

    auto all = repo.loadAll();
    // 检查 loadAll 返回数量正确
    QCOMPARE(all.size(), 2);
}

/// 新增节点后调用 update() 修改全部字段，断言 loadAll() 返回更新后的值
void TestFavoriteNodeRepository::updateNode()
{
    // 准备测试数据：创建并添加一个节点
    auto conn = makeTempDb();
    QVERIFY(conn.open());

    FavoriteNodeRepository repo(conn.database());

    auto node = repo.add(QStringLiteral("测试"), QStringLiteral("tcp://test.example.com:11010"));
    QVERIFY(node.has_value());

    QVERIFY(repo.update(node->id, QStringLiteral("已更新"), QStringLiteral("tcp://updated.example.com:11010"), QStringLiteral("pubkey123")));

    auto all = repo.loadAll();
    QCOMPARE(all.size(), 1);
    // 检查更新后的字段值
    QCOMPARE(all.first().name, QStringLiteral("已更新"));
    QCOMPARE(all.first().uri, QStringLiteral("tcp://updated.example.com:11010"));
    QCOMPARE(all.first().publicKey, QStringLiteral("pubkey123"));
}

/// 新增节点后调用 remove()，断言该节点不再存在于 loadAll() 结果中
void TestFavoriteNodeRepository::removeNode()
{
    // 准备测试数据：创建并添加一个节点
    auto conn = makeTempDb();
    QVERIFY(conn.open());

    FavoriteNodeRepository repo(conn.database());

    auto node = repo.add(QStringLiteral("待删除"), QStringLiteral("tcp://del.example.com:11010"));
    QVERIFY(node.has_value());

    QVERIFY(repo.remove(node->id));
    // 检查删除后列表为空
    QVERIFY(repo.loadAll().isEmpty());
}

/// 新增 3 个节点后调用 clear()，断言全部删除
void TestFavoriteNodeRepository::clearAll()
{
    // 准备测试数据：创建并添加 3 个节点
    auto conn = makeTempDb();
    QVERIFY(conn.open());

    FavoriteNodeRepository repo(conn.database());

    repo.add(QStringLiteral("A"), QStringLiteral("tcp://a.example.com"));
    repo.add(QStringLiteral("B"), QStringLiteral("tcp://b.example.com"));
    repo.add(QStringLiteral("C"), QStringLiteral("tcp://c.example.com"));
    QCOMPARE(repo.loadAll().size(), 3);

    QVERIFY(repo.clear());
    // 检查清空后列表为空
    QVERIFY(repo.loadAll().isEmpty());
}

/// 测试重复 URI 场景
/// - 相同 URI 第二次添加应失败
/// - existsByUri() 检查存在/不存在的 URI
/// - existsByUri(uri, excludeId) 排除自身后，自己的 URI 不算重复
void TestFavoriteNodeRepository::duplicateUri()
{
    // 准备测试数据：创建临时数据库
    auto conn = makeTempDb();
    QVERIFY(conn.open());

    FavoriteNodeRepository repo(conn.database());

    QVERIFY(repo.add(QStringLiteral("第一"), QStringLiteral("tcp://dup.example.com:11010")).has_value());
    // 相同 URI 第二次添加应失败
    QVERIFY(!repo.add(QStringLiteral("第二"), QStringLiteral("tcp://dup.example.com:11010")).has_value());

    auto all = repo.loadAll();
    QCOMPARE(all.size(), 1);

    // 检查 existsByUri 各项行为
    QVERIFY(repo.existsByUri(QStringLiteral("tcp://dup.example.com:11010")));
    QVERIFY(!repo.existsByUri(QStringLiteral("tcp://nonexist.example.com")));

    auto node2 = repo.add(QStringLiteral("其他"), QStringLiteral("tcp://other.example.com"));
    QVERIFY(node2.has_value());
    QVERIFY(repo.existsByUri(QStringLiteral("tcp://dup.example.com:11010"), node2->id));
    QVERIFY(repo.existsByUri(QStringLiteral("tcp://dup.example.com:11010")));

    // 检查排除自身后自己的 URI 不算重复
    QVERIFY(!repo.existsByUri(QStringLiteral("tcp://other.example.com"), node2->id));
}

/// 新增节点时传递公钥参数，验证公钥被正确存储和读取
/// 并测试 update() 将公钥设为空字符串后能清空
void TestFavoriteNodeRepository::addWithPublicKey()
{
    // 准备测试数据：创建临时数据库
    auto conn = makeTempDb();
    QVERIFY(conn.open());

    FavoriteNodeRepository repo(conn.database());

    auto node = repo.add(QStringLiteral("带公钥节点"), QStringLiteral("tcp://secure.example.com:11010"),
                         QStringLiteral("ABCDEF123456"));
    QVERIFY(node.has_value());
    // 检查公钥被正确存储
    QCOMPARE(node->publicKey, QStringLiteral("ABCDEF123456"));

    auto all = repo.loadAll();
    QCOMPARE(all.size(), 1);
    QCOMPARE(all.first().publicKey, QStringLiteral("ABCDEF123456"));

    // 更新时将公钥设为空，检查可清空
    QVERIFY(repo.update(node->id, QStringLiteral("带公钥节点"), QStringLiteral("tcp://secure.example.com:11010"), QString()));
    all = repo.loadAll();
    QVERIFY(all.first().publicKey.isEmpty());
}

QTEST_MAIN(TestFavoriteNodeRepository)
#include "tst_favorite_node_repository.moc"
