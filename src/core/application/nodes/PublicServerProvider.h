/** @file PublicServerProvider.h @brief 公共服务器提供者，从资源文件加载公共服务器列表 */
#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

/** @brief 从 JSON 资源文件加载公共服务器列表，供 QML 层使用 */
class PublicServerProvider : public QObject {
    Q_OBJECT

public:
    /** @brief 默认构造函数，使用内置的 :/publicservers.json 资源 */
    explicit PublicServerProvider(QObject *parent = nullptr);
    /** @brief 指定资源路径的构造函数 @param resourcePath JSON 资源文件路径 @param parent 父对象 */
    explicit PublicServerProvider(QString resourcePath, QObject *parent = nullptr);

    /** @brief 加载并解析公共服务器列表 @return 包含 uri/contributor/publicKey 的 QVariantMap 列表 */
    QVariantList loadPublicServers() const;

private:
    QString m_resourcePath;
};
