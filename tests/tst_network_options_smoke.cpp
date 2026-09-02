/**
 * @file tst_network_options_smoke.cpp
 * @brief 网络配置编辑器薄壳的 QML 冒烟测试。
 *
 * 用真实 AppServices 装配 + registerQmlSingletons 构建 QML 环境，
 * 从构建树的模块目录加载 pages/NetworkOptions.qml，
 * 验证数据驱动薄壳按 formSections 元数据完成实例化且无任何 QML 错误
 * （覆盖 10 个表单渲染器的创建与全部字段绑定的首次求值）。
 */
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QSqlDatabase>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <memory>

#include "app/AppServices.h"
#include "app/QmlSingletonRegistrar.h"
#include "core/viewmodels/ConfigEditorViewModel.h"
#include "sqlite_repository/DatabaseConnection.h"

namespace {
/// 收集 QML 错误/警告的消息缓冲（qt.qml 频道的消息视为失败）
QStringList g_qmlMessages;
void qmlMessageCollector(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg)
        g_qmlMessages.append(QStringLiteral("%1:%2 %3").arg(context.file).arg(context.line).arg(msg));
}
} // namespace

class TestNetworkOptionsSmoke : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tempDir;
    std::unique_ptr<DatabaseConnection> m_db;

private slots:
    void init()
    {
        QVERIFY(m_tempDir.isValid());
        m_db = std::make_unique<DatabaseConnection>(
            m_tempDir.filePath(QStringLiteral("smoke.db")));
        QVERIFY(m_db->open());
    }

    void cleanup()
    {
        m_db.reset();
    }

    /// 目标：薄壳页面在真实装配环境下加载成功，动态渲染全部字段且无 QML 错误
    void networkOptionsLoadsAndRendersWithoutQmlErrors() {
        const QString buildDir = QStringLiteral(QTET_TEST_QML_DIR);

        QQmlApplicationEngine engine;
        // 构建树根下有 QtEasyTier/ 与 SwbControls/ 两个模块目录（由 copy_qml 目标生成）
        engine.addImportPath(buildDir);

        // 真实临时数据库：AppServices 对配置相关 ViewModel 采用条件创建，空库时为 null
        AppServices services(m_db->database(), &engine, AppServices::SkipDaemonConnection);
        registerQmlSingletons(engine, services);

        // C++ 侧预检：单例实例存在且携带表单元数据（区分实例问题与 QML 解析问题）
        auto *editor = services.configEditorViewModel();
        QVERIFY2(editor != nullptr, "AppServices 未创建 ConfigEditorViewModel（空库懒创建？）");
        QVERIFY(editor->formSections().size() >= 6);
        QVERIFY(editor->metaObject()->indexOfProperty("formSections") >= 0);
        QVERIFY(editor->metaObject()->indexOfMethod("flushAutoSave()") >= 0);

        g_qmlMessages.clear();
        const QtMessageHandler oldHandler = qInstallMessageHandler(qmlMessageCollector);
        engine.load(QUrl::fromLocalFile(buildDir + QStringLiteral("/QtEasyTier/pages/NetworkOptions.qml")));
        qInstallMessageHandler(oldHandler);

        if (engine.rootObjects().isEmpty()) {
            QFAIL(qPrintable(QStringLiteral("QML 加载失败:\n%1").arg(g_qmlMessages.join(u'\n'))));
        }

        // 让延迟求值的绑定（联动禁用等）完成一轮事件循环
        QTest::qWait(100);

        QVERIFY2(g_qmlMessages.isEmpty(),
                 qPrintable(QStringLiteral("QML 消息:\n%1").arg(g_qmlMessages.join(u'\n'))));
    }
};

QTEST_MAIN(TestNetworkOptionsSmoke)
#include "tst_network_options_smoke.moc"
