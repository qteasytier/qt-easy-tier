/**
 * @file tst_log_repository.cpp
 * @brief 日志仓库模块的单元测试。测试内容：日志插入与自动裁剪、清空日志表。
 */
#include <QTest>
#include <QDir>
#include <QTemporaryDir>
#include <memory>
#include "core/repository/DatabaseConnection.h"
#include "core/log/LogTypes.h"
#include "core/repository/LogRepository.h"

static_assert(static_cast<int>(LogLevel::Info) == 0);
static_assert(static_cast<int>(LogLevel::Warning) == 1);
static_assert(static_cast<int>(LogLevel::Error) == 2);
static_assert(static_cast<int>(LogLevel::None) == 3);

static_assert(requires(LogEntry entry) {
    entry.id;
    entry.timestamp;
    entry.level;
    entry.context;
    entry.message;
});

class TestLogRepository : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void testInsertAndTrim();
    void testClear();

private:
    QTemporaryDir m_tempDir;
    std::unique_ptr<DatabaseConnection> m_db;
    std::unique_ptr<LogRepository> m_repo;
};

void TestLogRepository::initTestCase()
{
    // 准备测试数据：创建临时数据库和 LogRepository
    QVERIFY(m_tempDir.isValid());
    const QString dbPath = QDir(m_tempDir.path()).filePath(QStringLiteral("test-log.db"));
    m_db = std::make_unique<DatabaseConnection>(dbPath);
    QVERIFY(m_db->open());
    m_repo = std::make_unique<LogRepository>(m_db->database());
}

/// 测试目标：验证插入日志后自动裁剪，保留数量不得超过 maxEntries
void TestLogRepository::testInsertAndTrim()
{
    m_repo->clearAll();
    for (int i = 0; i < 5; ++i)
        m_repo->insertLog(LogLevel::Info, QStringLiteral("msg %1").arg(i), QString(), 3);
    // 检查最多保留 3 条
    QCOMPARE(m_repo->count(), 3);
}

/// 测试目标：验证 clearAll 能清空所有日志
void TestLogRepository::testClear()
{
    m_repo->insertLog(LogLevel::Info, "test", QString(), 100);
    QVERIFY(m_repo->clearAll());
    // 检查清空后 count 为 0
    QCOMPARE(m_repo->count(), 0);
}

QTEST_MAIN(TestLogRepository)
#include "tst_log_repository.moc"
