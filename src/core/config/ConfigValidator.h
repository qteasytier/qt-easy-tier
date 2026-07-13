/**
 * @file ConfigValidator.h
 * @brief 配置字段级验证器
 *
 * 对 NetworkConf 进行基本合法性校验：
 * - 必填字段非空检查
 * - 集合元素去重检查
 * - 返回人类可读的错误信息列表
 *
 * 注意：当前不校验 IP/CIDR 格式、端口范围等网络有效性，
 * 仅做结构和业务逻辑层面的基础校验。
 */
#pragma once
#include <QString>
#include <QStringList>

class NetworkConf;

class ConfigValidator {
public:
    /**
     * @brief 对网络配置进行校验
     * @param conf 要校验的配置对象（const 引用，不修改）
     * @return 错误信息列表，为空表示校验通过
     */
    static QStringList validate(const NetworkConf &conf);
};
