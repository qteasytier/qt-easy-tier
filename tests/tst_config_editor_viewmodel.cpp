/**
 * @file tst_config_editor_viewmodel.cpp
 * @brief 配置编辑器 ViewModel 的单元测试。测试内容：即时保存（防抖自动落库）、连续编辑合并、切换/清空前的刷写、手动 flush 入口。
 *
 * 覆盖：
 * - 字段修改后，超过防抖间隔自动持久化到仓库
 * - 防抖窗口内的连续编辑合并为最终值，窗口内不落库
 * - loadConfig 切换实例前刷写待保存修改
 * - clear 清空编辑器前刷写待保存修改
 * - flushAutoSave 手动立即刷写
 */
#include <QSet>
#include <QSignalSpy>
#include <QTest>
#include <QTemporaryDir>
#include <QUuid>
#include <memory>

#include "core/config/ConfigCommandService.h"
#include "sqlite_repository/DatabaseConnection.h"
#include "sqlite_repository/NetworkConfigRepository.h"
#include "core/viewmodels/ConfigEditorViewModel.h"
#include "config/NetworkConf.h"

namespace {
/// 防抖间隔为 300ms，等待时留出余量保证定时器必然触发
constexpr int kWaitForAutoSaveMs = 400;
}

class TestConfigEditorViewModel : public QObject {
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

    /// 目标：修改字段后，超过防抖间隔自动持久化到仓库，无需手动保存
    void modifyField_autoSavesAfterDebounce() {
        insertConfig(QStringLiteral("inst-auto-1"), QStringLiteral("自动保存测试"));

        ConfigCommandService commandService(m_repo.get(), this);
        ConfigEditorViewModel editor(&commandService, this);
        editor.loadConfig(QStringLiteral("inst-auto-1"));

        // 修改主机名：防抖窗口内尚未落库，仓库中仍是旧值
        editor.setHostname(QStringLiteral("new-host"));
        QVERIFY(editor.hasUnsavedChanges());
        {
            const auto loaded = m_repo->load(QStringLiteral("inst-auto-1"));
            QVERIFY(loaded.has_value());
            QCOMPARE(loaded->hostname, QString());
        }

        // 等待超过防抖间隔，触发自动保存
        QTest::qWait(kWaitForAutoSaveMs);

        const auto loaded = m_repo->load(QStringLiteral("inst-auto-1"));
        QVERIFY(loaded.has_value());
        QCOMPARE(loaded->hostname, QStringLiteral("new-host"));
        QVERIFY(!editor.hasUnsavedChanges());
    }

    /// 目标：防抖窗口内的连续编辑合并为单次保存，最终持久化最后一次的值
    void consecutiveEdits_debouncedToFinalValue() {
        insertConfig(QStringLiteral("inst-auto-2"), QStringLiteral("防抖合并测试"));

        ConfigCommandService commandService(m_repo.get(), this);
        ConfigEditorViewModel editor(&commandService, this);
        editor.loadConfig(QStringLiteral("inst-auto-2"));

        // 连续修改多次，防抖窗口内不落库
        editor.setHostname(QStringLiteral("h1"));
        editor.setHostname(QStringLiteral("h2"));
        editor.setHostname(QStringLiteral("h3"));
        {
            const auto loaded = m_repo->load(QStringLiteral("inst-auto-2"));
            QVERIFY(loaded.has_value());
            QCOMPARE(loaded->hostname, QString());
        }

        QTest::qWait(kWaitForAutoSaveMs);

        // 最终持久化的应为最后一次修改的值
        const auto loaded = m_repo->load(QStringLiteral("inst-auto-2"));
        QVERIFY(loaded.has_value());
        QCOMPARE(loaded->hostname, QStringLiteral("h3"));
    }

