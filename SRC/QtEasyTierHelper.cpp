#include "easytier_ffi.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QUuid>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace {

constexpr int kMaxNetworkInstances = 16;
constexpr int kProtocolVersion = 1;
#if !QTEASYTIER_ENABLE_PROTOTYPE_HELPER
constexpr const char *kPrototypeHelperDisabledMarker = "QTEASYTIER_PROTOTYPE_HELPER_DISABLED";
#endif
#ifndef PROJECT_VERSION
#define PROJECT_VERSION "0.0.0"
#endif
constexpr const char *kHelperVersion = PROJECT_VERSION;

struct KVPair
{
    std::string key;
    std::string value;
};

QString argumentValue(const QStringList &arguments, const QString &name)
{
    const int index = arguments.indexOf(name);
    if (index < 0 || index + 1 >= arguments.size()) {
        return {};
    }
    return arguments.at(index + 1);
}

bool parseUid(const QString &value, uid_t *uid)
{
    bool ok = false;
    const uint parsed = value.toUInt(&ok, 10);
    if (!ok) {
        return false;
    }
    *uid = static_cast<uid_t>(parsed);
    return true;
}

bool consoleUserUid(uid_t *uid)
{
    QFile console(QStringLiteral("/dev/console"));
    if (!console.exists()) {
        return false;
    }

    struct stat consoleStat {};
    if (stat("/dev/console", &consoleStat) != 0 || consoleStat.st_uid == 0) {
        return false;
    }

    *uid = consoleStat.st_uid;
    return true;
}

bool preparePathParent(const QString &path, uid_t ownerUid, bool hasOwnerUid)
{
    const QFileInfo info(path);
    const QString dirPath = info.absolutePath();
    if (dirPath.isEmpty()) {
        return true;
    }

    QDir dir;
    if (!dir.mkpath(dirPath)) {
        return false;
    }

    chmod(dirPath.toLocal8Bit().constData(), 0700);
    if (hasOwnerUid) {
        chown(dirPath.toLocal8Bit().constData(), ownerUid, static_cast<gid_t>(-1));
    }
    return true;
}

bool ensureTokenFile(const QString &tokenFilePath, uid_t ownerUid, bool hasOwnerUid)
{
    if (!preparePathParent(tokenFilePath, ownerUid, hasOwnerUid)) {
        return false;
    }

    QFile tokenFile(tokenFilePath);
    if (tokenFile.exists()) {
        if (hasOwnerUid) {
            chown(tokenFilePath.toLocal8Bit().constData(), ownerUid, static_cast<gid_t>(-1));
        }
        chmod(tokenFilePath.toLocal8Bit().constData(), 0600);
        return true;
    }

    if (!tokenFile.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        return false;
    }

    const QByteArray token = QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8();
    tokenFile.write(token);
    tokenFile.close();

    if (hasOwnerUid) {
        chown(tokenFilePath.toLocal8Bit().constData(), ownerUid, static_cast<gid_t>(-1));
    }
    chmod(tokenFilePath.toLocal8Bit().constData(), 0600);
    return true;
}

std::string lastError()
{
    const char *errorMsg = nullptr;
    get_error_msg(&errorMsg);
    if (!errorMsg) {
        return "Unknown error";
    }

    const std::string result(errorMsg);
    free_string(errorMsg);
    return result;
}

bool collectInfos(std::vector<KVPair> &infos)
{
    std::vector<KeyValuePair> cInfos(kMaxNetworkInstances);
    const int count = collect_network_infos(cInfos.data(), cInfos.size());
    if (count < 0) {
        return false;
    }

    infos.clear();
    infos.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        KVPair pair;
        if (cInfos[i].key) {
            pair.key = cInfos[i].key;
            free_string(cInfos[i].key);
        }
        if (cInfos[i].value) {
            pair.value = cInfos[i].value;
            free_string(cInfos[i].value);
        }
        infos.push_back(std::move(pair));
    }

    return true;
}

