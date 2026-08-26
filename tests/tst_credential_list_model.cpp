/**
 * @file tst_credential_list_model.cpp
 * @brief 临时凭证列表 Model 单元测试
 *
 * 验证：
 * - setFromVariantList 正确反序列化 credentials 数组并填充全部角色
 * - expiry_unix 兼容数字与字符串两种 JSON 形态
 * - count 属性与 countChanged 信号在数量变化时正确发射
 * - 空列表/清空行为
 */
#include <QTest>
#include <QSignalSpy>
#include <QVariantList>

#include "app_service/credential/CredentialListModel.h"

class TestCredentialListModel : public QObject {
    Q_OBJECT

private slots:
    /// 测试目标: setFromVariantList 填充全部角色
    void variantListPopulatesAllRoles()
    {
        CredentialListModel model;
        QVariantList items;
        items.append(QVariantMap{
            {QStringLiteral("credential_id"), QStringLiteral("c1")},
            {QStringLiteral("groups"), QVariantList{QStringLiteral("ops")}},
            {QStringLiteral("allow_relay"), false},
            {QStringLiteral("expiry_unix"), 1786000000},
            {QStringLiteral("allowed_proxy_cidrs"), QVariantList{QStringLiteral("10.0.0.0/8")}},
            {QStringLiteral("reusable"), true},
            {QStringLiteral("public_key_fingerprint"), QStringLiteral("aa:bb")}});
        model.setFromVariantList(items);

        QCOMPARE(model.count(), 1);
        QCOMPARE(model.rowCount(), 1);

        const QModelIndex idx = model.index(0, 0);
        QCOMPARE(model.data(idx, CredentialListModel::CredentialIdRole).toString(),
                 QStringLiteral("c1"));
        QCOMPARE(model.data(idx, CredentialListModel::GroupsRole).toStringList().size(), 1);
        QCOMPARE(model.data(idx, CredentialListModel::GroupsRole).toStringList().at(0),
                 QStringLiteral("ops"));
        QCOMPARE(model.data(idx, CredentialListModel::AllowRelayRole).toBool(), false);
        QCOMPARE(model.data(idx, CredentialListModel::ExpiryUnixRole).toLongLong(),
                 qint64(1786000000));
        QCOMPARE(model.data(idx, CredentialListModel::AllowedProxyCidrsRole).toStringList().size(), 1);
        QCOMPARE(model.data(idx, CredentialListModel::AllowedProxyCidrsRole).toStringList().at(0),
                 QStringLiteral("10.0.0.0/8"));
        QCOMPARE(model.data(idx, CredentialListModel::ReusableRole).toBool(), true);
        QCOMPARE(model.data(idx, CredentialListModel::PublicKeyFingerprintRole).toString(),
                 QStringLiteral("aa:bb"));
    }

    /// 测试目标: expiry_unix 为字符串时也能正确解析（protobuf uint64 字符串形态）
    void stringExpiryParses()
    {
        CredentialListModel model;
        QVariantList items;
        items.append(QVariantMap{
            {QStringLiteral("credential_id"), QStringLiteral("c1")},
            {QStringLiteral("expiry_unix"), QStringLiteral("1786000123")}});
        model.setFromVariantList(items);

        QCOMPARE(model.count(), 1);
        QCOMPARE(model.data(model.index(0, 0), CredentialListModel::ExpiryUnixRole).toLongLong(),
                 qint64(1786000123));
    }

    /// 测试目标: count 与 countChanged 仅在数量变化时发射
    void countChangeIsEmitted()
    {
        CredentialListModel model;
        QSignalSpy spy(&model, &CredentialListModel::countChanged);
        QCOMPARE(model.count(), 0);

        // 首次注入 2 条：count 0 -> 2，发射一次
        QVariantList items;
        for (int i = 0; i < 2; ++i) {
            items.append(QVariantMap{
                {QStringLiteral("credential_id"), QStringLiteral("c%1").arg(i)},
                {QStringLiteral("expiry_unix"), 1786000000 + i}});
        }
        model.setFromVariantList(items);
        QCOMPARE(model.count(), 2);
        QCOMPARE(spy.count(), 1);

        // 注入同样数量的 2 条：数量不变，不发射
        model.setFromVariantList(items);
        QCOMPARE(model.count(), 2);
        QCOMPARE(spy.count(), 1);

        // 清空：count 2 -> 0，发射一次
        model.setFromVariantList({});
        QCOMPARE(model.count(), 0);
        QCOMPARE(spy.count(), 2);
    }

    /// 测试目标: 空列表清空模型，无效索引返回空 QVariant
    void emptyListClears()
    {
        CredentialListModel model;
        QVariantList items;
        items.append(QVariantMap{
            {QStringLiteral("credential_id"), QStringLiteral("c1")},
            {QStringLiteral("expiry_unix"), 1786000000}});
        model.setFromVariantList(items);
        QCOMPARE(model.count(), 1);

        model.setFromVariantList({});
        QCOMPARE(model.count(), 0);
        QVERIFY(model.data(model.index(0, 0), CredentialListModel::CredentialIdRole).isNull());
    }
};

QTEST_MAIN(TestCredentialListModel)
#include "tst_credential_list_model.moc"
