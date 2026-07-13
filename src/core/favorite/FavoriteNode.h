/**
 * @file FavoriteNode.h
 * @brief 收藏节点数据结构
 *
 * 存储用户收藏的服务器节点信息，对应 SQLite 表中一行记录。
 * 纯 POD 结构体，无行为逻辑，无指针成员，值语义安全。
 *
 * id == -1 表示尚未存入数据库（新创建待保存的记录）。
 */
#pragma once
#include <QString>
#include <QtTypes>

struct FavoriteNode {
    qint64 id = -1;          ///< 数据库主键，-1 表示未持久化
    QString name;            ///< 收藏节点显示名称
    QString uri;             ///< 节点地址 URI
    QString publicKey;       ///< 对等节点公钥（可选）
    qint64 createdAt = 0;    ///< 创建时间戳（Unix 毫秒）
};
