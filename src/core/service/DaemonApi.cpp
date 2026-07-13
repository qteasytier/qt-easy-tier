/**
 * @file DaemonApi.cpp
 * @brief daemon API 调用封装实现
 *
 * 将高层 API 方法映射为 DaemonClient::call() 的 JSON-RPC 调用。
 * 每个方法封装了对应的方法名字符串和参数构造逻辑，
 * 调用方只需关心业务语义，无需了解底层协议细节。
 */
#include "DaemonApi.h"

#include "core/service/DaemonClient.h"

#include <QJsonArray>

/**
 * @brief 构造函数：保存 DaemonClient 引用
 * @param client daemon IPC 客户端指针
 * @param parent Qt 父对象
 */
DaemonApi::DaemonApi(DaemonClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
}

/**
 * @brief 解析配置 → daemon RPC: parse_config
 * @param payload 配置 JSON 对象
 * @return 异步解析结果
 */
QFuture<QJsonObject> DaemonApi::parseConfig(const QJsonObject &payload)
{
    return m_client->call(QStringLiteral("parse_config"), payload);
}

/**
 * @brief 运行网络实例 → daemon RPC: run_network_instance
 * @param payload 实例配置 JSON 对象
 * @return 异步运行结果
 */
QFuture<QJsonObject> DaemonApi::runNetworkInstance(const QJsonObject &payload)
{
    return m_client->call(QStringLiteral("run_network_instance"), payload);
}

/**
 * @brief 删除网络实例 → daemon RPC: delete_network_instance
 * @param instanceName 实例名称
 * @return 异步删除结果
 *
 * daemon 接口要求 inst_names 参数为 JSON 数组（支持批量），
 * 此处将单个实例名包装为单元素数组传递。
 */
QFuture<QJsonObject> DaemonApi::deleteNetworkInstance(const QString &instanceName)
{
    QJsonArray names;
    names.append(instanceName);
    return m_client->call(QStringLiteral("delete_network_instance"),
                          QJsonObject{{QStringLiteral("inst_names"), names}});
}

/**
 * @brief 列出所有实例 → daemon RPC: list_instances
 * @return 异步实例列表
 */
QFuture<QJsonObject> DaemonApi::listInstances()
{
    return m_client->call(QStringLiteral("list_instances"), QJsonObject{});
}

/**
 * @brief 采集网络信息 → daemon RPC: collect_network_infos
 * @param maxLength 最大采集长度
 * @return 异步网络信息
 */
QFuture<QJsonObject> DaemonApi::collectNetworkInfos(int maxLength)
{
    return m_client->call(QStringLiteral("collect_network_infos"),
                          QJsonObject{{QStringLiteral("max_length"), maxLength}});
}

QFuture<QJsonObject> DaemonApi::setAutoReconnect(bool enabled)
{
    return m_client->call(QStringLiteral("set_auto_reconnect"),
                          QJsonObject{{QStringLiteral("enabled"), enabled}});
}

QFuture<QJsonObject> DaemonApi::getAutoReconnect()
{
    return m_client->call(QStringLiteral("get_auto_reconnect"), QJsonObject{});
}
