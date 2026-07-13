/**
 * @file DaemonApi.h
 * @brief daemon API 调用封装
 *
 * 对 DaemonClient 的二次封装，将常用的 daemon RPC 调用包装为语义明确的方法。
 * 每个方法对应一个 daemon JSON-RPC 方法名，调用方无需手动构造方法名字符串和参数。
 */
#pragma once

#include <QFuture>
#include <QJsonObject>
#include <QObject>
#include <QString>

class DaemonClient;

/**
 * @class DaemonApi
 * @brief daemon JSON-RPC 接口的高层封装
 *
 * 提供解析配置、运行/删除网络实例、列出实例、采集网络信息等常用接口。
 * 所有方法均返回 QFuture<QJsonObject>，支持异步等待结果。
 *
 * 设计要点：
 * - 不持有 DaemonClient 所有权，仅通过外部传入的指针调用
 * - 每个方法内部调用 DaemonClient::call()，屏蔽了 method 字符串的拼写差异
 * - 通过此抽象层，调用方不需要了解 daemon 的底层 JSON-RPC 方法名
 */
class DaemonApi : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造 API 封装
     * @param client 已初始化的 DaemonClient 指针（非空，由外部管理生命周期）
     * @param parent Qt 父对象
     */
    explicit DaemonApi(DaemonClient *client, QObject *parent = nullptr);

    /**
     * @brief 解析网络配置文件
     * @param payload 包含配置内容的 JSON 对象
     * @return 解析结果的 QFuture
     */
    QFuture<QJsonObject> parseConfig(const QJsonObject &payload);

    /**
     * @brief 运行网络实例
     * @param payload 包含实例配置的 JSON 对象
     * @return 运行结果的 QFuture
     */
    QFuture<QJsonObject> runNetworkInstance(const QJsonObject &payload);

    /**
     * @brief 删除网络实例
     * @param instanceName 要删除的实例名称
     * @return 删除结果的 QFuture
     */
    QFuture<QJsonObject> deleteNetworkInstance(const QString &instanceName);

    /**
     * @brief 列出所有运行中的网络实例
     * @return 实例列表的 QFuture
     */
    QFuture<QJsonObject> listInstances();

    /**
     * @brief 采集网络信息
     * @param maxLength 最大采集长度
     * @return 网络信息的 QFuture
     */
    QFuture<QJsonObject> collectNetworkInfos(int maxLength);

    QFuture<QJsonObject> setAutoReconnect(bool enabled);
    QFuture<QJsonObject> getAutoReconnect();

private:
    DaemonClient *m_client = nullptr; ///< daemon IPC 客户端指针（外部管理生命周期）
};