    /// 目标：切换配置前，防抖窗口内未触发的修改应被立即刷写保存
    void loadConfig_flushesPendingEditsBeforeSwitching() {
        insertConfig(QStringLiteral("inst-a"), QStringLiteral("配置A"));
        insertConfig(QStringLiteral("inst-b"), QStringLiteral("配置B"));

        ConfigCommandService commandService(m_repo.get(), this);
        ConfigEditorViewModel editor(&commandService, this);
        editor.loadConfig(QStringLiteral("inst-a"));

        // 修改后不等待防抖，立即切换到另一配置
        editor.setNetworkName(QStringLiteral("dirty-a"));
        editor.loadConfig(QStringLiteral("inst-b"));

        // 配置A 的修改应已刷写保存
        const auto loadedA = m_repo->load(QStringLiteral("inst-a"));
        QVERIFY(loadedA.has_value());
        QCOMPARE(loadedA->networkName, QStringLiteral("dirty-a"));
        // 编辑器当前应为配置B
        QCOMPARE(editor.currentInstanceName(), QStringLiteral("inst-b"));
    }

    /// 目标：清空编辑器前，防抖窗口内未触发的修改应被立即刷写保存
    void clear_flushesPendingEdits() {
        insertConfig(QStringLiteral("inst-a"), QStringLiteral("配置A"));

        ConfigCommandService commandService(m_repo.get(), this);
        ConfigEditorViewModel editor(&commandService, this);
        editor.loadConfig(QStringLiteral("inst-a"));

        // 修改后不等待防抖，立即清空编辑器
        editor.setNetworkName(QStringLiteral("dirty-a"));
        editor.clear();

        // 配置A 的修改应已刷写保存，编辑器回到空状态
        const auto loadedA = m_repo->load(QStringLiteral("inst-a"));
        QVERIFY(loadedA.has_value());
        QCOMPARE(loadedA->networkName, QStringLiteral("dirty-a"));
        QVERIFY(editor.currentInstanceName().isEmpty());
        QVERIFY(!editor.hasUnsavedChanges());
    }

    /// 目标：flushAutoSave 可手动立即持久化防抖窗口内的修改
    void flushAutoSave_persistsPendingEditsImmediately() {
        insertConfig(QStringLiteral("inst-a"), QStringLiteral("配置A"));

        ConfigCommandService commandService(m_repo.get(), this);
        ConfigEditorViewModel editor(&commandService, this);
        editor.loadConfig(QStringLiteral("inst-a"));

        editor.setNetworkName(QStringLiteral("dirty-net"));
        QVERIFY(editor.hasUnsavedChanges());

        editor.flushAutoSave();

        const auto loadedA = m_repo->load(QStringLiteral("inst-a"));
        QVERIFY(loadedA.has_value());
        QCOMPARE(loadedA->networkName, QStringLiteral("dirty-net"));
        QVERIFY(!editor.hasUnsavedChanges());
    }

    /// 目标：resetToDefaults 将实例全部网络设置恢复默认并保留显示名称
    void resetToDefaults_restoresDefaultsKeepsDisplayName() {
        // 准备测试数据：带非默认字段的配置
        NetworkConf cfg(QStringLiteral("inst-reset"));
        cfg.displayName = QStringLiteral("保留名称");
        cfg.hostname = QStringLiteral("custom-host");
        cfg.mtu = 1200;
        cfg.enableExitNode = true;
        QVERIFY(m_repo->save(cfg));

        ConfigCommandService commandService(m_repo.get(), this);
        ConfigEditorViewModel editor(&commandService, this);
        editor.loadConfig(QStringLiteral("inst-reset"));

        // 执行重置
        QVERIFY(editor.resetToDefaults());

        // 重置后编辑器回到默认状态，无未保存修改
        QVERIFY(!editor.hasUnsavedChanges());
        QCOMPARE(editor.currentInstanceName(), QStringLiteral("inst-reset"));
        QVERIFY(editor.errorMessages().isEmpty());

        // 仓库中该实例：网络设置恢复默认，显示名称保留
        const auto loaded = m_repo->load(QStringLiteral("inst-reset"));
        QVERIFY(loaded.has_value());
        QCOMPARE(loaded->displayName, QStringLiteral("保留名称"));
        QCOMPARE(loaded->hostname, QString());
        QCOMPARE(loaded->mtu, 1380);
        QVERIFY(!loaded->enableExitNode);
    }

