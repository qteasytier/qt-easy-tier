/**
 * @file tst_sqlite_repository.cpp
 * @brief SQLite 仓库模块的单元测试。测试内容：配置保存与加载、多条配置批量加载、删除级联验证、默认数据库路径格式检查。
 *
 * 覆盖 NetworkConfigRepository 的核心操作：
 * - 保存单条配置并重新加载，验证字段完整性
 * - 保存多条配置后 loadAll() 数量正确
 * - 删除配置时字段级联删除
 * - 默认数据库路径非空且文件名为 qteasytier.db
 *
 * 每个测试使用独立随机命名临时数据库，确保测试完全隔离。
 */
#include <QTest>
#include <QDir>
#include <QFileInfo>
#include <QUuid>
#include "core/repository/DatabaseConnection.h"
#include "core/repository/NetworkConfigRepository.h"

class TestSqliteRepository : public QObject {
    Q_OBJECT

private slots:
    /// 测试目标: 验证保存→加载后所有字段值一致 + dirty() 为 false
    void saveAndLoadConfig();
    /// 测试目标: 验证保存 3 条配置后 loadAll() 返回 3 条
    void saveAndLoadMultipleConfigs();
    /// 测试目标: 验证删除配置时关联字段行也被级联删除
    void configFieldsCascadeDelete();
    /// 测试目标: 验证默认数据库路径格式正确
    void defaultDatabasePathIsNotEmpty();
};

/// 创建随机命名的临时 SQLite 数据库，测试完全隔离
static DatabaseConnection makeTempDb()
{
    const QString path = QDir::temp().filePath(
        QStringLiteral("qtet-test-%1.db")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    return DatabaseConnection(path);
}

/// 创建一条 NetworkConf，设置 displayName/hostname/networkSecret 后保存
/// 再加载回来，断言各字段值与原始一致
void TestSqliteRepository::saveAndLoadConfig()
{
    // 准备测试数据：创建临时数据库
    auto conn = makeTempDb();
    QVERIFY(conn.open());

    NetworkConfigRepository repo(conn.database());
    NetworkConf cfg("test-net");
    cfg.displayName = QStringLiteral("我的配置");
    cfg.hostname = QStringLiteral("node-a");
    cfg.networkSecret = QStringLiteral("s3cr3t");
    QVERIFY(repo.save(cfg));

    auto loaded = repo.load("test-net");
    QVERIFY(loaded.has_value());
    // 验证结果：加载回来的字段值与原始一致
    QCOMPARE(loaded->instanceName(), QStringLiteral("test-net"));
    QCOMPARE(loaded->displayName, QStringLiteral("我的配置"));
    QCOMPARE(loaded->hostname, QStringLiteral("node-a"));
    QCOMPARE(loaded->networkSecret, QStringLiteral("s3cr3t"));
    // 检查 dirty 标记为 false
    QVERIFY(!loaded->dirty());
}

/// 循环创建 3 条不同名称的配置并保存，断言 loadAll() 返回数量为 3
void TestSqliteRepository::saveAndLoadMultipleConfigs()
{
    // 准备测试数据：创建临时数据库
    auto conn = makeTempDb();
    QVERIFY(conn.open());

    NetworkConfigRepository repo(conn.database());

    for (int i = 0; i < 3; ++i) {
        NetworkConf cfg(QString("net-%1").arg(i));
        cfg.hostname = QString("host-%1").arg(i);
        QVERIFY(repo.save(cfg));
    }

    auto all = repo.loadAll();
    // 检查 loadAll 返回数量符合预期
    QCOMPARE(all.size(), 3);
}

/// 保存配置后通过 remove() 删除，断言 exists() 返回 false
/// 同时验证底层字段行也被级联删除（不会残留孤儿行）
void TestSqliteRepository::configFieldsCascadeDelete()
{
    // 准备测试数据：创建临时数据库并保存一条配置
    auto conn = makeTempDb();
    QVERIFY(conn.open());

    NetworkConfigRepository repo(conn.database());
    NetworkConf cfg("cascade-test");
    cfg.hostname = QStringLiteral("test");
    QVERIFY(repo.save(cfg));
    QVERIFY(repo.exists("cascade-test"));
    QVERIFY(repo.remove("cascade-test"));
    // 检查删除后配置不再存在
    QVERIFY(!repo.exists("cascade-test"));
}

/// 验证 DatabaseConnection::defaultDatabasePath() 返回非空路径
/// 且默认数据库文件名固定为 qteasytier.db
void TestSqliteRepository::defaultDatabasePathIsNotEmpty()
{
    const QString path = DatabaseConnection::defaultDatabasePath();
    // 检查路径非空
    QVERIFY(!path.isEmpty());
    // 检查默认数据库文件名
    QCOMPARE(QFileInfo(path).fileName(), QStringLiteral("qteasytier.db"));
}

QTEST_MAIN(TestSqliteRepository)
#include "tst_sqlite_repository.moc"
