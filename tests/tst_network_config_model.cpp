/**
 * @file tst_network_config_model.cpp
 * @brief NetworkConfiguration 模型模块的单元测试。测试内容：Schema 默认值应用、setValue/value 读写、脏标记检测与重置、isValid 校验（空 instance_name/空 configId/合法配置）、toVariantMap/fromVariantMap 序列化往返。
 *
 * 覆盖：
 * - Schema 默认值正确应用
 * - setValue / value 读写正确
 * - 脏标记变更检测
 * - isValid 校验逻辑（空 instance_name、空 configId、合法配置）
 * - toVariantMap / fromVariantMap 序列化往返
 *
 * 注意: 此测试文件存在于 tests/ 目录但尚未在 tests/CMakeLists.txt 中注册。
 */
#include <QTest>
#include "core/model/NetworkConfiguration.h"

class TestNetworkConfigModel : public QObject {
    Q_OBJECT

private slots:
    /// 测试目标: 验证新建 NetworkConfiguration 自动填充 Schema 定义的默认值
    void defaultsAreApplied()
    {
        NetworkConfiguration cfg;
        // 检查默认值正确应用
        QCOMPARE(cfg.value("instance_name").toString(), QString("default"));
        QCOMPARE(cfg.value("enable_encryption").toBool(), true);
        QCOMPARE(cfg.value("mtu").toInt(), 1400);
    }

    /// 测试目标: 验证 setValue() 后 value() 立即返回新值
    void setAndGetValue()
    {
        NetworkConfiguration cfg;
        cfg.setValue("hostname", "node-a");
        // 检查设置后读取值一致
        QCOMPARE(cfg.value("hostname").toString(), QString("node-a"));
    }

    /// 测试目标: 验证修改字段后 dirty() 标记变为 true
    void dirtyAfterSetValue()
    {
        NetworkConfiguration cfg;
        // 检查初始 dirty 为 false
        QVERIFY(!cfg.dirty());
        cfg.setValue("hostname", "node-a");
        // 检查设置后 dirty 为 true
        QVERIFY(cfg.dirty());
    }

    /// 测试目标: 验证 markClean() 将 dirty() 重置为 false
    void markCleanResetsDirty()
    {
        NetworkConfiguration cfg;
        cfg.setValue("hostname", "node-a");
        QVERIFY(cfg.dirty());
        cfg.markClean();
        // 检查 markClean 后 dirty 重置
        QVERIFY(!cfg.dirty());
    }

    /// 测试目标: 验证 instance_name 为空时 isValid() 返回 false
    void validationFailsOnEmptyInstanceName()
    {
        NetworkConfiguration cfg;
        cfg.setValue("instance_name", "");
        // 检查空 instance_name 时校验失败
        QVERIFY(!cfg.isValid());
    }

    /// 测试目标: 验证 configId（构造参数）为空时 isValid() 返回 false
    void validationFailsOnEmptyConfigId()
    {
        NetworkConfiguration cfg(QString{});
        // 检查空 configId 时校验失败
        QVERIFY(!cfg.isValid());
    }

    /// 测试目标: 验证 configId 和 instance_name 均非空时 isValid() 返回 true
    void validConfigPassesValidation()
    {
        NetworkConfiguration cfg("my-instance");
        cfg.setValue("instance_name", "my-instance");
        // 检查合法配置校验通过
        QVERIFY(cfg.isValid());
    }

    /// 测试目标: 验证 toVariantMap → fromVariantMap 往返后字段值一致
    /// 未显式设置的字段应回退到 Schema 默认值而非丢失
    void variantMapRoundTrip()
    {
        // 准备测试数据：设置部分字段并标记干净
        NetworkConfiguration orig("roundtrip");
        orig.setValue("hostname", "host-a");
        orig.setValue("network_secret", "secret123");
        orig.markClean();

        NetworkConfiguration restored;
        restored.fromVariantMap(orig.toVariantMap());
        // 检查往返后显式设置的值一致
        QCOMPARE(restored.value("hostname").toString(), QString("host-a"));
        QCOMPARE(restored.value("network_secret").toString(), QString("secret123"));
        // 检查未设置的字段回退到默认值
        QCOMPARE(restored.value("instance_name").toString(), QString("default"));
    }
};

QTEST_MAIN(TestNetworkConfigModel)
#include "tst_network_config_model.moc"
