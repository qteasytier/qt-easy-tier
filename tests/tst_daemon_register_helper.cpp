/**
 * @file tst_daemon_register_helper.cpp
 * @brief daemon systemd 注册辅助工具单元测试。
 */
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "core/util/DaemonRegisterHelper.h"

class TestDaemonRegisterHelper : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
#if defined(Q_OS_LINUX)
        QVERIFY(m_tempDir.isValid());
        m_servicePath = QDir(m_tempDir.path()).filePath(QStringLiteral("qtet-daemon.service"));
        m_daemonPath = QDir(m_tempDir.path()).filePath(QStringLiteral("qtet-daemon"));
        DaemonRegisterHelper::setSystemdServicePathOverrideForTesting(m_servicePath);
        DaemonRegisterHelper::setDaemonBinaryPathOverrideForTesting(m_daemonPath);
#else
        QSKIP("DaemonRegisterHelper systemd tests require Linux");
#endif
    }

    void cleanupTestCase()
    {
#if defined(Q_OS_LINUX)
        DaemonRegisterHelper::setSystemdServicePathOverrideForTesting(QString());
        DaemonRegisterHelper::setDaemonBinaryPathOverrideForTesting(QString());
        DaemonRegisterHelper::setDaemonProcessRunningOverrideForTesting(false, false);
#endif
    }

    void isServiceRegistered_returnsFalseWhenServiceFileMissing()
    {
#if defined(Q_OS_LINUX)
        QFile::remove(m_servicePath);
        QVERIFY(!DaemonRegisterHelper::isServiceRegistered());
#endif
    }

    void isServiceRegistered_returnsTrueWhenServiceFileExists()
    {
#if defined(Q_OS_LINUX)
        QFile file(m_servicePath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("[Unit]\nDescription=EasyTier Service\n");
        file.close();

        QVERIFY(DaemonRegisterHelper::isServiceRegistered());
#endif
    }

    void ensureDaemonService_returnsMissingWhenDaemonBinaryMissing()
    {
#if defined(Q_OS_LINUX)
        QFile::remove(m_daemonPath);
        QCOMPARE(DaemonRegisterHelper::ensureDaemonService(), DaemonRegisterHelper::EnsureResult::DaemonBinaryMissing);
#endif
    }

    void requiredAction_returnsRegisterServiceWhenServiceFileMissing()
    {
#if defined(Q_OS_LINUX)
        QFile daemonFile(m_daemonPath);
        QVERIFY(daemonFile.open(QIODevice::WriteOnly));
        daemonFile.close();
        QVERIFY(daemonFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QFile::remove(m_servicePath);

        QCOMPARE(DaemonRegisterHelper::requiredAction(), DaemonRegisterHelper::RequiredAction::RegisterService);
#endif
    }

    void requiredAction_returnsStartServiceWhenRegisteredButProcessNotRunning()
    {
#if defined(Q_OS_LINUX)
        QFile daemonFile(m_daemonPath);
        QVERIFY(daemonFile.open(QIODevice::WriteOnly));
        daemonFile.close();
        QVERIFY(daemonFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

        QFile serviceFile(m_servicePath);
        QVERIFY(serviceFile.open(QIODevice::WriteOnly | QIODevice::Text));
        serviceFile.write("[Unit]\nDescription=EasyTier Service\n");
        serviceFile.close();
        DaemonRegisterHelper::setDaemonProcessRunningOverrideForTesting(true, false);

        QCOMPARE(DaemonRegisterHelper::requiredAction(), DaemonRegisterHelper::RequiredAction::StartService);
#endif
    }

    void requiredAction_returnsNoneWhenRegisteredAndProcessRunning()
    {
#if defined(Q_OS_LINUX)
        QFile daemonFile(m_daemonPath);
        QVERIFY(daemonFile.open(QIODevice::WriteOnly));
        daemonFile.close();
        QVERIFY(daemonFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

        QFile serviceFile(m_servicePath);
        QVERIFY(serviceFile.open(QIODevice::WriteOnly | QIODevice::Text));
        serviceFile.write("[Unit]\nDescription=EasyTier Service\n");
        serviceFile.close();
        DaemonRegisterHelper::setDaemonProcessRunningOverrideForTesting(true, true);

        QCOMPARE(DaemonRegisterHelper::requiredAction(), DaemonRegisterHelper::RequiredAction::None);
#endif
    }

    void systemdServiceContent_containsDaemonPathAndWorkingDirectory()
    {
#if defined(Q_OS_LINUX)
        const QString content = DaemonRegisterHelper::systemdServiceContentForTesting(m_daemonPath);

        QVERIFY(content.contains(QStringLiteral("ExecStart=%1").arg(m_daemonPath)));
        QVERIFY(content.contains(QStringLiteral("WorkingDirectory=%1").arg(QFileInfo(m_daemonPath).absolutePath())));
        QVERIFY(content.contains(QStringLiteral("Restart=always")));
#endif
    }

private:
    QTemporaryDir m_tempDir;
    QString m_servicePath;
    QString m_daemonPath;
};

QTEST_MAIN(TestDaemonRegisterHelper)
#include "tst_daemon_register_helper.moc"
