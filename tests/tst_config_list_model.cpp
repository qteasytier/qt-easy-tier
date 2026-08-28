/**
 * @file tst_config_list_model.cpp
 * @brief 配置列表模型模块的单元测试
 *
 * 覆盖：
 * - 删除运行中配置时，数据库记录应保留到状态变为 Unstarted 后才删除
 * - 删除未运行配置时，应立即删除数据库记录
 */
#include <QTest>
#include <QFile>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QUrl>
#include <QUuid>
#include <QMutex>
#include <QMutexLocker>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <memory>

#include "app_service/config/ConfigCommandService.h"
#include "app_service/config/ConfigImportExportService.h"
#include "app_service/runtime/NodeInfoModel.h"
#include "app_service/runtime/RuntimeLogModel.h"
#include "app_service/runtime/VpnRuntimeService.h"
#include "core/config/ConfigRunState.h"
#include "core/repository/DatabaseConnection.h"
#include "core/repository/NetworkConfigRepository.h"
#include "viewmodels/ConfigListModel.h"
#include "viewmodels/ConfigEditorViewModel.h"
#include "viewmodels/runtime/NetworkPageViewModel.h"
#include "core/service/DaemonApi.h"
#include "core/service/DaemonClient.h"
#include "core/vpn_manager/StatusMonitor.h"
#include "core/vpn_manager/VpnController.h"
#include "core/config/NetworkConf.h"

static_assert(configRunStateCanDelete(ConfigRunState::Stopped));
static_assert(configRunStateCanDelete(ConfigRunState::Error));
static_assert(!configRunStateCanDelete(ConfigRunState::Starting));
static_assert(!configRunStateCanDelete(ConfigRunState::Running));
static_assert(!configRunStateCanDelete(ConfigRunState::Stopping));

namespace {
QMutex g_capturedMessagesMutex;
QStringList *g_capturedMessages = nullptr;

void captureInfoMessages(QtMsgType type, const QMessageLogContext &, const QString &message)
{
    if (type != QtInfoMsg)
        return;

    QMutexLocker locker(&g_capturedMessagesMutex);
    if (g_capturedMessages)
        g_capturedMessages->append(message);
}
}

class TestConfigListModel : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tempDir;
    std::unique_ptr<DatabaseConnection> m_db;
    std::unique_ptr<NetworkConfigRepository> m_repo;

    void insertConfig(const QString &instanceName, const QString &displayName) {
        NetworkConf cfg(instanceName);
        cfg.displayName = displayName;
        QVERIFY(m_repo->save(cfg));
    }

