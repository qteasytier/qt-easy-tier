/**
 * @file ConfigPayloadBuilder.h
 * @brief 配置载荷构建器，将 NetworkConf 序列化为 daemon 可识别的 JSON 载荷
 *
 * 位于 core/config 基础服务层，供 VPN 运行（VpnController）与配置导入（ConfigImportExportService）
 * 等调用方统一构建运行时的 TOML / Base64 / JSON 载荷，避免各层各自实现序列化。
 */
#pragma once

#include <QJsonObject>
#include <QString>

class NetworkConf;

/** @brief 配置载荷构建器，负责将网络配置序列化为 daemon 使用的传输格式 */
class ConfigPayloadBuilder
{
public:
    /**
     * @brief 构建 daemon 可用的配置 JSON 载荷
     *
     * 载荷包含 cfg_str 字段，值为带 instance_name 的运行时 TOML 的 Base64 编码。
     * 注意：运行时载荷刻意包含 instance_name（daemon 启动实例时需要），
     * 而文件/URL 导出的 TOML 刻意不包含（导入时会分配新的实例名）。
     * @param conf 网络配置
     * @return 包含 cfg_str 字段的 JSON 对象
     */
    static QJsonObject daemonConfigPayload(const NetworkConf &conf);

private:
    /**
     * @brief 将配置序列化为 Base64 编码的 TOML 字符串（含 instance_name）
     * @param conf 网络配置
     * @return Base64 TOML 字符串
     */
    static QString toBase64Toml(const NetworkConf &conf);
};
