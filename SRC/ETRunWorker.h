//
// Created by YMHuang on 2026/4/5.
//

#ifndef QTEASYTIER_ETRUNWORKER_H
#define QTEASYTIER_ETRUNWORKER_H

#include "easytier_ffi.h"
#include <QObject>
#include <string>
#include <vector>
#include <QMetaType>

// 注册 std::string 为 Qt 元类型，以便在信号槽中使用
Q_DECLARE_METATYPE(std::string)

#define MAX_WAIT_TIME 30000
#define MAX_NETWORK_INSTANCES 16

// easytier_ffi的C++绑定
namespace EasyTierFFI {
    /**
     * @brief 键值对结构体
     *
     * 用于运行时获取网络实例的信息，
     * 获取到的信息可以用于监测网络状态等。
     *
     * @param[in] key 键，实例名称
     * @param[in] value 值，JSON格式的实例信息字符串
     */
    struct KVPair
    {
        std::string key;
        std::string value;
    };

    /**
     * @brief 解析配置字符串
     *
     * 验证TOML格式的配置字符串是否有效。此函数仅进行语法检查，
     * 不会创建或启动网络实例。
     *
     * @param[in] cfgStr TOML格式的配置字符串，UTF-8编码，不能为空
     * @return 成功返回 true，失败返回 false
     *
     * @see run_network_instance() 启动网络实例
     * @see get_error_msg() 获取错误信息
     */
    bool parseConfig(const std::string& cfgStr);

    /**
     * @brief 运行网络实例
     *
     * 根据提供的TOML配置字符串创建并启动一个新的网络实例。
     * 每个实例通过配置中的 instance_name 字段唯一标识。
     *
     * @param[in] cfgStr TOML格式的配置字符串，UTF-8编码，不能为NULL
     * @return 成功返回 true，失败返回 false
     *
     * @note 如果实例名称已存在，将返回错误。
     *
     * @see parseConfig() 验证配置
     * @see retainNetworkInstance() 管理实例生命周期
     * @see getErrorMsg() 获取错误信息
     */
    bool runNetworkInstance(const std::string &cfgStr);

    /**
     * @brief 保留指定的网络实例
     *
     * 保留指定的网络实例，终止不在列表中的实例。
     * 这是一个批量管理函数，可用于清理不再需要的实例。
     *
     * @param[in] instNames 要保留的实例名称数组，每个元素为实例名称字符串
     * @param[in] length 实例名称数组的长度，若为0则终止所有实例
     * @return 成功返回 true，失败返回 false
     *
     * @warning 此操作不可逆，被终止的实例将无法恢复。
     * @warning 传入 length=0 将终止所有运行中的实例。
     *
     * @see run_network_instance() 启动实例
     * @see get_error_msg() 获取错误信息
     */
    bool retainNetworkInstance(const std::vector<std::string> &instNames, size_t &length);

    /**
     * @brief 收集网络实例信息
     *
     * 收集所有运行中的网络实例的状态信息，返回实例名称与详细信息的映射。
     * 实例信息以JSON格式返回，包含节点、路由、对等连接等详细信息。
     *
     * @param[out] infos 接收结果的 KeyValuePair 数组，由调用者分配内存
     * @param[in] maxLength infos数组的最大容量
     * @return 成功返回实际填充的数量，失败返回 -1
     *
     * @note 返回的 KeyValuePair 中的 key 和 value 指针需要调用 free_string() 释放。
     * @note 如果实例数量超过 max_length，将只返回前 max_length 个实例的信息。
     *
     * @see getErrorMsg() 获取错误信息
     */
    int collectNetworkInfos(std::vector<KVPair>& infos, size_t &maxLength);

    /**
     * @brief 获取最后的错误信息
     *
     * 获取最近一次 FFI 调用失败时的详细错误信息。
     * 错误信息在下次调用失败时会被覆盖。
     *
     * @return 错误信息字符串的向量
     *
     * @note 如果没有错误，out 将被设置为空。
     * @note 返回的字符串需要调用 free_string() 释放。
     */
    std::vector<std::string> getErrorMsg();

}

/**
 * @brief EasyTier 网络运行工作类
 *
 * 符合 Qt Thread-Worker 规范的工作类，用于在独立线程中管理网络实例的运行和停止。
 * 通过信号槽机制与主线程通信，避免阻塞 UI。
 */
class ETRunWorker : public QObject
{
    Q_OBJECT

public:
    explicit ETRunWorker(QObject *parent = nullptr);
    ~ETRunWorker() override = default;

public slots:
    /**
     * @brief 运行网络实例
     * @param instName 实例名称（网络名称）
     * @param tomlConfig TOML 配置字符串
     */
    void runNetwork(const std::string &instName, const std::string &tomlConfig);

    /**
     * @brief 停止网络实例
     * @param instName 实例名称（网络名称）
     */
    void stopNetwork(const std::string &instName);

    /**
     * @brief 收集网络信息
     * 
     * 收集所有运行中网络实例的状态信息，通过信号返回。
     */
    void collectInfos();

signals:
    /**
     * @brief 网络启动完成信号
     * @param instName 实例名称
     * @param success 是否成功
     * @param errorMsg 错误信息（成功时为空）
     */
    void etRunStarted(const std::string &instName, bool success, const std::string &errorMsg);

    /**
     * @brief 网络停止完成信号
     * @param instName 实例名称
     * @param success 是否成功
     * @param errorMsg 错误信息（成功时为空）
     */
    void etRunStopped(const std::string &instName, bool success, const std::string &errorMsg);

    /**
     * @brief 网络信息收集完成信号
     * @param infos 网络信息列表（KVPair格式）
     */
    void infosCollected(const std::vector<EasyTierFFI::KVPair> &infos);

private:
    /**
     * @brief 获取错误信息字符串
     * @return 错误信息字符串
     */
    static std::string getLastErrorMsg();
};

// 注册 EasyTierFFI::KVPair 为 Qt 元类型
Q_DECLARE_METATYPE(EasyTierFFI::KVPair)
Q_DECLARE_METATYPE(std::vector<EasyTierFFI::KVPair>)

#endif //QTEASYTIER_ETRUNWORKER_H