private slots:
    void init() {
        QVERIFY(m_tempDir.isValid());
        const QString dbPath = m_tempDir.path() + QStringLiteral("/test-%1.db")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
        m_db = std::make_unique<DatabaseConnection>(dbPath);
        QVERIFY(m_db->open());
        m_repo = std::make_unique<NetworkConfigRepository>(m_db->database(), this);
    }

    void cleanup() {
        m_repo.reset();
        m_db.reset();
    }

    /// 目标：运行中配置调用 deleteConfig 后，数据库不立即删除；等状态变为 Unstarted 后再删除
    void deleteRunningConfig_deferredUntilStopped() {
        // 准备测试数据：插入一条配置并模拟运行状态
        insertConfig(QStringLiteral("inst-1"), QStringLiteral("配置1"));

        ConfigCommandService commandService(m_repo.get(), this);
        ConfigListModel model(&commandService, nullptr, this);
        QCOMPARE(model.rowCount(), 1);

        // 模拟 daemon 报告该配置正在运行
        model.onRunningStateChanged(QStringLiteral("inst-1"),
                                    ConfigRunState::Running);

        // 发起删除
        QVERIFY(model.deleteConfig(QStringLiteral("inst-1")));
        // 验证结果：运行中删除不立即生效
        QCOMPARE(model.rowCount(), 1);
        QVERIFY(m_repo->exists(QStringLiteral("inst-1")));

        // 模拟 daemon 报告已停止，触发延迟删除
        QSignalSpy spy(&model, &ConfigListModel::configDeleted);
        model.onRunningStateChanged(QStringLiteral("inst-1"),
                                    ConfigRunState::Stopped);
        // 检查已删除
        QCOMPARE(model.rowCount(), 0);
        QVERIFY(!m_repo->exists(QStringLiteral("inst-1")));
        QCOMPARE(spy.count(), 1);
    }

    /// 目标：未运行配置调用 deleteConfig 后，应立即删除
    void deleteStoppedConfig_immediate() {
        // 准备测试数据：插入一条配置
        insertConfig(QStringLiteral("inst-2"), QStringLiteral("配置2"));

        ConfigCommandService commandService(m_repo.get(), this);
        ConfigListModel model(&commandService, nullptr, this);
        QCOMPARE(model.rowCount(), 1);

        QSignalSpy spy(&model, &ConfigListModel::configDeleted);
        QVERIFY(model.deleteConfig(QStringLiteral("inst-2")));
        // 检查立即删除
        QCOMPARE(model.rowCount(), 0);
        QVERIFY(!m_repo->exists(QStringLiteral("inst-2")));
        QCOMPARE(spy.count(), 1);
    }

    /// 目标：导入服务在 TOML 解析失败时直接返回失败，不访问 daemon 或仓库写入
    void importServiceRejectsInvalidToml() {
        // 准备测试数据：写入非法 TOML 文件
        const QString path = m_tempDir.path() + QStringLiteral("/invalid.toml");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("not = [valid");
        file.close();

        ConfigImportExportService service(m_repo.get(), nullptr, this);
        auto future = service.importFromFile(QUrl::fromLocalFile(path));
        QVERIFY(future.isFinished());
        const auto result = future.result();

        // 检查导入失败且不写入仓库
        QVERIFY(!result.success);
        QVERIFY(result.message.contains(QStringLiteral("TOML")));
        QCOMPARE(m_repo->loadAll().size(), 0);
    }

    /// 目标：命令服务负责创建和重命名配置，列表模型之外也可独立测试
    void commandServiceRenamesConfig() {
        ConfigCommandService service(m_repo.get(), this);

        const auto created = service.createNewConfig();
        QVERIFY(created.success);
        QVERIFY(!created.instanceName.isEmpty());

        const QString newName = QStringLiteral("renamed-config");
        const auto renamed = service.renameConfig(created.instanceName, newName);
        QVERIFY(renamed.success);

        // 验证结果：重命名后 displayName 已更新
        const auto loaded = m_repo->load(created.instanceName);
        QVERIFY(loaded.has_value());
        QCOMPARE(loaded->displayName, newName);
    }

    /// 目标：运行状态模型提供稳定 role，QML 不再依赖 QVariantMap 字段名
    void runtimeModelsExposeStableRoles() {
        NodeInfoModel nodeModel;
        const auto nodeRoles = nodeModel.roleNames().values();
        // 检查节点信息模型暴露关键 role
        QVERIFY(nodeRoles.contains("virtualIp"));
        QVERIFY(nodeRoles.contains("connectionTypeText"));
        QVERIFY(nodeRoles.contains("latencyText"));

        RuntimeLogModel logModel;
        const auto logRoles = logModel.roleNames().values();
        // 检查运行日志模型暴露关键 role
        QVERIFY(logRoles.contains("timestamp"));
        QVERIFY(logRoles.contains("message"));
    }

    /// 目标：节点信息模型暴露可绑定 count，QML 显隐绑定可随 model reset 更新
    void nodeInfoModelCountNotifiesWhenItemsChange() {
        NodeInfoModel model;
        QVERIFY(model.metaObject()->indexOfProperty("count") >= 0);
        QSignalSpy countSpy(&model, SIGNAL(countChanged()));

        // 准备测试数据：构造一个节点信息
        QVariantMap node;
        node[QStringLiteral("virtualIp")] = QStringLiteral("10.0.0.1");
        node[QStringLiteral("hostname")] = QStringLiteral("local");
        node[QStringLiteral("isLocalNode")] = true;

        model.setFromVariantList({node});

        // 检查 count 属性和信号
        QCOMPARE(model.property("count").toInt(), 1);
        QCOMPARE(countSpy.count(), 1);

        // 清空后检查 count 更新
        model.setFromVariantList({});

        QCOMPARE(model.property("count").toInt(), 0);
        QCOMPARE(countSpy.count(), 2);
    }

    /// 目标：隐藏服务节点只影响模型可见行，关闭后可从内部缓存恢复显示
    void nodeInfoModelCanHideServerNodesWithoutDroppingCachedItems() {
        NodeInfoModel model;
        QVERIFY(model.metaObject()->indexOfProperty("hideServerNodes") >= 0);

        QVariantMap local;
        local[QStringLiteral("virtualIp")] = QStringLiteral("10.0.0.1");
        local[QStringLiteral("hostname")] = QStringLiteral("local");
        local[QStringLiteral("isLocalNode")] = true;
        local[QStringLiteral("connType")] = QStringLiteral("Local");

        QVariantMap peer;
        peer[QStringLiteral("virtualIp")] = QStringLiteral("10.0.0.2");
        peer[QStringLiteral("hostname")] = QStringLiteral("peer");
        peer[QStringLiteral("connType")] = QStringLiteral("P2P");

        QVariantMap server;
        server[QStringLiteral("virtualIp")] = QStringLiteral("10.0.0.3");
        server[QStringLiteral("hostname")] = QStringLiteral("server");
        server[QStringLiteral("connType")] = QStringLiteral("Server");

        model.setFromVariantList({local, peer, server});
        QCOMPARE(model.property("count").toInt(), 3);

        model.setProperty("hideServerNodes", true);
        QCOMPARE(model.property("count").toInt(), 2);
        QCOMPARE(model.index(0, 0).data(NodeInfoModel::HostnameRole).toString(), QStringLiteral("local"));
        QCOMPARE(model.index(1, 0).data(NodeInfoModel::HostnameRole).toString(), QStringLiteral("peer"));

        model.setProperty("hideServerNodes", false);
        QCOMPARE(model.property("count").toInt(), 3);
        QCOMPARE(model.index(2, 0).data(NodeInfoModel::HostnameRole).toString(), QStringLiteral("server"));
    }

    /// 目标：运行日志模型暴露可绑定 count，QML 显隐绑定可随 model reset 更新
    void runtimeLogModelCountNotifiesWhenItemsChange() {
        RuntimeLogModel model;
        QVERIFY(model.metaObject()->indexOfProperty("count") >= 0);
        QSignalSpy countSpy(&model, SIGNAL(countChanged()));

        // 准备测试数据：构造一条日志
        QVariantMap entry;
        entry[QStringLiteral("timestamp")] = QStringLiteral("07-12 12:00:00");
        entry[QStringLiteral("message")] = QStringLiteral("TUN 设备就绪");

        model.setFromVariantList({entry});

        // 检查 count 属性和信号
        QCOMPARE(model.property("count").toInt(), 1);
        QCOMPARE(countSpy.count(), 1);

        // 清空后检查 count 更新
        model.setFromVariantList({});

        QCOMPARE(model.property("count").toInt(), 0);
        QCOMPARE(countSpy.count(), 2);
    }

    /// 目标：运行日志模型提供完整多行文本，供 QML 文本显示框直接绑定
    void runtimeLogModelExposesPlainText() {
        RuntimeLogModel model;
        QVERIFY(model.metaObject()->indexOfProperty("plainText") >= 0);

        // 准备测试数据：两条日志
        QVariantMap first;
        first[QStringLiteral("timestamp")] = QStringLiteral("07-12 12:00:00");
        first[QStringLiteral("message")] = QStringLiteral("first event");

        QVariantMap second;
        second[QStringLiteral("timestamp")] = QStringLiteral("07-12 12:00:01");
        second[QStringLiteral("message")] = QStringLiteral("second event");

        model.setFromVariantList({first, second});

        // 检查 plainText 格式正确
        QCOMPARE(model.property("plainText").toString(),
                 QStringLiteral("[07-12 12:00:00] first event\n[07-12 12:00:01] second event"));
    }

    /// 目标：daemon 每次返回最近一段 events，controller 只追加比本地最新事件戳更新的日志
    void vpnControllerCachesOnlyLogsNewerThanLatestCachedTimestamp() {
        VpnController controller(QStringLiteral("inst-logs"), nullptr, nullptr, this);

        // 准备测试数据：构造不同时间戳的日志
        QVariantMap first;
        first[QStringLiteral("rawTimestamp")] = QStringLiteral("2026-07-12T12:00:00.000000000+08:00");
        first[QStringLiteral("timestamp")] = QStringLiteral("07-12 12:00:00");
        first[QStringLiteral("message")] = QStringLiteral("first event");

        QVariantMap second;
        second[QStringLiteral("rawTimestamp")] = QStringLiteral("2026-07-12T12:00:01.000000000+08:00");
        second[QStringLiteral("timestamp")] = QStringLiteral("07-12 12:00:01");
        second[QStringLiteral("message")] = QStringLiteral("second event");

        QVariantMap third;
        third[QStringLiteral("rawTimestamp")] = QStringLiteral("2026-07-12T12:00:02.000000000+08:00");
        third[QStringLiteral("timestamp")] = QStringLiteral("07-12 12:00:02");
        third[QStringLiteral("message")] = QStringLiteral("third event");

        QVariantMap older;
        older[QStringLiteral("rawTimestamp")] = QStringLiteral("2026-07-12T11:59:59.000000000+08:00");
        older[QStringLiteral("timestamp")] = QStringLiteral("07-12 11:59:59");
        older[QStringLiteral("message")] = QStringLiteral("older event");

        // 首次推送含较新日志，第二次推送含重复和更旧日志
        controller.setRunningStatus({}, {first, second});
        controller.setRunningStatus({}, {older, second, third});

        const QVariantList entries = controller.logEntries();
        // 检查仅追加更新的日志，不重复
        QCOMPARE(entries.size(), 3);
        QCOMPARE(entries.at(0).toMap().value(QStringLiteral("message")).toString(), QStringLiteral("first event"));
        QCOMPARE(entries.at(1).toMap().value(QStringLiteral("message")).toString(), QStringLiteral("second event"));
        QCOMPARE(entries.at(2).toMap().value(QStringLiteral("message")).toString(), QStringLiteral("third event"));
    }

    /// 目标：单实例运行事件日志最多保留最新 200 条，超过后删除最旧日志
    void vpnControllerKeepsOnlyLatestTwoHundredRuntimeLogs() {
        VpnController controller(QStringLiteral("inst-logs"), nullptr, nullptr, this);

        // 准备测试数据：构造 205 条日志
        QVariantList entries;
        for (int i = 0; i < 205; ++i) {
            QVariantMap entry;
            entry[QStringLiteral("rawTimestamp")] = QStringLiteral("2026-07-12T12:%1:%2.000000000+08:00")
                .arg(i / 60, 2, 10, QLatin1Char('0'))
                .arg(i % 60, 2, 10, QLatin1Char('0'));
            entry[QStringLiteral("timestamp")] = QStringLiteral("07-12 12:%1:%2")
                .arg(i / 60, 2, 10, QLatin1Char('0'))
                .arg(i % 60, 2, 10, QLatin1Char('0'));
            entry[QStringLiteral("message")] = QStringLiteral("event %1").arg(i);
            entries.append(entry);
        }

        controller.setRunningStatus({}, entries);

        const QVariantList cached = controller.logEntries();
        // 检查最多保留 200 条，最旧的 5 条已被裁剪
        QCOMPARE(cached.size(), 200);
        QCOMPARE(cached.first().toMap().value(QStringLiteral("message")).toString(), QStringLiteral("event 5"));
        QCOMPARE(cached.last().toMap().value(QStringLiteral("message")).toString(), QStringLiteral("event 204"));
    }

    /// 目标：VpnRuntimeService 刷新 RuntimeLogModel 时使用 controller 的累计日志缓存，而非本次 daemon 返回窗口
    void vpnRuntimeServiceRefreshesRuntimeLogModelFromCachedLogs() {
        insertConfig(QStringLiteral("inst-logs"), QStringLiteral("日志配置"));

        DaemonClient client;
        DaemonApi api(&client, this);
        StatusMonitor monitor(this);
        VpnRuntimeService service(&client, &api, m_repo.get(), &monitor, this);
        service.setActiveInstanceName(QStringLiteral("inst-logs"));

        // 准备测试数据：构造不同批次的日志
        QVariantMap first;
        first[QStringLiteral("rawTimestamp")] = QStringLiteral("2026-07-12T12:00:00.000000000+08:00");
        first[QStringLiteral("timestamp")] = QStringLiteral("07-12 12:00:00");
        first[QStringLiteral("message")] = QStringLiteral("first event");

        QVariantMap second;
        second[QStringLiteral("rawTimestamp")] = QStringLiteral("2026-07-12T12:00:01.000000000+08:00");
        second[QStringLiteral("timestamp")] = QStringLiteral("07-12 12:00:01");
        second[QStringLiteral("message")] = QStringLiteral("second event");

        QVariantMap third;
        third[QStringLiteral("rawTimestamp")] = QStringLiteral("2026-07-12T12:00:02.000000000+08:00");
        third[QStringLiteral("timestamp")] = QStringLiteral("07-12 12:00:02");
        third[QStringLiteral("message")] = QStringLiteral("third event");

        // 分两次推送，第二次与第一次有重叠；服务缓存累计日志并刷新展示模型
        service.onInstanceInfoParsed(QStringLiteral("inst-logs"), {}, {first, second});
        service.onInstanceInfoParsed(QStringLiteral("inst-logs"), {}, {second, third});

        // 检查 RuntimeLogModel 包含全部 3 条日志（不重复）
        QCOMPARE(service.runtimeLogModel()->rowCount(), 3);
        QCOMPARE(service.runtimeLogModel()->data(service.runtimeLogModel()->index(0, 0), RuntimeLogModel::MessageRole).toString(),
                 QStringLiteral("first event"));
        QCOMPARE(service.runtimeLogModel()->data(service.runtimeLogModel()->index(1, 0), RuntimeLogModel::MessageRole).toString(),
                 QStringLiteral("second event"));
        QCOMPARE(service.runtimeLogModel()->data(service.runtimeLogModel()->index(2, 0), RuntimeLogModel::MessageRole).toString(),
                 QStringLiteral("third event"));
    }

    /// 目标：VpnRuntimeService::exportLog 必须是 QML 可调用的（Q_INVOKABLE）
    /// 回归：曾因漏标 Q_INVOKABLE 导致 QML 中 exportLog 绑定失败、点击导出日志无响应
    void vpnRuntimeServiceExportLogIsQmlInvokable() {
        insertConfig(QStringLiteral("inst-logs"), QStringLiteral("日志配置"));

        DaemonClient client;
        DaemonApi api(&client, this);
        StatusMonitor monitor(this);
        VpnRuntimeService service(&client, &api, m_repo.get(), &monitor, this);

        // QML 可调用方法必须出现在 metaobject 中（Q_INVOKABLE 或 slot）
        const int methodIndex = service.metaObject()->indexOfMethod("exportLog(QString)");
        QVERIFY(methodIndex >= 0);
        const QMetaMethod method = service.metaObject()->method(methodIndex);
        QVERIFY(method.isValid());
        QVERIFY(method.methodType() == QMetaMethod::Method);
    }

    /// 目标：StatusMonitor 应完整解析 daemon events 数组，不应在 20 条处截断
    void statusMonitorParsesMoreThanTwentyEvents() {
        // 准备测试数据：构造 25 条事件
        QJsonArray events;
        for (int i = 0; i < 25; ++i) {
            const QJsonObject event{
                {QStringLiteral("time"), QStringLiteral("2026-07-12T15:40:%1").arg(i, 2, 10, QLatin1Char('0'))},
                {QStringLiteral("event"), QJsonObject{
                    {QStringLiteral("Connecting"), QStringLiteral("tcp://node-%1.example:11010").arg(i)}
                }}
            };
            events.append(QString::fromUtf8(QJsonDocument(event).toJson(QJsonDocument::Compact)));
        }

        const QJsonObject runtimeJson{{QStringLiteral("events"), events}};
        const QString encoded = QString::fromUtf8(QJsonDocument(runtimeJson)
            .toJson(QJsonDocument::Compact).toBase64());
        const QJsonObject response{{QStringLiteral("instances"), QJsonArray{
            QJsonObject{{QStringLiteral("key"), QStringLiteral("inst-logs")},
                        {QStringLiteral("value"), encoded}}
        }}};

        StatusMonitor monitor;
        QSignalSpy parsedSpy(&monitor, &StatusMonitor::instanceInfoParsed);

        monitor.processNetworkInfos(response);

        QTRY_COMPARE_WITH_TIMEOUT(parsedSpy.count(), 1, 3000);
        const QVariantList args = parsedSpy.takeFirst();
        QCOMPARE(args.at(0).toString(), QStringLiteral("inst-logs"));
        const QVariantList parsedLogs = args.at(2).toList();
        // 检查解析出全部 25 条日志
        QCOMPARE(parsedLogs.size(), 25);
        QCOMPARE(parsedLogs.first().toMap().value(QStringLiteral("rawTimestamp")).toString(),
                 QStringLiteral("2026-07-12T15:40:24"));
        QCOMPARE(parsedLogs.first().toMap().value(QStringLiteral("message")).toString(),
                 QStringLiteral("正在连接: tcp://node-24.example:11010"));
        QCOMPARE(parsedLogs.last().toMap().value(QStringLiteral("message")).toString(),
                 QStringLiteral("正在连接: tcp://node-0.example:11010"));
    }

    /// 目标：解析 daemon 节点信息时不应把 Base64 解码后的原始 JSON 输出到终端
    void statusMonitorDoesNotPrintDecodedNetworkInfo() {
        const QJsonObject runtimeJson{
            {QStringLiteral("routes"), QJsonArray{}},
            {QStringLiteral("events"), QJsonArray{}}
        };
        const QString encoded = QString::fromUtf8(QJsonDocument(runtimeJson)
            .toJson(QJsonDocument::Compact).toBase64());
        const QJsonObject response{{QStringLiteral("instances"), QJsonArray{
            QJsonObject{{QStringLiteral("key"), QStringLiteral("inst-logs")},
                        {QStringLiteral("value"), encoded}}
        }}};

        // 准备截获日志输出
        QStringList capturedMessages;
        {
            QMutexLocker locker(&g_capturedMessagesMutex);
            g_capturedMessages = &capturedMessages;
        }
        QtMessageHandler previousHandler = qInstallMessageHandler(captureInfoMessages);

        StatusMonitor monitor;
        QSignalSpy parsedSpy(&monitor, &StatusMonitor::instanceInfoParsed);
        monitor.processNetworkInfos(response);
        QTRY_COMPARE_WITH_TIMEOUT(parsedSpy.count(), 1, 3000);

        qInstallMessageHandler(previousHandler);
        {
            QMutexLocker locker(&g_capturedMessagesMutex);
            g_capturedMessages = nullptr;
        }

        // 检查捕获的消息中不包含原始 JSON
        for (const QString &message : capturedMessages) {
            QVERIFY2(!message.contains(QStringLiteral("\"events\"")),
                     qPrintable(QStringLiteral("unexpected decoded network info output: %1").arg(message)));
        }
    }

    /// 目标：页面级 ViewModel 作为 QML 后端命令入口，初始状态为空且显示编辑区
    void networkPageViewModelInitialState() {
        NetworkPageViewModel vm(nullptr, nullptr, nullptr, nullptr, this);

        // 检查初始状态：无当前实例、不运行、非安全模式、显示编辑区、不显示运行状态
        QVERIFY(vm.currentInstanceName().isEmpty());
        QVERIFY(!vm.currentInstanceRunning());
        QVERIFY(!vm.currentInstanceSecureMode());
        QVERIFY(vm.showEditor());
        QVERIFY(!vm.showRuntimeStatus());
    }

    /// 目标：选中外部实例时不加载编辑器（避免仓库无此配置报错），消失后 clearSelection 清空选中
    void networkPageViewModel_externalInstanceSelectAndClear() {
        insertConfig(QStringLiteral("inst-1"), QStringLiteral("配置1"));

        ConfigCommandService commandService(m_repo.get(), this);
        ConfigListModel model(&commandService, nullptr, this);
        ConfigEditorViewModel editor(&commandService, this);
        NetworkPageViewModel vm(&model, &editor, nullptr, nullptr, this);

        // 先选中本地配置，编辑器加载
        vm.selectConfig(QStringLiteral("inst-1"));
        QCOMPARE(vm.currentInstanceName(), QStringLiteral("inst-1"));
        QCOMPARE(editor.currentInstanceName(), QStringLiteral("inst-1"));

        // 外部实例出现在列表中
        model.onExternalInstancesChanged(QStringList{QStringLiteral("ext-a")});
        QVERIFY(model.isExternal(QStringLiteral("ext-a")));

        // 选中外部实例：跳过编辑器加载，编辑器保持上一个本地配置不变
        vm.selectConfig(QStringLiteral("ext-a"));
        QCOMPARE(vm.currentInstanceName(), QStringLiteral("ext-a"));
        QCOMPARE(editor.currentInstanceName(), QStringLiteral("inst-1"));

        // 外部实例消失后清空选中（AppServices 接线触发，此处直接调用验证行为）
        model.onExternalInstancesChanged({});
        vm.clearSelection();
        QVERIFY(vm.currentInstanceName().isEmpty());
        QVERIFY(!vm.currentInstanceRunning());
    }

    /// 目标：currentInstanceSecureMode 随编辑器加载配置的安全模式派生（凭据能力前置条件）
    void networkPageViewModelTracksSecureMode() {
        insertConfig(QStringLiteral("inst-sec"), QStringLiteral("安全配置"));

        ConfigCommandService commandService(m_repo.get(), this);
        ConfigEditorViewModel editor(&commandService, this);
        editor.loadConfig(QStringLiteral("inst-sec"));
        QCOMPARE(editor.currentInstanceName(), QStringLiteral("inst-sec"));
        QVERIFY(!editor.secureModeEnabled());

        NetworkPageViewModel vm(nullptr, &editor, nullptr, nullptr, this);
        QVERIFY(!vm.currentInstanceSecureMode());

        // 编辑器打开安全模式开关 → ViewModel 同步发射变化信号
        QSignalSpy secureSpy(&vm, &NetworkPageViewModel::currentInstanceSecureModeChanged);
        editor.setSecureModeEnabled(true);
        QCOMPARE(vm.currentInstanceSecureMode(), true);
        QCOMPARE(secureSpy.count(), 1);

        // 关闭 → 同步恢复 false
        editor.setSecureModeEnabled(false);
        QCOMPARE(vm.currentInstanceSecureMode(), false);
        QCOMPARE(secureSpy.count(), 2);
    }

    /// 目标：正在启动或停止的配置禁止删除
    void deleteStartingOrStoppingConfig_rejected() {
        // 准备测试数据
        insertConfig(QStringLiteral("inst-3"), QStringLiteral("配置3"));

        ConfigCommandService commandService(m_repo.get(), this);
        ConfigListModel model(&commandService, nullptr, this);

        // 模拟 daemon 报告该配置正在启动中
        model.onRunningStateChanged(QStringLiteral("inst-3"),
                                    ConfigRunState::Starting);

        // 尝试删除，应被拒绝
        QVERIFY(!model.deleteConfig(QStringLiteral("inst-3")));
        QCOMPARE(model.rowCount(), 1);
        QVERIFY(m_repo->exists(QStringLiteral("inst-3")));

        // 模拟 daemon 报告该配置正在停止中
        model.onRunningStateChanged(QStringLiteral("inst-3"),
                                    ConfigRunState::Stopping);

        // 再次尝试删除，仍应被拒绝
        QVERIFY(!model.deleteConfig(QStringLiteral("inst-3")));
        QCOMPARE(model.rowCount(), 1);
        QVERIFY(m_repo->exists(QStringLiteral("inst-3")));

        // 变为 Unstarted 后可以删除
        model.onRunningStateChanged(QStringLiteral("inst-3"),
                                    ConfigRunState::Stopped);
        QVERIFY(model.deleteConfig(QStringLiteral("inst-3")));
        // 检查已删除
        QCOMPARE(model.rowCount(), 0);
    }

    /// 目标：外部实例条目按传入顺序追加在本地配置之后，各 role 正确，消失后移除
    void externalInstances_appendedAfterLocalAndRemovedWhenGone() {
        insertConfig(QStringLiteral("inst-1"), QStringLiteral("配置1"));

        ConfigCommandService commandService(m_repo.get(), this);
        ConfigListModel model(&commandService, nullptr, this);
        QCOMPARE(model.rowCount(), 1);

        // 追加两个外部实例（模拟 VpnRuntimeService 心跳发现 daemon 中存在本地没有的实例）
        model.onExternalInstancesChanged(QStringList{QStringLiteral("ext-a"), QStringLiteral("ext-b")});

        // 总数 = 本地 + 外部，本地在前、外部在后
        QCOMPARE(model.rowCount(), 3);
        QCOMPARE(model.data(model.index(0), ConfigListModel::InstanceNameRole).toString(), QStringLiteral("inst-1"));
        QCOMPARE(model.data(model.index(0), ConfigListModel::IsExternalRole).toBool(), false);

        // 外部条目：displayName = instance_name，始终运行中，isExternal = true
        const QModelIndex extA = model.index(1);
        QCOMPARE(model.data(extA, ConfigListModel::InstanceNameRole).toString(), QStringLiteral("ext-a"));
        QCOMPARE(model.data(extA, ConfigListModel::DisplayNameRole).toString(), QStringLiteral("ext-a"));
        QCOMPARE(model.data(extA, ConfigListModel::RunningRole).toBool(), true);
        QCOMPARE(model.data(extA, ConfigListModel::IsExternalRole).toBool(), true);

        const QModelIndex extB = model.index(2);
        QCOMPARE(model.data(extB, ConfigListModel::InstanceNameRole).toString(), QStringLiteral("ext-b"));
        QCOMPARE(model.data(extB, ConfigListModel::DisplayNameRole).toString(), QStringLiteral("ext-b"));
        QCOMPARE(model.data(extB, ConfigListModel::RunningRole).toBool(), true);
        QCOMPARE(model.data(extB, ConfigListModel::IsExternalRole).toBool(), true);

        // isExternal 查询
        QVERIFY(model.isExternal(QStringLiteral("ext-a")));
        QVERIFY(!model.isExternal(QStringLiteral("inst-1")));

        // 部分外部实例从 daemon 消失后仅移除对应条目
        model.onExternalInstancesChanged(QStringList{QStringLiteral("ext-b")});
        QCOMPARE(model.rowCount(), 2);
        QVERIFY(!model.isExternal(QStringLiteral("ext-a")));
        QVERIFY(model.isExternal(QStringLiteral("ext-b")));

        // 全部消失后恢复为仅本地配置
        model.onExternalInstancesChanged({});
        QCOMPARE(model.rowCount(), 1);
        QVERIFY(!model.isExternal(QStringLiteral("ext-b")));
    }

    /// 目标：新建配置成功后发射 configCreated（供外部同步本地 controller，保持与数据库配置集合一致）
    void createNewConfig_emitsConfigCreated() {
        ConfigCommandService commandService(m_repo.get(), this);
        ConfigListModel model(&commandService, nullptr, this);

        QSignalSpy spy(&model, &ConfigListModel::configCreated);
        const QString instanceName = model.createNewConfig();
        QVERIFY(!instanceName.isEmpty());
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toString(), instanceName);
        QCOMPARE(model.rowCount(), 1);
    }
};

QTEST_MAIN(TestConfigListModel)
#include "tst_config_list_model.moc"
