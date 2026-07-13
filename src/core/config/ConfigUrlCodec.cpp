#include "ConfigUrlCodec.h"
#include "core/util/LogHelper.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <toml.hpp>

namespace {

QMap<QString, int> s_nameToId;
QMap<int, QString> s_idToName;
QSet<int> s_booleanIds;
bool s_mappingLoaded = false;
QString s_mappingFilePath;

void ensureMappingLoaded()
{
    if (s_mappingLoaded) return;
    s_mappingLoaded = true;

    QFile file(s_mappingFilePath.isEmpty() ? QStringLiteral(":/field_mapping.json") : s_mappingFilePath);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    if (error.error != QJsonParseError::NoError) return;

    QJsonObject mapping = doc.object()[QStringLiteral("mapping")].toObject();
    for (auto it = mapping.begin(); it != mapping.end(); ++it) {
        int id = it.value().toInt();
        s_nameToId[it.key()] = id;
        s_idToName[id] = it.key();
    }

    const QSet<QString> booleanNames = {
        QStringLiteral("dhcp"),
        QStringLiteral("enable_encryption"),
        QStringLiteral("latency_first"),
        QStringLiteral("private_mode"),
        QStringLiteral("no_tun"),
        QStringLiteral("enable_kcp_proxy"),
        QStringLiteral("disable_kcp_input"),
        QStringLiteral("enable_quic_proxy"),
        QStringLiteral("disable_quic_input"),
        QStringLiteral("disable_relay_kcp"),
        QStringLiteral("disable_relay_quic"),
        QStringLiteral("enable_relay_foreign_network_kcp"),
        QStringLiteral("enable_relay_foreign_network_quic"),
        QStringLiteral("disable_udp_hole_punching"),
        QStringLiteral("disable_tcp_hole_punching"),
        QStringLiteral("disable_upnp"),
        QStringLiteral("need_p2p"),
        QStringLiteral("lazy_p2p"),
        QStringLiteral("p2p_only"),
        QStringLiteral("disable_p2p"),
        QStringLiteral("disable_sym_hole_punching"),
        QStringLiteral("relay_all_peer_rpc"),
        QStringLiteral("bind_device"),
        QStringLiteral("multi_thread"),
        QStringLiteral("use_smoltcp"),
        QStringLiteral("enable_ipv6"),
        QStringLiteral("enable_exit_node"),
        QStringLiteral("proxy_forward_by_system"),
        QStringLiteral("accept_dns"),
        QStringLiteral("enable_foreign_network_whitelist"),
        QStringLiteral("secure_mode_enabled"),
        QStringLiteral("enabled")
    };
    for (const auto &name : booleanNames) {
        if (s_nameToId.contains(name))
            s_booleanIds.insert(s_nameToId[name]);
    }
}

QString escapeToml(const QString &s)
{
    QString r = s;
    r.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    r.replace(QLatin1Char('"'),  QStringLiteral("\\\""));
    r.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    return r;
}

QVariantMap encodeTable(const toml::table &tbl, const QMap<QString, int> &nameToId);

QVariant encodeNode(const toml::node &node, const QMap<QString, int> &nameToId)
{
    if (node.is_string()) {
        if (auto s = node.value<std::string>())
            return QString::fromStdString(*s);
    } else if (node.is_boolean()) {
        if (auto b = node.value<bool>())
            return *b ? 1 : 0;
    } else if (node.is_integer()) {
        if (auto i = node.value<int64_t>())
            return static_cast<int>(*i);
    } else if (node.is_table()) {
        if (auto t = node.as_table())
            return encodeTable(*t, nameToId);
    } else if (node.is_array()) {
        if (auto a = node.as_array()) {
            if (a->empty()) return QVariantList();
            if (a->front().is_table()) {
                QVariantList list;
                for (const auto &elem : *a) {
                    if (auto t = elem.as_table())
                        list.append(encodeTable(*t, nameToId));
                }
                return list;
            }
            QStringList sl;
            for (const auto &elem : *a) {
                if (auto s = elem.value<std::string>())
                    sl.append(QString::fromStdString(*s));
            }
            return QVariant::fromValue(sl);
        }
    }
    return {};
}

QVariantMap encodeTable(const toml::table &tbl, const QMap<QString, int> &nameToId)
{
    QVariantMap result;
    for (const auto &[key, value] : tbl) {
        int fieldId = nameToId.value(QString::fromStdString(std::string{key.begin(), key.end()}), -1);
        if (fieldId < 0) continue;
        QVariant encoded = encodeNode(value, nameToId);
        if (encoded.isValid() && !encoded.isNull())
            result[QString::number(fieldId)] = encoded;
    }
    return result;
}

void emitFieldLine(QStringList &lines, int fieldId, const QString &name,
                   const QVariant &value, const QSet<int> &booleanIds)
{
    int metaType = value.userType();

    if (metaType == QMetaType::QStringList || metaType == QMetaType::QVariantList) {
        QStringList list;
        if (metaType == QMetaType::QStringList)
            list = value.toStringList();
        else {
            for (const auto &v : value.toList())
                list.append(v.toString());
        }
        QStringList quoted;
        for (const auto &s : list)
            quoted << QLatin1Char('"') + escapeToml(s) + QLatin1Char('"');
        lines << QStringLiteral("%1 = [%2]").arg(name, quoted.join(QStringLiteral(", ")));
        return;
    }
    if (metaType == QMetaType::Int || metaType == QMetaType::Bool
        || metaType == QMetaType::LongLong) {
        int v = value.toInt();
        if ((v == 0 || v == 1) && booleanIds.contains(fieldId))
            lines << QStringLiteral("%1 = %2").arg(name, v ? QStringLiteral("true") : QStringLiteral("false"));
        else
            lines << QStringLiteral("%1 = %2").arg(name).arg(v);
        return;
    }
    if (metaType == QMetaType::QString) {
        lines << QStringLiteral("%1 = \"%2\"").arg(name, escapeToml(value.toString()));
        return;
    }
}

} // anonymous namespace

