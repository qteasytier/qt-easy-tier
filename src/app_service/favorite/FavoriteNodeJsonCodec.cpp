/**
 * @file FavoriteNodeJsonCodec.cpp
 * @brief FavoriteNodeJsonCodec 实现
 */
#include "FavoriteNodeJsonCodec.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

QString FavoriteNodeJsonCodec::defaultDisplayName()
{
    return QStringLiteral("UNKNOW");
}

FavoriteNodeJsonParseResult FavoriteNodeJsonCodec::parseNodes(const QByteArray &json)
{
    FavoriteNodeJsonParseResult result;

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        result.error = QStringLiteral("JSON 格式错误: %1").arg(parseError.errorString());
        return result;
    }
    if (!doc.isArray()) {
        result.error = QStringLiteral("节点文件格式错误：顶层必须是数组");
        return result;
    }

    const QJsonArray array = doc.array();
    for (const QJsonValue &value : array) {
        if (!value.isObject()) {
            ++result.skippedCount;
            continue;
        }

        const QJsonObject obj = value.toObject();
        const QString uri = obj.value(QStringLiteral("uri")).toString().trimmed();
        if (uri.isEmpty()) {
            ++result.skippedCount;
            continue;
        }

        FavoriteNode node;
        node.uri = uri;
        node.name = obj.value(QStringLiteral("display_name")).toString().trimmed();
        if (node.name.isEmpty())
            node.name = defaultDisplayName();
        node.publicKey = obj.value(QStringLiteral("publicKey")).toString().trimmed();
        result.nodes.append(node);
    }

    return result;
}

QByteArray FavoriteNodeJsonCodec::toJson(const QList<FavoriteNode> &nodes)
{
    QJsonArray array;
    for (const FavoriteNode &node : nodes) {
        QJsonObject obj;
        obj.insert(QStringLiteral("uri"), node.uri);
        obj.insert(QStringLiteral("display_name"), node.name.isEmpty() ? defaultDisplayName() : node.name);
        obj.insert(QStringLiteral("publicKey"), node.publicKey);
        array.append(obj);
    }

    return QJsonDocument(array).toJson(QJsonDocument::Indented);
}

FavoriteNodeJsonParseResult FavoriteNodeJsonCodec::loadNodes(const QUrl &fileUrl)
{
    FavoriteNodeJsonParseResult result;
    const QString path = pathFromUrl(fileUrl);
    if (path.isEmpty()) {
        result.error = QStringLiteral("节点文件路径为空");
        return result;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("无法打开节点文件: %1").arg(path);
        return result;
    }

    return parseNodes(file.readAll());
}

bool FavoriteNodeJsonCodec::saveNodes(const QUrl &fileUrl, const QList<FavoriteNode> &nodes, QString *error)
{
    const QString path = pathFromUrl(fileUrl);
    if (path.isEmpty()) {
        if (error)
            *error = QStringLiteral("节点文件路径为空");
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error)
            *error = QStringLiteral("无法写入节点文件: %1").arg(path);
        return false;
    }

    if (file.write(toJson(nodes)) == -1) {
        if (error)
            *error = QStringLiteral("写入节点文件失败: %1").arg(path);
        return false;
    }
    return true;
}

QString FavoriteNodeJsonCodec::pathFromUrl(const QUrl &fileUrl)
{
    if (fileUrl.isLocalFile())
        return fileUrl.toLocalFile();
    if (fileUrl.scheme() == QStringLiteral("qrc"))
        return QStringLiteral(":") + fileUrl.path();
    if (fileUrl.scheme().isEmpty())
        return fileUrl.toString();
    return fileUrl.toString();
}