    /// 目标：无当前实例名时 resetToDefaults 返回 false 并写入错误消息
    void resetToDefaults_withoutInstance_returnsFalse() {
        ConfigCommandService commandService(m_repo.get(), this);
        ConfigEditorViewModel editor(&commandService, this);

        QVERIFY(!editor.resetToDefaults());
        QVERIFY(!editor.errorMessages().isEmpty());
    }

    /// 目标：setCredentialFile 直接接受任意路径（不再校验 .json 后缀）
    void setCredentialFile_acceptsAnyPath() {
        insertConfig(QStringLiteral("inst-cred"), QStringLiteral("凭据测试"));

        ConfigCommandService commandService(m_repo.get(), this);
        ConfigEditorViewModel editor(&commandService, this);
        editor.loadConfig(QStringLiteral("inst-cred"));

        // 非 .json 后缀也接受
        editor.setCredentialFile(QStringLiteral("/path/to/key.txt"));
        QCOMPARE(editor.credentialFile(), QStringLiteral("/path/to/key.txt"));
        QVERIFY(editor.errorMessages().isEmpty());

        // 空值允许（清空）
        editor.setCredentialFile(QString());
        QCOMPARE(editor.credentialFile(), QString());
    }

    /// 目标：toLocalFilePath 将 file:// URL 转为本地路径，普通路径原样返回，空输入返回空
    void toLocalFilePath_convertsUrlAndKeepsLocalPath() {
        ConfigCommandService commandService(m_repo.get(), this);
        ConfigEditorViewModel editor(&commandService, this);

        // file:// URL → 本地路径
        QCOMPARE(editor.toLocalFilePath(QStringLiteral("file:///home/user/cred.json")),
                 QStringLiteral("/home/user/cred.json"));
        // 普通本地路径原样返回
        QCOMPARE(editor.toLocalFilePath(QStringLiteral("/home/user/cred.json")),
                 QStringLiteral("/home/user/cred.json"));
        // 空输入返回空
        QCOMPARE(editor.toLocalFilePath(QString()), QString());
    }

    /// 目标：外部重命名同步当前编辑配置的显示名称，且不标记 dirty、不触发自动保存
    void syncDisplayName_updatesCurrentConfigWithoutDirtying() {
        insertConfig(QStringLiteral("inst-ren"), QStringLiteral("旧名称"));

        ConfigCommandService commandService(m_repo.get(), this);
        ConfigEditorViewModel editor(&commandService, this);
        editor.loadConfig(QStringLiteral("inst-ren"));

        QSignalSpy displaySpy(&editor, &ConfigEditorViewModel::displayNameChanged);
        editor.syncDisplayName(QStringLiteral("inst-ren"), QStringLiteral("新名称"));

        // 内存快照已同步，且不标记未保存
        QCOMPARE(editor.displayName(), QStringLiteral("新名称"));
        QVERIFY(!editor.hasUnsavedChanges());
        QCOMPARE(displaySpy.count(), 1);

        // 修改其他字段触发自动保存，显示名称不被旧快照覆盖
        editor.setHostname(QStringLiteral("new-host"));
        QVERIFY(editor.hasUnsavedChanges());
        QTest::qWait(kWaitForAutoSaveMs);

        const auto loaded = m_repo->load(QStringLiteral("inst-ren"));
        QVERIFY(loaded.has_value());
        QCOMPARE(loaded->displayName, QStringLiteral("新名称"));
        QCOMPARE(loaded->hostname, QStringLiteral("new-host"));
    }

