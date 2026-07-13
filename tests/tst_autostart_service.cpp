/**
 * @file tst_autostart_service.cpp
 * @brief 开机自启动服务模块的单元测试。测试内容：启用/禁用自启动时后端状态同步更新。
 */
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "core/application/settings/AutoStartService.h"
#include "core/util/AutoStartHelper.h"

class TestAutoStartService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
#if defined(Q_OS_LINUX)
        QVERIFY(m_tempDir.isValid());
        m_desktopPath = QDir(m_tempDir.path()).filePath(QStringLiteral("autostart/QtEasyTier.desktop"));
        AutoStartHelper::setDesktopFilePathOverrideForTesting(m_desktopPath);
        AutoStartHelper::setAutoStart(false);
#else
        QSKIP("AutoStartService tests require an isolated backend on this platform");
#endif
    }

    void cleanupTestCase()
    {
#if defined(Q_OS_LINUX)
        AutoStartHelper::setAutoStart(false);
        AutoStartHelper::setDesktopFilePathOverrideForTesting(QString());
#endif
    }

    /// 测试目标：验证 setEnabled 能正确更新后端自启动状态
    void setEnabled_updatesBackendState()
    {
        AutoStartService service;

        // 启用自启动，检查状态同步
        QVERIFY(service.setEnabled(true));
        QVERIFY(service.isEnabled());

        // 禁用自启动，检查状态同步
        QVERIFY(service.setEnabled(false));
        QVERIFY(!service.isEnabled());
    }

private:
    QTemporaryDir m_tempDir;
    QString m_desktopPath;
};

QTEST_MAIN(TestAutoStartService)
#include "tst_autostart_service.moc"
