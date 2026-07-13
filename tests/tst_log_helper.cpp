/**
 * @file tst_log_helper.cpp
 * @brief 日志辅助工具模块的单元测试。测试内容：日志分发器级别过滤、日志级别动态切换过滤。
 */
#include <QTest>
#include <QDir>
#include <QStandardPaths>
#include <QTemporaryDir>
#include "core/repository/DatabaseConnection.h"
#include "core/application/logging/RepositoryLogSink.h"
#include "core/log/LogDispatcher.h"
#include "core/log/LogSink.h"
#include "core/repository/LogRepository.h"
#include "core/util/LogHelper.h"
#include "core/viewmodel/SettingsViewModel.h"

class CapturingSink final : public LogSink
{
public:
    QList<LogEntry> entries;

    void write(const LogEntry &entry) override
    {
        entries.append(entry);
    }
};

class TestLogHelper : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void testDispatcherFiltering();
    void testFiltering();

private:
    QTemporaryDir m_tempDir;
};

void TestLogHelper::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

/// 测试目标：验证 LogDispatcher 根据 minimumLevel 正确过滤日志级别
void TestLogHelper::testDispatcherFiltering()
{
    // 准备测试数据：创建 Dispatcher 和 CapturingSink
    LogDispatcher dispatcher;
    CapturingSink sink;
    dispatcher.addSink(&sink);
    dispatcher.setMinimumLevel(LogLevel::Warning);

    // 写入不同级别的日志
    dispatcher.log(LogLevel::Info, QStringLiteral("info msg"), QStringLiteral("Test"));
    dispatcher.log(LogLevel::Warning, QStringLiteral("warn msg"), QStringLiteral("Test"));
    dispatcher.log(LogLevel::Error, QStringLiteral("error msg"), QStringLiteral("Test"));

    // 检查仅 Warning 和 Error 级别被记录
    QCOMPARE(sink.entries.size(), 2);
    QCOMPARE(sink.entries.at(0).level, LogLevel::Warning);
    QCOMPARE(sink.entries.at(0).message, QStringLiteral("warn msg"));
    QCOMPARE(sink.entries.at(0).context, QStringLiteral("Test"));
    QCOMPARE(sink.entries.at(1).level, LogLevel::Error);
    QCOMPARE(sink.entries.at(1).message, QStringLiteral("error msg"));
}

/// 测试目标：验证通过 SettingsViewModel 动态切换日志级别后 LogHelper 正确过滤
void TestLogHelper::testFiltering()
{
    // 准备测试数据：创建数据库、仓库和日志组件
    QVERIFY(m_tempDir.isValid());
    const QString dbPath = QDir(m_tempDir.path()).filePath(QStringLiteral("test-log.db"));
    auto *db = new DatabaseConnection(dbPath);
    QVERIFY(db->open());
    auto *repo = new LogRepository(db->database(), this);
    repo->clearAll();
    auto *settings = new SettingsViewModel(nullptr, this);
    settings->setLogLevel(static_cast<int>(LogLevel::Info));
    settings->setMaxLogEntries(100);
    auto *sink = new RepositoryLogSink(repo, this);
    sink->setMaxEntries(settings->maxLogEntries());
    auto *dispatcher = LogDispatcher::instance();
    dispatcher->clearSinks();
    dispatcher->setMinimumLevel(static_cast<LogLevel>(settings->logLevel()));
    dispatcher->addSink(sink);
    connect(settings, &SettingsViewModel::logLevelChanged, dispatcher, [dispatcher, settings]() {
        dispatcher->setMinimumLevel(static_cast<LogLevel>(settings->logLevel()));
    });
    connect(settings, &SettingsViewModel::maxLogEntriesChanged, sink, [sink, settings]() {
        sink->setMaxEntries(settings->maxLogEntries());
    });

    // 写入不同级别日志，Info 级别全部写入
    LogHelper::logInfo("info msg", "Test");
    LogHelper::logWarning("warn msg", "Test");
    LogHelper::logError("error msg", "Test");
    // 检查 3 条全部写入
    QCOMPARE(repo->count(), 3);

    // 切换到 Warning 级别后，Info 不再写入
    settings->setLogLevel(static_cast<int>(LogLevel::Warning));
    LogHelper::logInfo("info msg 2", "Test");
    // 检查 count 不变，Info 被过滤
    QCOMPARE(repo->count(), 3);

    // 切换到 None 级别后，所有日志均不写入
    settings->setLogLevel(static_cast<int>(LogLevel::None));
    LogHelper::logError("error msg 2", "Test");
    // 检查 count 仍不变
    QCOMPARE(repo->count(), 3);
}

QTEST_MAIN(TestLogHelper)
#include "tst_log_helper.moc"
