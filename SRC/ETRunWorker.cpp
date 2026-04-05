//
// Created by YMHuang on 2026/4/5.
//

#include "ETRunWorker.h"
#include <cstring>
#include <iostream>


// FFI 函数已在 easytier_ffi.h 中声明为 extern "C"
// 这里直接使用，链接时会找到对应的符号

namespace EasyTierFFI {

bool parseConfig(const std::string& cfgStr)
{
    if (cfgStr.empty()) {
        return false;
    }
    return parse_config(cfgStr.c_str()) == 0;
}

bool runNetworkInstance(const std::string& cfgStr)
{
    if (cfgStr.empty()) {
        return false;
    }
    return run_network_instance(cfgStr.c_str()) == 0;
}

bool retainNetworkInstance(const std::vector<std::string>& instNames, size_t& length)
{
    // 如果长度为0，终止所有实例
    if (instNames.empty() || length == 0) {
        length = 0;
        return retain_network_instance(nullptr, 0) == 0;
    }

    // 将 vector<string> 转换为 const char** 数组
    std::vector<const char*> cStrArray;
    cStrArray.reserve(instNames.size());
    
    for (const auto& name : instNames) {
        cStrArray.push_back(name.c_str());
    }

    const int result = retain_network_instance(cStrArray.data(), cStrArray.size());
    length = cStrArray.size();
    
    return result == 0;
}

int collectNetworkInfos(std::vector<KVPair>& infos, size_t& maxLength)
{
    // 创建 C 风格的 KeyValuePair 数组，大小为 maxLength
    std::vector<KeyValuePair> cInfos(maxLength);

    // 调用 FFI 函数
    const int count = collect_network_infos(cInfos.data(), maxLength);

    if (count < 0) {
        // 调用失败
        maxLength = 0;
        return -1;
    }

    // 清空输出 vector 并预分配空间
    infos.clear();
    infos.reserve(count);

    // 将 C 风格的 KeyValuePair 转换为 C++ 的 KVPair
    for (int i = 0; i < count; ++i) {
        KVPair pair;

        // 拷贝 key
        if (cInfos[i].key != nullptr) {
            pair.key = std::string(cInfos[i].key);
            // 释放 FFI 分配的内存
            free_string(cInfos[i].key);
        }

        // 拷贝 value
        if (cInfos[i].value != nullptr) {
            pair.value = std::string(cInfos[i].value);
            // 释放 FFI 分配的内存
            free_string(cInfos[i].value);
        }

        infos.push_back(std::move(pair));
    }

    // 更新 maxLength 为实际获取的数量
    maxLength = static_cast<size_t>(count);

    return count;
}

std::vector<std::string> getErrorMsg()
{
    const char* errorMsg = nullptr;
    
    // 调用 FFI 函数获取错误信息指针
    get_error_msg(&errorMsg);
    
    std::vector<std::string> result;
    
    if (errorMsg != nullptr) {
        // 将 C 字符串内容拷贝到 C++ string
        // 注意：必须先拷贝再释放，不能直接使用返回的指针
        result.emplace_back(errorMsg);
        
        // 立即释放 FFI 分配的字符串内存
        free_string(errorMsg);
    }
    
    return result;
}

} // namespace EasyTierFFI

// ==================== ETRunWorker 类实现 ====================

ETRunWorker::ETRunWorker(QObject *parent)
    : QObject(parent)
{
    // 注册 std::string 元类型，以便在跨线程信号槽中使用
    qRegisterMetaType<std::string>("std::string");
}

void ETRunWorker::runNetwork(const std::string &instName, const std::string &tomlConfig)
{
    // 调用 FFI 运行网络实例
    const bool success = EasyTierFFI::runNetworkInstance(tomlConfig);
    
    std::string errorMsg;
    if (!success) {
        errorMsg = getLastErrorMsg();
    }
    
    // 发出启动完成信号
    emit etRunStarted(instName, success, errorMsg);
}

void ETRunWorker::stopNetwork(const std::string &instName)
{
    // 使用 retain_network_instance 终止指定实例
    // 传入空列表表示终止所有实例，这里我们需要保留其他实例
    // 所以需要获取当前运行的所有实例，然后移除要停止的实例
    
    std::vector<EasyTierFFI::KVPair> infos;
    size_t maxLen = MAX_NETWORK_INSTANCES;
    int count = EasyTierFFI::collectNetworkInfos(infos, maxLen);
    
    // 构建要保留的实例名称列表（排除要停止的实例）
    std::vector<std::string> retainNames;
    if (count > 0) {
        for (const auto &info : infos) {
            if (info.key != instName) {
                retainNames.push_back(info.key);
            }
        }
    }
    
    // 调用 retain 终止指定实例
    size_t retainLen = retainNames.size();
    bool success = EasyTierFFI::retainNetworkInstance(retainNames, retainLen);
    
    std::string errorMsg;
    if (!success) {
        errorMsg = getLastErrorMsg();
    }
    
    // 发出停止完成信号
    emit etRunStopped(instName, success, errorMsg);
}

std::string ETRunWorker::getLastErrorMsg()
{
    auto errors = EasyTierFFI::getErrorMsg();
    if (errors.empty()) {
        return "Unknown error";
    }
    return errors[0];
}