bool retainInstances(const std::vector<std::string> &names)
{
    if (names.empty()) {
        return retain_network_instance(nullptr, 0) == 0;
    }

    std::vector<const char *> cNames;
    cNames.reserve(names.size());
    for (const std::string &name : names) {
        cNames.push_back(name.c_str());
    }
    return retain_network_instance(cNames.data(), cNames.size()) == 0;
}

bool containsName(const std::vector<std::string> &names, const std::string &name)
{
    return std::find(names.begin(), names.end(), name) != names.end();
}

bool removeRuntimeFile(const QString &path)
{
    if (path.isEmpty()) {
        return true;
    }

    QFile file(path);
    return !file.exists() || file.remove();
}

QJsonObject okResponse()
{
    QJsonObject response;
    response.insert(QStringLiteral("success"), true);
    response.insert(QStringLiteral("version"), kProtocolVersion);
    return response;
}

QJsonObject errorResponse(const QString &message)
{
    QJsonObject response;
    response.insert(QStringLiteral("success"), false);
    response.insert(QStringLiteral("version"), kProtocolVersion);
    response.insert(QStringLiteral("error"), message);
    return response;
}

QByteArray responseData(const QJsonObject &response)
{
    return QJsonDocument(response).toJson(QJsonDocument::Compact);
}

class HelperServer : public QObject
{
    Q_OBJECT

public:
    HelperServer(
        QString socketPath,
        QString tokenFilePath,
        QString token,
        uid_t ownerUid,
        bool hasOwnerUid,
        bool allowShutdown,
        bool requireToken,
        QObject *parent = nullptr)
        : QObject(parent)
        , m_socketPath(std::move(socketPath))
        , m_tokenFilePath(std::move(tokenFilePath))
        , m_token(std::move(token))
        , m_ownerUid(ownerUid)
        , m_hasOwnerUid(hasOwnerUid)
        , m_allowShutdown(allowShutdown)
        , m_requireToken(requireToken)
    {
    }

    bool listen()
    {
        const QFileInfo socketInfo(m_socketPath);
        const QString socketDir = socketInfo.absolutePath();
        if (!socketDir.isEmpty()) {
            QDir dir;
            if (!dir.mkpath(socketDir)) {
                return false;
            }
            const bool systemRuntime = socketDir == QStringLiteral("/var/run/qteasytier");
            if (systemRuntime && geteuid() == 0) {
                chown(socketDir.toLocal8Bit().constData(), 0, 0);
                chmod(socketDir.toLocal8Bit().constData(), 0711);
            } else {
                chmod(socketDir.toLocal8Bit().constData(), 0700);
            }
            if (m_hasOwnerUid && !systemRuntime) {
                chown(socketDir.toLocal8Bit().constData(), m_ownerUid, static_cast<gid_t>(-1));
            }
        }

        QLocalServer::removeServer(m_socketPath);
        connect(&m_server, &QLocalServer::newConnection, this, &HelperServer::onNewConnection);
        if (!m_server.listen(m_socketPath)) {
            return false;
        }
        if (m_hasOwnerUid) {
            chown(m_socketPath.toLocal8Bit().constData(), m_ownerUid, static_cast<gid_t>(-1));
            chmod(m_socketPath.toLocal8Bit().constData(), 0600);
        } else {
            chmod(m_socketPath.toLocal8Bit().constData(), 0600);
        }
        return true;
    }

private slots:
    void onNewConnection()
    {
        while (QLocalSocket *socket = m_server.nextPendingConnection()) {
            connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
                socket->setProperty("requestData", socket->property("requestData").toByteArray() + socket->readAll());
                const QByteArray bufferedData = socket->property("requestData").toByteArray();
                if (!bufferedData.contains('\n')) {
                    return;
                }

                const QByteArray requestData = bufferedData.left(bufferedData.indexOf('\n')).trimmed();
                const QJsonObject response = isPeerAllowed(socket)
                    ? handleRequest(requestData)
                    : errorResponse(QStringLiteral("Unauthorized helper peer"));
                socket->write(responseData(response));
                socket->write("\n");
                socket->flush();
                socket->disconnectFromServer();
            });
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        }
    }

