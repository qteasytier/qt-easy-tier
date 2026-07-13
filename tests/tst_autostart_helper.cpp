/**
 * @file tst_autostart_helper.cpp
 * @brief 开机自启动辅助工具模块的单元测试。测试内容：Linux XDG Autostart 自启动条目创建、自启动条目删除、默认状态检查。
 *
 * 覆盖：
 * - Linux：XDG Autostart .desktop 文件创建/删除（使用临时目录隔离）
 *
 * 每个测试独立运行，initTestCase/cleanupTestCase 确保初始状态干净。
 */
#include <QTest>
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QDir>
#include "core/util/AutoStartHelper.h"

class TestAutoStartHelper : public QObject {
    Q_OBJECT

private slots:
    /// 测试前: 确保开机自启动功能关闭，避免残留状态影响测试
    void initTestCase()
    {
#if defined(Q_OS_LINUX)
        QVERIFY(m_tempDir.isValid());
        m_desktopPath = QDir(m_tempDir.path()).filePath(QStringLiteral("autostart/QtEasyTier.desktop"));
        AutoStartHelper::setDesktopFilePathOverrideForTesting(m_desktopPath);
        AutoStartHelper::setAutoStart(false);
#else
        QSKIP("AutoStartHelper tests require an isolated backend on this platform");
#endif
    }

    /// 测试后: 清理所有测试可能创建的开机自启动项
    void cleanupTestCase()
    {
#if defined(Q_OS_LINUX)
        AutoStartHelper::setAutoStart(false);
        AutoStartHelper::setDesktopFilePathOverrideForTesting(QString());
#endif
    }

    /// 测试目标: 验证 setAutoStart(true) 正确创建自启动条目
    /// - Linux: 检查临时 autostart/QtEasyTier.desktop 文件存在且内容正确
    void setAutoStart_createsEntry()
    {
        QVERIFY(AutoStartHelper::setAutoStart(true));
        // 检查自启动已启用
        QVERIFY(AutoStartHelper::isAutoStartEnabled());

#if defined(Q_OS_LINUX)
        // 检查 .desktop 文件已创建
        QFile file(m_desktopPath);
        QVERIFY(file.exists());
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        const QByteArray content = file.readAll();
        // 检查文件内容包含必要的 Desktop Entry 字段
        QVERIFY(content.contains("[Desktop Entry]"));
        QVERIFY(content.contains("--autostart"));
        QVERIFY(content.contains("Exec="));
#endif
    }

    /// 测试目标: 验证 setAutoStart(false) 正确删除自启动条目
    /// 先创建再删除，确认 isAutoStartEnabled() 返回 false
    void setAutoStart_removesEntry()
    {
        QVERIFY(AutoStartHelper::setAutoStart(true));
        QVERIFY(AutoStartHelper::isAutoStartEnabled());

        QVERIFY(AutoStartHelper::setAutoStart(false));
        // 检查自启动已禁用
        QVERIFY(!AutoStartHelper::isAutoStartEnabled());
    }

    /// 测试目标: 验证初始状态（未设置自启动时）isAutoStartEnabled() 返回 false
    void isAutoStartEnabled_returnsFalseByDefault()
    {
        AutoStartHelper::setAutoStart(false);
        // 检查默认状态为未启用
        QVERIFY(!AutoStartHelper::isAutoStartEnabled());
    }

private:
    QTemporaryDir m_tempDir;
    QString m_desktopPath;
};

QTEST_MAIN(TestAutoStartHelper)
#include "tst_autostart_helper.moc"
