/**
 * @file X25519KeyHelper.h
 * @brief X25519（Curve25519）密钥对生成与公钥派生工具
 *
 * 与 EasyTier secure mode 密钥对格式对齐：
 * - 算法：X25519（RFC 7748），纯 DH 密钥，无签名能力
 * - 私钥：32 字节随机数，按 RFC 7748 clamp
 * - 公钥：32 字节，= 私钥 × Curve25519 base point 的 u 坐标
 * - 编码：私钥/公钥均以原始 32 字节做标准 Base64（带 = padding），恒为 44 字符
 *
 * 基于 OpenSSL EVP API 实现（静态链接 libcrypto.a）。
 * 纯静态工具类，无状态、无所有权问题，可被 NetworkConfToml 序列化层复用。
 */
#pragma once

#include <QString>

class X25519KeyHelper {
public:
    /**
     * @brief 生成一个随机的 X25519 私钥，并以标准 Base64 返回
     *
     * OpenSSL 生成的 X25519 私钥已按 RFC 7748 完成 clamp。
     *
     * @return 44 字符 Base64 私钥；失败时返回空字符串
     */
    static QString generatePrivateKeyBase64();

    /**
     * @brief 由 Base64 私钥实时推导对应的 Base64 公钥
     *
     * 解码并校验私钥必须恰好 32 字节（否则判定无效），
     * 通过 X25519 标量乘法计算公钥后以标准 Base64 返回。
     *
     * @param privateKeyBase64 Base64 编码的 32 字节私钥
     * @param publicKeyBase64  输出参数，写入 44 字符 Base64 公钥；失败时清空
     * @return true 推导成功，false 私钥无效（长度不符 / 解析失败 / 内部错误）
     */
    static bool derivePublicKeyBase64(const QString &privateKeyBase64, QString *publicKeyBase64);
};