private:
    bool isPeerAllowed(QLocalSocket *socket) const
    {
        if (!m_hasOwnerUid) {
            return true;
        }

        uid_t peerUid = 0;
        gid_t peerGid = 0;
        if (getpeereid(static_cast<int>(socket->socketDescriptor()), &peerUid, &peerGid) != 0) {
            return false;
        }

        const bool uidAllowed = peerUid == m_ownerUid || peerUid == 0;
        if (!uidAllowed) {
            return false;
        }

        return true;
    }

    QJsonObject handleRequest(const QByteArray &requestData)
    {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(requestData, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            return errorResponse(QStringLiteral("Invalid helper request"));
        }

        const QJsonObject request = doc.object();
        if (m_requireToken && request.value(QStringLiteral("token")).toString() != m_token) {
            return errorResponse(QStringLiteral("Unauthorized helper request"));
        }

        const int version = request.value(QStringLiteral("version")).toInt(0);
        if (version != kProtocolVersion) {
            return errorResponse(QStringLiteral("Unsupported helper protocol version"));
        }

        const QString command = request.value(QStringLiteral("command")).toString();
        if (command == QStringLiteral("status")) {
            return handleStatus();
        }
        if (command == QStringLiteral("start")) {
            return handleStart(request);
        }
        if (command == QStringLiteral("stop")) {
            return handleStop(request);
        }
        if (command == QStringLiteral("list")) {
            return handleList();
        }
        if (command == QStringLiteral("shutdown")) {
            if (!m_allowShutdown) {
                return errorResponse(QStringLiteral("Helper shutdown is not allowed"));
            }
            if (!m_ownedInstances.empty()) {
                return errorResponse(QStringLiteral("Cannot shut down helper while instances are running"));
            }
            QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
            return okResponse();
        }
        if (command == QStringLiteral("prepare_uninstall")) {
            return handlePrepareUninstall();
        }

        return errorResponse(QStringLiteral("Unknown helper command"));
    }

    QJsonObject handleStatus() const
    {
        QJsonObject response = okResponse();
        response.insert(QStringLiteral("helper_version"), QString::fromLatin1(kHelperVersion));
        response.insert(QStringLiteral("owned_instance_count"), static_cast<int>(m_ownedInstances.size()));
        response.insert(QStringLiteral("client_identity_required"), false);
        return response;
    }

    QJsonObject handleStart(const QJsonObject &request)
    {
        const std::string instName = request.value(QStringLiteral("instance")).toString().toStdString();
        const std::string toml = request.value(QStringLiteral("toml")).toString().toStdString();
        if (instName.empty() || toml.empty()) {
            return errorResponse(QStringLiteral("Missing instance or TOML config"));
        }

        if (containsName(m_ownedInstances, instName)) {
            return errorResponse(QStringLiteral("Instance is already owned by QtEasyTier helper"));
        }

        if (run_network_instance(toml.c_str()) != 0) {
            return errorResponse(QString::fromStdString(lastError()));
        }

        m_ownedInstances.push_back(instName);
        return okResponse();
    }

    QJsonObject handleStop(const QJsonObject &request)
    {
        const std::string instName = request.value(QStringLiteral("instance")).toString().toStdString();
        if (instName.empty()) {
            return errorResponse(QStringLiteral("Missing instance"));
        }

        if (!containsName(m_ownedInstances, instName)) {
            return errorResponse(QStringLiteral("Instance is not owned by QtEasyTier helper"));
        }

        std::vector<std::string> remainingInstances = m_ownedInstances;
        remainingInstances.erase(
            std::remove(remainingInstances.begin(), remainingInstances.end(), instName),
            remainingInstances.end());

        if (!retainInstances(remainingInstances)) {
            return errorResponse(QString::fromStdString(lastError()));
        }

        m_ownedInstances = std::move(remainingInstances);
        return okResponse();
    }

    QJsonObject handleList()
    {
        std::vector<KVPair> infos;
        if (!collectInfos(infos)) {
            return errorResponse(QString::fromStdString(lastError()));
        }

        QJsonArray items;
        for (const std::string &ownedName : m_ownedInstances) {
            QString value = QStringLiteral("{}");
            for (const KVPair &info : infos) {
                if (info.key == ownedName) {
                    value = QString::fromStdString(info.value);
                    break;
                }
            }
            QJsonObject item;
            item.insert(QStringLiteral("key"), QString::fromStdString(ownedName));
            item.insert(QStringLiteral("value"), value);
            items.append(item);
        }

        QJsonObject response = okResponse();
        response.insert(QStringLiteral("infos"), items);
        return response;
    }

    QJsonObject handlePrepareUninstall()
    {
        if (!m_ownedInstances.empty()) {
            return errorResponse(QStringLiteral("Cannot uninstall helper while instances are running"));
        }

        if (!removeRuntimeFile(m_tokenFilePath)) {
            return errorResponse(QStringLiteral("Unable to remove helper token file"));
        }
        removeRuntimeFile(m_socketPath);
        QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
        return okResponse();
    }

    QString m_socketPath;
    QString m_tokenFilePath;
    QString m_token;
    uid_t m_ownerUid = 0;
    bool m_hasOwnerUid = false;
    bool m_allowShutdown = false;
    bool m_requireToken = true;
    QLocalServer m_server;
    std::vector<std::string> m_ownedInstances;
};

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QStringList arguments = app.arguments();

    if (!arguments.contains(QStringLiteral("--daemon"))) {
        std::cerr << "QtEasyTierHelper must be started with --daemon" << std::endl;
        return 2;
    }

    const QString socketPath = argumentValue(arguments, QStringLiteral("--socket"));
    const QString tokenFilePath = argumentValue(arguments, QStringLiteral("--token-file"));
    const QString ownerUidValue = argumentValue(arguments, QStringLiteral("--owner-uid"));
    const bool allowShutdown = arguments.contains(QStringLiteral("--allow-shutdown"));
    if (socketPath.isEmpty() || tokenFilePath.isEmpty()) {
        std::cerr << "QtEasyTierHelper missing --socket or --token-file" << std::endl;
        return 2;
    }

    uid_t ownerUid = 0;
    bool hasOwnerUid = false;
    if (!ownerUidValue.isEmpty()) {
        if (!parseUid(ownerUidValue, &ownerUid)) {
            std::cerr << "QtEasyTierHelper has invalid --owner-uid" << std::endl;
            return 2;
        }
        hasOwnerUid = true;
    } else if (geteuid() == 0 && consoleUserUid(&ownerUid)) {
        hasOwnerUid = true;
    }
    if (geteuid() == 0 && !hasOwnerUid) {
        std::cerr << "QtEasyTierHelper cannot determine console user uid" << std::endl;
        return 2;
    }

    QString token;
#if !QTEASYTIER_ENABLE_PROTOTYPE_HELPER
    std::cerr << kPrototypeHelperDisabledMarker << ": local socket prototype mode is disabled in this build" << std::endl;
    return 2;
#endif
    if (!ensureTokenFile(tokenFilePath, ownerUid, hasOwnerUid)) {
        std::cerr << "QtEasyTierHelper cannot prepare token file" << std::endl;
        return 2;
    }

    QFile tokenFile(tokenFilePath);
    if (!tokenFile.open(QIODevice::ReadOnly)) {
        std::cerr << "QtEasyTierHelper cannot read token file" << std::endl;
        return 2;
    }
    token = QString::fromUtf8(tokenFile.readAll()).trimmed();
    if (token.isEmpty()) {
        std::cerr << "QtEasyTierHelper token file is empty" << std::endl;
        return 2;
    }

    HelperServer server(socketPath, tokenFilePath, token, ownerUid, hasOwnerUid, allowShutdown, true);
    if (!server.listen()) {
        std::cerr << "QtEasyTierHelper failed to listen on " << socketPath.toStdString() << std::endl;
        return 1;
    }

    return app.exec();
}

#include "QtEasyTierHelper.moc"
