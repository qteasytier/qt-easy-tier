/** @file ConfigUrlCodec.h @brief TOML <-> qtet:// URL 编解码基础设施 */
#pragma once

#include <QString>

struct ConfigTextResult {
    bool success = false;
    QString value;
    QString error;

    static ConfigTextResult ok(const QString &v) { return {true, v, {}}; }
    static ConfigTextResult fail(const QString &e) { return {false, {}, e}; }
};

class ConfigUrlCodec {
public:
    static void setMappingFilePath(const QString &path);
    static ConfigTextResult encodeToml(const QString &toml);
    static ConfigTextResult decodeUrl(const QString &url);
};
