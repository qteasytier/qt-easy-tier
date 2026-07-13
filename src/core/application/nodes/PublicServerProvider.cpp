/** @file PublicServerProvider.cpp @brief PublicServerProvider 实现 */
#include "PublicServerProvider.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

PublicServerProvider::PublicServerProvider(QObject *parent)
    : PublicServerProvider(QStringLiteral(":/publicservers.json"), parent)
{
}

PublicServerProvider::PublicServerProvider(QString resourcePath, QObject *parent)
    : QObject(parent)
    , m_resourcePath(std::move(resourcePath))
{
}

QVariantList PublicServerProvider::loadPublicServers() const
{
    QVariantList result;

    // 打开 JSON 资源文件，失败则返回空列表
    QFile file(m_resourcePath);
    if (!file.open(QIODevice::ReadOnly))
        return result;

    // 解析 JSON，要求顶层为数组
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray())
        return result;

    // 遍历数组，提取每个服务器对象的 uri / contributor / publicKey
    const QJsonArray array = doc.array();
    for (const QJsonValue &value : array) {
        if (!value.isObject())
            continue;

        const QJsonObject obj = value.toObject();
        const QString uri = obj.value(QStringLiteral("uri")).toString();
        if (uri.isEmpty())
            continue;

        QVariantMap item;
        item[QStringLiteral("uri")] = uri;
        item[QStringLiteral("contributor")] = obj.value(QStringLiteral("contributor")).toString();
        item[QStringLiteral("publicKey")] = obj.value(QStringLiteral("publicKey")).toString();
        result.append(item);
    }

    return result;
}
