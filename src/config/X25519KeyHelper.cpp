/** @file X25519KeyHelper.cpp @brief X25519KeyHelper 实现，基于 OpenSSL EVP API */
#include "X25519KeyHelper.h"

#include <QByteArray>

#include <openssl/evp.h>

namespace {

/// X25519 密钥长度（32 字节 = 256 bit）
constexpr size_t kKeyLength = 32;

/// @brief 将原始 32 字节密钥编码为标准 Base64（带 = padding）
QString encodeRawKey(const unsigned char *raw, size_t len)
{
    return QString::fromLatin1(QByteArray(reinterpret_cast<const char *>(raw), int(len)).toBase64());
}

} // namespace

QString X25519KeyHelper::generatePrivateKeyBase64()
{
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr);
    if (!ctx)
        return {};

    EVP_PKEY *pkey = nullptr;
    QString result;

    do {
        if (EVP_PKEY_keygen_init(ctx) <= 0)
            break;
        if (EVP_PKEY_keygen(ctx, &pkey) <= 0)
            break;

        unsigned char raw[kKeyLength] = {0};
        size_t len = kKeyLength;
        if (EVP_PKEY_get_raw_private_key(pkey, raw, &len) != 1)
            break;
        if (len != kKeyLength)
            break;

        result = encodeRawKey(raw, len);
    } while (false);

    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(ctx);
    return result;
}

bool X25519KeyHelper::derivePublicKeyBase64(const QString &privateKeyBase64, QString *publicKeyBase64)
{
    if (publicKeyBase64)
        publicKeyBase64->clear();

    // 解码 Base64 私钥并校验长度：必须恰好 32 字节，否则报错（对应 easytier 的 invalid private key length）
    const QByteArray rawPrivate = QByteArray::fromBase64(privateKeyBase64.toLatin1());
    if (rawPrivate.size() != int(kKeyLength))
        return false;

    EVP_PKEY *pkey = EVP_PKEY_new_raw_private_key(
        EVP_PKEY_X25519, nullptr,
        reinterpret_cast<const unsigned char *>(rawPrivate.constData()), kKeyLength);
    if (!pkey)
        return false;

    bool ok = false;
    do {
        unsigned char rawPublic[kKeyLength] = {0};
        size_t pubLen = kKeyLength;
        if (EVP_PKEY_get_raw_public_key(pkey, rawPublic, &pubLen) != 1)
            break;
        if (pubLen != kKeyLength)
            break;

        if (publicKeyBase64)
            *publicKeyBase64 = encodeRawKey(rawPublic, pubLen);
        ok = true;
    } while (false);

    EVP_PKEY_free(pkey);
    return ok;
}
