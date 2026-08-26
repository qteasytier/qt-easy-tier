/**
 * @file tst_x25519_key_helper.cpp
 * @brief X25519KeyHelper 模块单元测试。
 *
 * 覆盖：
 * - 随机私钥生成：标准 Base64（44 字符）、解码后恰好 32 字节
 * - 公钥派生：44 字符、同私钥多次推导结果一致
 * - easytier 官方示例密钥对回归：私钥 → 公钥与官方一致（验证算法完全对齐）
 * - 无效私钥拒绝：空串 / 长度不足 / 非法 Base64 / 31 字节等
 */
#include <QTest>
#include "core/config/X25519KeyHelper.h"

class TestX25519KeyHelper : public QObject
{
    Q_OBJECT

private slots:
    /// 测试目标: 生成的私钥是标准 Base64（44 字符），解码后恰好 32 字节
    void generatePrivateKeyIsValidBase64()
    {
        const QString key = X25519KeyHelper::generatePrivateKeyBase64();
        QVERIFY(!key.isEmpty());
        QCOMPARE(key.size(), 44);

        const QByteArray raw = QByteArray::fromBase64(key.toLatin1());
        QCOMPARE(raw.size(), 32);
    }

    /// 测试目标: 由生成私钥派生公钥为 44 字符，且多次推导结果一致
    void derivePublicKeyFromGenerated()
    {
        const QString priv = X25519KeyHelper::generatePrivateKeyBase64();
        QVERIFY(!priv.isEmpty());

        QString pub1;
        QString pub2;
        QVERIFY(X25519KeyHelper::derivePublicKeyBase64(priv, &pub1));
        QVERIFY(X25519KeyHelper::derivePublicKeyBase64(priv, &pub2));

        QCOMPARE(pub1.size(), 44);
        QCOMPARE(pub1, pub2);
    }

    /// 测试目标: easytier 官方示例密钥对，验证算法与 easytier 完全一致
    /// 私钥 4AQit0gGw2am6mO0nFXZ2pFeCJpKmaRr9L5Dksh7slM= → 公钥 Uu23K3mO3i/O2GAOi3gbiznIfYXttFX/XjqCv9FMQUA=
    void easytierExampleVector()
    {
        const QString priv = QStringLiteral("4AQit0gGw2am6mO0nFXZ2pFeCJpKmaRr9L5Dksh7slM=");
        const QString expectedPub = QStringLiteral("Uu23K3mO3i/O2GAOi3gbiznIfYXttFX/XjqCv9FMQUA=");

        QString pub;
        QVERIFY(X25519KeyHelper::derivePublicKeyBase64(priv, &pub));
        QCOMPARE(pub, expectedPub);
    }

    /// 测试目标: 无效私钥（空串 / 长度不足 / 非法 Base64 / 非 32 字节长度）应拒绝派生
    void invalidPrivateKeyRejected()
    {
        QString pub;

        // 空字符串
        QVERIFY(!X25519KeyHelper::derivePublicKeyBase64(QString(), &pub));

        // 长度不足（解码后不足 32 字节）
        QVERIFY(!X25519KeyHelper::derivePublicKeyBase64(QStringLiteral("short-key"), &pub));

        // 非法 Base64 字符
        QVERIFY(!X25519KeyHelper::derivePublicKeyBase64(QStringLiteral("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"), &pub));

        // 31 字节私钥（Base64 44 字符但解码长度不符，对应 easytier 的 invalid private key length）
        const QByteArray raw31(31, '\x01');
        const QString b64_31 = QString::fromLatin1(raw31.toBase64());
        QCOMPARE(b64_31.size(), 44);
        QVERIFY(!X25519KeyHelper::derivePublicKeyBase64(b64_31, &pub));

        // 33 字节私钥
        const QByteArray raw33(33, '\x02');
        const QString b64_33 = QString::fromLatin1(raw33.toBase64());
        QVERIFY(!X25519KeyHelper::derivePublicKeyBase64(b64_33, &pub));
    }
};

QTEST_MAIN(TestX25519KeyHelper)
#include "tst_x25519_key_helper.moc"
