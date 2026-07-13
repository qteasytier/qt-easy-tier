/**
 * @file tst_settings_store.cpp
 * @brief 设置存储模块的单元测试。测试内容：设置保存与加载往返、非法数值钳位处理。
 */
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include "core/application/settings/SettingsStore.h"

class TestSettingsStore : public QObject {
    Q_OBJECT

private slots:
    /// 测试目标：验证设置的保存与加载完整往返，所有字段值一致
    void saveAndLoad_roundTripsSettings()
    {
        // 准备测试数据：创建临时目录和设置对象
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString path = QDir(tempDir.path()).filePath(QStringLiteral("settings3.json"));

        SettingsStore store(path);
        SettingsStore::Settings settings;
        settings.autoStart = true;
        settings.showExitPrompt = false;
        settings.hideServerNodes = true;
        settings.logLevel = 2;
        settings.maxLogEntries = 250;

        QVERIFY(store.save(settings));

        const SettingsStore::Settings loaded = store.load();
        // 验证结果：加载回来的字段值与保存时一致
        QCOMPARE(loaded.autoStart, true);
        QCOMPARE(loaded.showExitPrompt, false);
        QCOMPARE(loaded.hideServerNodes, true);
        QCOMPARE(loaded.logLevel, 2);
        QCOMPARE(loaded.maxLogEntries, 250);
    }

    /// 测试目标：验证加载时对非法数值进行钳位处理
    /// logLevel 超出上限应钳位到 1，maxLogEntries 为 0 应钳位到 1
    void load_clampsInvalidNumericValues()
    {
        // 准备测试数据：写入包含非法数值的 JSON 文件
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString path = QDir(tempDir.path()).filePath(QStringLiteral("settings3.json"));

        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QJsonObject obj;
        obj[QStringLiteral("autoStart")] = true;
        obj[QStringLiteral("autoReconnect")] = true;
        obj[QStringLiteral("showExitPrompt")] = false;
        obj[QStringLiteral("hideServerNodes")] = true;
        obj[QStringLiteral("logLevel")] = 9;    // 超出上限
        obj[QStringLiteral("maxLogEntries")] = 0; // 非法值
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        file.close();

        SettingsStore store(path);
        const SettingsStore::Settings loaded = store.load();
        // 检查布尔字段值正常
        QCOMPARE(loaded.autoStart, true);
        QCOMPARE(loaded.showExitPrompt, false);
        QCOMPARE(loaded.hideServerNodes, true);
        // 检查非法数值已被钳位
        QCOMPARE(loaded.logLevel, 1);
        QCOMPARE(loaded.maxLogEntries, 1);
    }

    /// 测试目标：JSON 中缺失 showExitPrompt 时默认返回 true
    void load_usesShowExitPromptDefaultWhenMissing()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString path = QDir(tempDir.path()).filePath(QStringLiteral("settings3.json"));

        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QJsonObject obj;
        obj[QStringLiteral("autoStart")] = true;
        obj[QStringLiteral("hideServerNodes")] = true;
        obj[QStringLiteral("logLevel")] = 1;
        obj[QStringLiteral("maxLogEntries")] = 100;
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        file.close();

        SettingsStore store(path);
        const SettingsStore::Settings loaded = store.load();

        QCOMPARE(loaded.showExitPrompt, true);
    }
};

QTEST_MAIN(TestSettingsStore)
#include "tst_settings_store.moc"
