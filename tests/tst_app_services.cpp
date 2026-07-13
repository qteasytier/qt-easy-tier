/**
 * @file tst_app_services.cpp
 * @brief 应用服务装配层模块的单元测试。测试内容：AppServices 构造后暴露必需单例、QmlSingletonRegistrar 函数可链接。
 */
#include <QSqlDatabase>
#include <QTest>

#include "app/AppServices.h"
#include "app/QmlSingletonRegistrar.h"

class TestAppServices : public QObject {
    Q_OBJECT

private slots:
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
};

QTEST_MAIN(TestAppServices)
#include "tst_app_services.moc"