void ConfigUrlCodec::setMappingFilePath(const QString &path)
{
    s_mappingFilePath = path;
    s_mappingLoaded = false;
    s_nameToId.clear();
    s_idToName.clear();
    s_booleanIds.clear();
}

ConfigTextResult ConfigUrlCodec::encodeToml(const QString &toml)
{
    ensureMappingLoaded();
    if (s_nameToId.isEmpty())
        return ConfigTextResult::fail(QStringLiteral("字段映射表未加载"));

    toml::table tbl;
    try {
        tbl = toml::parse(toml.toStdString());
    } catch (const std::exception &e) {
        return ConfigTextResult::fail(QStringLiteral("TOML 解析失败: %1").arg(e.what()));
    }

    QVariantMap root = encodeTable(tbl, s_nameToId);
    if (root.isEmpty())
        return ConfigTextResult::fail(QStringLiteral("TOML 编码后无有效字段"));

    QVariantMap wrapper;
    wrapper[QStringLiteral("v")] = 1;
    wrapper[QStringLiteral("d")] = root;
    QJsonDocument doc = QJsonDocument::fromVariant(wrapper);
    QString json = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    QString encoded = QString::fromUtf8(json.toUtf8().toBase64(QByteArray::Base64UrlEncoding));
    return ConfigTextResult::ok(QStringLiteral("qtet://") + encoded);
}

ConfigTextResult ConfigUrlCodec::decodeUrl(const QString &url)
{
    ensureMappingLoaded();
    if (s_idToName.isEmpty())
        return ConfigTextResult::fail(QStringLiteral("字段映射表未加载"));

    if (!url.startsWith(QStringLiteral("qtet://")))
        return ConfigTextResult::fail(QStringLiteral("无效的配置 URL，需要以 qtet:// 开头"));

    QString payload = url.mid(7);
    if (payload.isEmpty())
        return ConfigTextResult::fail(QStringLiteral("URL 载荷为空"));

    QByteArray decoded = QByteArray::fromBase64(payload.toUtf8(), QByteArray::Base64UrlEncoding);
    if (decoded.isEmpty())
        return ConfigTextResult::fail(QStringLiteral("Base64 解码失败"));

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(decoded, &error);
    if (error.error != QJsonParseError::NoError)
        return ConfigTextResult::fail(QStringLiteral("JSON 解析失败: %1").arg(error.errorString()));

    QVariantMap wrapper = doc.object().toVariantMap();
    if (wrapper.value(QStringLiteral("v")).toInt() != 1)
        return ConfigTextResult::fail(QStringLiteral("不支持的配置 URL 版本"));

    QVariantMap data = wrapper.value(QStringLiteral("d")).toMap();
    if (data.isEmpty())
        return ConfigTextResult::fail(QStringLiteral("URL 载荷无配置数据"));

    const QSet<int> tableSections = {9, 19, 53};
    const QSet<int> arrayTableSections = {12, 15};

    QStringList rootLines;
    QStringList sectionLines;

    for (auto it = data.begin(); it != data.end(); ++it) {
        int fieldId = it.key().toInt();
        QString name = s_idToName.value(fieldId);
        if (name.isEmpty()) continue;

        if (tableSections.contains(fieldId)) {
            QVariantMap sub = it.value().toMap();
            if (sub.isEmpty()) continue;
            sectionLines << QString();
            sectionLines << QStringLiteral("[%1]").arg(name);
            for (auto si = sub.begin(); si != sub.end(); ++si) {
                int subId = si.key().toInt();
                QString subName = s_idToName.value(subId);
                if (subName.isEmpty()) continue;
                emitFieldLine(sectionLines, subId, subName, si.value(), s_booleanIds);
            }
        } else if (arrayTableSections.contains(fieldId)) {
            QVariantList entries = it.value().toList();
            for (const auto &entry : entries) {
                QVariantMap entryMap = entry.toMap();
                if (entryMap.isEmpty()) continue;
                sectionLines << QString();
                sectionLines << QStringLiteral("[[%1]]").arg(name);
                for (auto si = entryMap.begin(); si != entryMap.end(); ++si) {
                    int subId = si.key().toInt();
                    QString subName = s_idToName.value(subId);
                    if (subName.isEmpty()) continue;
                    emitFieldLine(sectionLines, subId, subName, si.value(), s_booleanIds);
                }
            }
        } else {
            emitFieldLine(rootLines, fieldId, name, it.value(), s_booleanIds);
        }
    }

    QStringList lines = rootLines + sectionLines;
    QString toml = lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
    if (toml.trimmed().isEmpty())
        return ConfigTextResult::fail(QStringLiteral("解码后 TOML 为空"));
    return ConfigTextResult::ok(toml);
}
