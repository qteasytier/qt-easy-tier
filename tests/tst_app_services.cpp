/**
 * @file tst_app_services.cpp
 * @brief 应用服务装配层模块的单元测试。测试内容：AppServices 构造后暴露必需单例、QmlSingletonRegistrar 函数可链接。
 */
#include <QSqlDatabase>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include "app/AppServices.h"
#include "app/QmlSingletonRegistrar.h"
#include "core/repository/DatabaseConnection.h"
#include "core/system_tray/TrayMessageDispatcher.h"
#include "core/system_tray/TrayMessageSink.h"
#include "core/viewmodel/AppState.h"
#include "core/viewmodel/FavoriteNodeViewModel.h"

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

    /// 测试目标：收藏节点导入完成会通过 AppServices 转发为系统托盘信息通知
    void favoriteNodeImportDispatchesTrayInfoMessage()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        DatabaseConnection db(QDir(tempDir.path()).filePath(QStringLiteral("app-services.db")));
        QVERIFY(db.open());

        const QString importPath = QDir(tempDir.path()).filePath(QStringLiteral("nodes.json"));
        QFile file(importPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("[{\"uri\":\"tcp://new.example.com:11010\",\"display_name\":\"新节点\",\"publicKey\":\"\"}]");
        file.close();

        RecordingTrayMessageSink sink;
        TrayMessageDispatcher::instance()->addSink(&sink);
        AppServices services(db.database(), nullptr, AppServices::SkipDaemonConnection);

        QVERIFY(services.favoriteNodeViewModel());
        QVERIFY(services.favoriteNodeViewModel()->importNodesFromFile(QUrl::fromLocalFile(importPath).toString()));

        QCOMPARE(sink.messages.size(), 1);
        QCOMPARE(sink.messages.first().level, TrayMessageLevel::Info);
        QCOMPARE(sink.messages.first().title, QStringLiteral("节点导入完成"));
        QCOMPARE(sink.messages.first().message, QStringLiteral("已导入 1 个节点，跳过 0 个节点"));
    }
};

QTEST_MAIN(TestAppServices)
#include "tst_app_services.moc"
