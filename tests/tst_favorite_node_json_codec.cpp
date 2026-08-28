/**
 * @file tst_favorite_node_json_codec.cpp
 * @brief 收藏节点 JSON 编解码模块的单元测试。
 */
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

#include "core/favorite/FavoriteNodeJsonCodec.h"

class TestFavoriteNodeJsonCodec : public QObject {
    Q_OBJECT

private slots:
    /// 测试目标：验证 display_name 格式可被解析，空 URI 和非对象项会被跳过
    void parseNodes_returnsValidEntriesOnly()
    {
        const QByteArray json = "["
            "{\"uri\":\"tcp://valid.example.com:11010\",\"display_name\":\"alice\",\"publicKey\":\"key-a\"},"
            "{\"uri\":\"\",\"display_name\":\"ignored\",\"publicKey\":\"key-b\"},"
            "42"
            "]";

        const FavoriteNodeJsonParseResult result = FavoriteNodeJsonCodec::parseNodes(json);

        QVERIFY(result.error.isEmpty());
        QCOMPARE(result.nodes.size(), 1);
        QCOMPARE(result.skippedCount, 2);
        QCOMPARE(result.nodes.first().uri, QStringLiteral("tcp://valid.example.com:11010"));
        QCOMPARE(result.nodes.first().name, QStringLiteral("alice"));
        QCOMPARE(result.nodes.first().publicKey, QStringLiteral("key-a"));
    }

    /// 测试目标：验证缺失或空 display_name 时使用 UNKNOW
    void parseNodes_usesDefaultNameWhenDisplayNameMissing()
    {
        const QByteArray json = "["
            "{\"uri\":\"tcp://missing.example.com:11010\",\"publicKey\":\"key-a\"},"
            "{\"uri\":\"tcp://empty.example.com:11010\",\"display_name\":\"   \"}"
            "]";

        const FavoriteNodeJsonParseResult result = FavoriteNodeJsonCodec::parseNodes(json);

        QVERIFY(result.error.isEmpty());
        QCOMPARE(result.nodes.size(), 2);
        QCOMPARE(result.nodes.at(0).name, QStringLiteral("UNKNOW"));
        QCOMPARE(result.nodes.at(1).name, QStringLiteral("UNKNOW"));
    }

    /// 测试目标：验证非法 JSON 与非数组 JSON 会返回错误
    void parseNodes_returnsErrorForInvalidJsonOrRootType()
    {
        QVERIFY(!FavoriteNodeJsonCodec::parseNodes("not-json").error.isEmpty());
        QVERIFY(!FavoriteNodeJsonCodec::parseNodes("{\"uri\":\"tcp://one.example.com\"}").error.isEmpty());
    }

    /// 测试目标：验证序列化字段使用 display_name，不再输出 contributor
    void toJson_writesDisplayNameField()
    {
        FavoriteNode node;
        node.name = QStringLiteral("alice");
        node.uri = QStringLiteral("tcp://valid.example.com:11010");
        node.publicKey = QStringLiteral("key-a");

        const QByteArray json = FavoriteNodeJsonCodec::toJson({node});
        const QJsonDocument doc = QJsonDocument::fromJson(json);
        QVERIFY(doc.isArray());
        const QJsonObject obj = doc.array().first().toObject();

        QCOMPARE(obj.value(QStringLiteral("display_name")).toString(), QStringLiteral("alice"));
        QVERIFY(!obj.contains(QStringLiteral("contributor")));
    }
};

QTEST_MAIN(TestFavoriteNodeJsonCodec)
#include "tst_favorite_node_json_codec.moc"
