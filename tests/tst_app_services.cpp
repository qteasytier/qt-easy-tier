/**
 * @file tst_app_services.cpp
 * @brief 应用服务装配层模块的单元测试。测试内容：AppServices 构造后暴露必需单例、QmlSingletonRegistrar 函数可链接。
 */
#include <QSqlDatabase>
#include <QTest>

#include "app/AppServices.h"
#include "app/QmlSingletonRegistrar.h"
#include "core/system_tray/TrayMessageDispatcher.h"
#include "core/system_tray/TrayMessageSink.h"
#include "core/viewmodel/AppState.h"

class RecordingTrayMessageSink final : public TrayMessageSink {
public:
    void showTrayMessage(const TrayMessage &message) override
    {
        messages.append(message);
    }

    QList<TrayMessage> messages;
};

class TestAppServices : public QObject {
    Q_OBJECT

private slots:
    void cleanup()
    {
        TrayMessageDispatcher::instance()->clearSinks();
    }

    /// 测试目标：验证 AppServices 构造后有必需的 singleton 可用
    void constructedServicesExposeRequiredSingletons()
    {
        AppServices services(QSqlDatabase(), nullptr, AppServices::SkipDaemonConnection);

        // 检查各核心服务非空
        QVERIFY(services.appState());
        QVERIFY(services.settingsViewModel());
        QVERIFY(services.favoriteNodeViewModel() == nullptr);
        QVERIFY(services.logViewModel() == nullptr);
        QVERIFY(services.fontHelper());
        QVERIFY(services.daemonClient());
        QVERIFY(services.daemonApi());
        QVERIFY(services.backendStatusViewModel());
        QVERIFY(services.systemTrayManager());
    }

    /// 测试目标：验证 registerQmlSingletons 函数符号可链接
    void singletonRegistrarFunctionIsLinkable()
    {
        auto registrar = &registerQmlSingletons;
        // 检查函数指针非空
        QVERIFY(registrar != nullptr);
    }

    /// 测试目标：全局错误会通过装配层转发为系统托盘错误通知
    void appStateErrorDispatchesTrayErrorMessage()
    {
        RecordingTrayMessageSink sink;
        TrayMessageDispatcher::instance()->addSink(&sink);
        AppServices services(QSqlDatabase(), nullptr, AppServices::SkipDaemonConnection);

        services.appState()->showError(QStringLiteral("保存失败"));

        QCOMPARE(sink.messages.size(), 1);
        QCOMPARE(sink.messages.first().level, TrayMessageLevel::Error);
        QCOMPARE(sink.messages.first().title, QStringLiteral("错误"));
        QCOMPARE(sink.messages.first().message, QStringLiteral("保存失败"));
    }
};

QTEST_MAIN(TestAppServices)
#include "tst_app_services.moc"