    /// 目标：外部重命名其他配置时，当前编辑内容保持不变
    void syncDisplayName_ignoresNonCurrentConfig() {
        insertConfig(QStringLiteral("inst-cur"), QStringLiteral("当前配置"));
        insertConfig(QStringLiteral("inst-other"), QStringLiteral("其他配置"));

        ConfigCommandService commandService(m_repo.get(), this);
        ConfigEditorViewModel editor(&commandService, this);
        editor.loadConfig(QStringLiteral("inst-cur"));

        editor.syncDisplayName(QStringLiteral("inst-other"), QStringLiteral("其他配置新名"));
        QCOMPARE(editor.displayName(), QStringLiteral("当前配置"));
        QCOMPARE(editor.currentInstanceName(), QStringLiteral("inst-cur"));
    }

    /// 目标：discardAndClear 丢弃待保存修改，不把已删除配置重新写回仓库
    void discardAndClear_doesNotPersistPendingEdits() {
        insertConfig(QStringLiteral("inst-del"), QStringLiteral("待删除配置"));

        ConfigCommandService commandService(m_repo.get(), this);
        ConfigEditorViewModel editor(&commandService, this);
        editor.loadConfig(QStringLiteral("inst-del"));

        // 修改字段，产生未保存修改
        editor.setHostname(QStringLiteral("pending-edit"));
        QVERIFY(editor.hasUnsavedChanges());

        editor.discardAndClear();
        QVERIFY(editor.currentInstanceName().isEmpty());
        QVERIFY(!editor.hasUnsavedChanges());
        QVERIFY(editor.errorMessages().isEmpty());

        // 仓库中的原配置未被 pending 修改覆盖
        const auto loaded = m_repo->load(QStringLiteral("inst-del"));
        QVERIFY(loaded.has_value());
        QCOMPARE(loaded->hostname, QString());
    }

    /// 目标：formSections 元数据结构完整——每个卡片归属合法标签页，
    /// 每个字段 key 均对应本类存在的 Q_PROPERTY，控件类型合法，
    /// comboBox 必带非空 options，spinBox 必带 from/to
    void formSections_metadataIsCompleteAndKeysResolve() {
        ConfigCommandService commandService(m_repo.get(), this);
        ConfigEditorViewModel editor(&commandService, this);

        const QVariantList sections = editor.formSections();
        QVERIFY(sections.size() >= 6);

        const QStringList validTabs = { QStringLiteral("basic"), QStringLiteral("advanced") };
        const QStringList validTypes = {
            QStringLiteral("switch"), QStringLiteral("textField"), QStringLiteral("password"),
            QStringLiteral("comboBox"), QStringLiteral("spinBox"),
            QStringLiteral("stringList"), QStringLiteral("serverList"),
            QStringLiteral("proxyNetworkList"), QStringLiteral("filePath"), QStringLiteral("keyActions"),
        };

        const QMetaObject *meta = editor.metaObject();
        int boundFieldCount = 0;
        QSet<QString> seenKeys;
        QVariantList defaultProtocolOptions;
        QString networkNamePlaceholder;
        for (const auto &sectionVar : sections) {
            const QVariantMap section = sectionVar.toMap();
            QVERIFY(validTabs.contains(section.value(QStringLiteral("tab")).toString()));
            QVERIFY(!section.value(QStringLiteral("cardKey")).toString().isEmpty());
            const QVariantList fields = section.value(QStringLiteral("fields")).toList();
            QVERIFY(!fields.isEmpty());

            for (const auto &fieldVar : fields) {
                const QVariantMap field = fieldVar.toMap();
                const QString type = field.value(QStringLiteral("type")).toString();
                QVERIFY2(validTypes.contains(type), qPrintable(type));
                const QString key = field.value(QStringLiteral("key")).toString();

                if (type == QStringLiteral("keyActions")) {
                    // 动作行无数据绑定，无 key
                    QVERIFY(key.isEmpty());
                    continue;
                }

                QVERIFY2(!key.isEmpty(), qPrintable(type));
                QVERIFY2(!seenKeys.contains(key), qPrintable(QStringLiteral("重复 key: %1").arg(key)));
                seenKeys.insert(key);
                // key 必须能反射到本类的 Q_PROPERTY（fieldValue/setFieldValue 的前提）
                QVERIFY2(meta->indexOfProperty(key.toUtf8().constData()) >= 0,
                         qPrintable(QStringLiteral("key 无对应属性: %1").arg(key)));
                QVERIFY(editor.fieldValue(key).isValid());

                if (type == QStringLiteral("comboBox"))
                    QVERIFY(field.value(QStringLiteral("options")).toList().size() >= 2);
                if (key == QStringLiteral("defaultProtocol"))
                    defaultProtocolOptions = field.value(QStringLiteral("options")).toList();
                if (key == QStringLiteral("networkName"))
                    networkNamePlaceholder = field.value(QStringLiteral("placeholder")).toString();
                if (type == QStringLiteral("spinBox")) {
                    QVERIFY(field.value(QStringLiteral("from")).toInt()
                            < field.value(QStringLiteral("to")).toInt());
                }
                ++boundFieldCount;
            }
        }

        // 47 个带数据绑定的字段（48 个配置字段中 displayName 不进表单）
        QCOMPARE(boundFieldCount, 47);

        // 默认连接协议：不再提供"不指定"空值选项，首项即 tcp（存量空值配置的显示回退项）
        QVERIFY(!defaultProtocolOptions.isEmpty());
        for (const auto &optVar : defaultProtocolOptions)
            QVERIFY(!optVar.toMap().value(QStringLiteral("value")).toString().isEmpty());
        QCOMPARE(defaultProtocolOptions.first().toMap().value(QStringLiteral("value")).toString(),
                 QStringLiteral("tcp"));

        // 网络名称：必须带默认值提示（TOML 侧空值回退 default）
        QVERIFY(!networkNamePlaceholder.isEmpty());
    }

