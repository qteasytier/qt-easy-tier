/**
 * @file tst_settings_viewmodel.cpp
 * @brief SettingsViewModel 开机自启逻辑单元测试。
 *
 * 验证自启动以系统实际状态为唯一权威源（settings3.json 不再持久化该字段）：
 * - autoStart() 实时读取系统状态，不依赖内存缓存
 * - setAutoStart() 创建/删除系统自启动项，系统状态实际变化时发射 autoStartChanged
 * - 同状态设置保持幂等，不重复修改系统或发射信号
 * - refreshAutoStart() 重新发射属性通知
 */
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "platform/AutoStartHelper.h"
#include "viewmodels/SettingsViewModel.h"

class TestSettingsViewModel : public QObject {
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
        QSKIP("SettingsViewModel autostart tests require an isolated backend on this platform");
#endif
    }

    void cleanupTestCase()
    {
#if defined(Q_OS_LINUX)
        AutoStartHelper::setAutoStart(false);
        AutoStartHelper::setDesktopFilePathOverrideForTesting(QString());
#endif
    }

    /// 目标：autoStart() 直接读取系统实际状态（外部修改后 getter 返回最新值，无缓存）
    void autoStart_readsSystemState()
    {
        SettingsViewModel vm;
        QVERIFY(!vm.autoStart());

        // 绕过 ViewModel 直接修改系统自启动项
        QVERIFY(AutoStartHelper::setAutoStart(true));
        QVERIFY(vm.autoStart());

        QVERIFY(AutoStartHelper::setAutoStart(false));
        QVERIFY(!vm.autoStart());
    }

    /// 目标：setAutoStart(true) 创建系统自启动项并发射 autoStartChanged
    void setAutoStart_enablesAndEmitsSignal()
    {
        SettingsViewModel vm;
        QSignalSpy spy(&vm, &SettingsViewModel::autoStartChanged);
        QVERIFY(vm.setAutoStart(true));
        QVERIFY(AutoStartHelper::isAutoStartEnabled());
        QCOMPARE(spy.count(), 1);
    }

    /// 目标：setAutoStart(false) 删除系统自启动项并发射 autoStartChanged
    void setAutoStart_disablesAndEmitsSignal()
    {
        SettingsViewModel vm;
        QSignalSpy spy(&vm, &SettingsViewModel::autoStartChanged);
        QVERIFY(vm.setAutoStart(false));
        QVERIFY(!AutoStartHelper::isAutoStartEnabled());
        QCOMPARE(spy.count(), 1);
    }

    /// 目标：同状态设置保持幂等，不重复修改系统状态或发射信号
    void setAutoStart_isIdempotent()
    {
        SettingsViewModel vm;
        QVERIFY(vm.setAutoStart(true));

        QSignalSpy spy(&vm, &SettingsViewModel::autoStartChanged);
        QVERIFY(vm.setAutoStart(true));
        QCOMPARE(spy.count(), 0);
        QVERIFY(AutoStartHelper::isAutoStartEnabled());
    }

    /// 目标：refreshAutoStart() 重新发射属性通知，供 QML 重新读取系统状态
    void refreshAutoStart_emitsSignal()
    {
        SettingsViewModel vm;
        QSignalSpy spy(&vm, &SettingsViewModel::autoStartChanged);
        vm.refreshAutoStart();
        QCOMPARE(spy.count(), 1);
    }

private:
    QTemporaryDir m_tempDir;
    QString m_desktopPath;
};

QTEST_MAIN(TestSettingsViewModel)
#include "tst_settings_viewmodel.moc"