    /// 目标：setFieldValue 反射写回等价于具名 setter——值生效、自动保存、
    /// 未知 key 返回 false
    void setFieldValue_writesThroughAndAutoSaves() {
        insertConfig(QStringLiteral("inst-reflect"), QStringLiteral("反射写入测试"));

        ConfigCommandService commandService(m_repo.get(), this);
        ConfigEditorViewModel editor(&commandService, this);
        editor.loadConfig(QStringLiteral("inst-reflect"));

        QVERIFY(editor.setFieldValue(QStringLiteral("hostname"), QStringLiteral("via-reflect")));
        QCOMPARE(editor.hostname(), QStringLiteral("via-reflect"));

        QVERIFY(editor.setFieldValue(QStringLiteral("mtu"), 1200));
        QCOMPARE(editor.mtu(), 1200);

        QVERIFY(editor.setFieldValue(
            QStringLiteral("listenAddresses"),
            QVariantList{QStringLiteral("tcp://0.0.0.0:11010"), QStringLiteral("udp://0.0.0.0:11011")}));
        QCOMPARE(editor.listenAddresses().size(), 2);

        // 未知 key 与只读属性写入失败
        QVERIFY(!editor.setFieldValue(QStringLiteral("no_such_field"), 1));
        QVERIFY(!editor.setFieldValue(QStringLiteral("currentInstanceName"), QStringLiteral("x")));

        // 防抖自动保存后仓库中即为反射写入的值
        QTest::qWait(kWaitForAutoSaveMs);
        const auto loaded = m_repo->load(QStringLiteral("inst-reflect"));
        QVERIFY(loaded.has_value());
        QCOMPARE(loaded->hostname, QStringLiteral("via-reflect"));
        QCOMPARE(loaded->mtu, 1200);
    }
};

QTEST_MAIN(TestConfigEditorViewModel)
#include "tst_config_editor_viewmodel.moc"
