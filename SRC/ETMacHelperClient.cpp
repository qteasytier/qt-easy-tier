#include "ETMacHelperClient.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QProcess>

#ifdef Q_OS_MACOS
#include <QThread>
#include <QUuid>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace {

constexpr int kConnectTimeoutMs = 800;
constexpr int kRequestTimeoutMs = 30000;
constexpr int kAuthorizationTimeoutMs = 300000;
constexpr int kMaxConnectAttempts = 25;
constexpr int kProtocolVersion = 1;

bool tomlBooleanFlagIsTrue(const std::string &toml, const std::string &flagName)
{
    const std::string needle = flagName + " = true";
    return toml.find(needle) != std::string::npos;
}

bool prototypeHelperAllowed()
{
#ifdef Q_OS_MACOS
#if QTEASYTIER_ENABLE_PROTOTYPE_HELPER
    if (qEnvironmentVariableIsSet("QTEASYTIER_DISABLE_PROTOTYPE_HELPER")) {
        return false;
    }
    return true;
#else
    return false;
#endif
#else
    return false;
#endif
}

QJsonObject baseRequest(const QString &command, const std::string &token)
{
    QJsonObject request;
    request.insert(QStringLiteral("version"), kProtocolVersion);
    request.insert(QStringLiteral("command"), command);
    if (!token.empty()) {
        request.insert(QStringLiteral("token"), QString::fromStdString(token));
    }
    return request;
}

QString helperRuntimeDir(std::string *errorMsg)
{
#ifdef Q_OS_MACOS
    QDir dir(QStringLiteral("/tmp/qteasytier-%1").arg(getuid()));
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (errorMsg) {
            *errorMsg = "Unable to create runtime directory for QtEasyTier helper";
        }
        return {};
    }

    chmod(dir.absolutePath().toLocal8Bit().constData(), 0700);
    return dir.absolutePath();
#else
    Q_UNUSED(errorMsg)
    return {};
#endif
}

QString helperTokenFilePath(std::string *errorMsg)
{
    const QString runtimeDir = helperRuntimeDir(errorMsg);
    if (runtimeDir.isEmpty()) {
        return {};
    }

    return QDir(runtimeDir).filePath(QStringLiteral("helper.token"));
}

QString currentSocketPath()
{
#ifdef Q_OS_MACOS
    const QString runtimeDir = helperRuntimeDir(nullptr);
    if (runtimeDir.isEmpty()) {
        return QStringLiteral("/tmp/qteasytier-helper-%1.sock").arg(getuid());
    }
    return QDir(runtimeDir).filePath(QStringLiteral("helper.sock"));
#else
    return {};
#endif
}

QString currentTokenFilePath(std::string *errorMsg)
{
    return helperTokenFilePath(errorMsg);
}

bool parseResponse(const QByteArray &responseData, QJsonObject *response, std::string *errorMsg)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMsg) {
            *errorMsg = "Invalid helper response";
        }
        return false;
    }

    *response = doc.object();
    const int version = response->value(QStringLiteral("version")).toInt(0);
    if (version != kProtocolVersion) {
        if (errorMsg) {
            *errorMsg = "Unsupported QtEasyTier helper protocol version";
        }
        return false;
    }

    const bool success = response->value(QStringLiteral("success")).toBool(false);
    if (!success && errorMsg) {
        *errorMsg = response->value(QStringLiteral("error")).toString(QStringLiteral("Helper request failed")).toStdString();
    }
    return success;
}

} // namespace

bool ETMacHelperClient::shouldUseHelper(const std::string &tomlConfig)
{
#ifdef Q_OS_MACOS
    if (tomlConfig.empty()) {
        return false;
    }
    if (!prototypeHelperAllowed()) {
        return false;
    }
    return !tomlBooleanFlagIsTrue(tomlConfig, "no_tun");
#else
    Q_UNUSED(tomlConfig)
    return false;
#endif
}

bool ETMacHelperClient::isUsingPrototypeRuntime()
{
#ifdef Q_OS_MACOS
    return prototypeHelperAllowed();
#else
    return false;
#endif
}

ETMacHelperState ETMacHelperClient::queryHelperState(std::string *errorMsg)
{
#ifdef Q_OS_MACOS
    if (errorMsg) {
        errorMsg->clear();
    }

    ETMacHelperState state;

    if (!QFile::exists(QString::fromStdString(helperExecutablePath()))) {
        state.kind = ETMacHelperStateKind::Missing;
        state.errorMsg = "QtEasyTier helper is missing from the app bundle";
        if (errorMsg) {
            *errorMsg = state.errorMsg;
        }
        return state;
    }

    if (!QFile::exists(currentSocketPath())) {
        state.kind = ETMacHelperStateKind::Stopped;
        return state;
    }

    QLocalSocket probe;
    probe.connectToServer(currentSocketPath());
    if (!probe.waitForConnected(kConnectTimeoutMs)) {
        state.kind = ETMacHelperStateKind::Stopped;
        state.errorMsg = "QtEasyTier helper socket is stale";
        return state;
    }
    probe.disconnectFromServer();

    const std::string token = helperToken(errorMsg, false);
    if (token.empty()) {
        state.kind = ETMacHelperStateKind::Unauthorized;
        state.errorMsg = errorMsg && !errorMsg->empty()
            ? *errorMsg
            : "QtEasyTier helper token is unavailable";
        if (errorMsg) {
            *errorMsg = state.errorMsg;
        }
        return state;
    }

    const QJsonObject request = baseRequest(QStringLiteral("status"), token);
    QByteArray responseData;
    std::string requestError;
    if (!sendRequest(QJsonDocument(request).toJson(QJsonDocument::Compact), &responseData, &requestError)) {
        state.kind = ETMacHelperStateKind::Unknown;
        state.errorMsg = requestError;
        if (errorMsg) {
            *errorMsg = state.errorMsg;
        }
        return state;
    }

    QJsonObject response;
    std::string parseError;
    if (!parseResponse(responseData, &response, &parseError)) {
        state.kind = response.value(QStringLiteral("version")).toInt(0) == kProtocolVersion
            ? ETMacHelperStateKind::Unauthorized
            : ETMacHelperStateKind::Incompatible;
        state.errorMsg = parseError;
        if (errorMsg) {
            *errorMsg = state.errorMsg;
        }
        return state;
    }

    state.kind = ETMacHelperStateKind::Available;
    state.status.available = true;
    state.status.protocolVersion = response.value(QStringLiteral("version")).toInt();
    state.status.helperVersion = response.value(QStringLiteral("helper_version")).toString().toStdString();
    state.status.ownedInstanceCount = response.value(QStringLiteral("owned_instance_count")).toInt();
    state.status.clientIdentityRequired = response.value(QStringLiteral("client_identity_required")).toBool(false);
    return state;
#else
    Q_UNUSED(errorMsg)
    ETMacHelperState state;
    state.kind = ETMacHelperStateKind::Missing;
    state.errorMsg = "macOS helper is not available on this platform";
    return state;
#endif
}

bool ETMacHelperClient::status(ETMacHelperStatus &status, std::string *errorMsg)
{
#ifdef Q_OS_MACOS
    status = {};

    if (!QFile::exists(currentSocketPath())) {
        return true;
    }

    const std::string token = helperToken(errorMsg, false);
    if (token.empty()) {
        return false;
    }

    const QJsonObject request = baseRequest(QStringLiteral("status"), token);

    QByteArray responseData;
    if (!sendRequest(QJsonDocument(request).toJson(QJsonDocument::Compact), &responseData, errorMsg)) {
        return false;
    }

    QJsonObject response;
    if (!parseResponse(responseData, &response, errorMsg)) {
        return false;
    }

    status.available = true;
    status.protocolVersion = response.value(QStringLiteral("version")).toInt();
    status.helperVersion = response.value(QStringLiteral("helper_version")).toString().toStdString();
    status.ownedInstanceCount = response.value(QStringLiteral("owned_instance_count")).toInt();
    status.clientIdentityRequired = response.value(QStringLiteral("client_identity_required")).toBool(false);
    return true;
#else
    Q_UNUSED(errorMsg)
    status = {};
    return true;
#endif
}

bool ETMacHelperClient::startNetwork(const std::string &instName, const std::string &tomlConfig, std::string *errorMsg)
{
#ifdef Q_OS_MACOS
    if (!ensureHelperAvailable(errorMsg)) {
        return false;
    }

    const std::string token = helperToken(errorMsg, false);
    if (token.empty()) {
        return false;
    }

    QJsonObject request = baseRequest(QStringLiteral("start"), token);
    request.insert(QStringLiteral("instance"), QString::fromStdString(instName));
    request.insert(QStringLiteral("toml"), QString::fromStdString(tomlConfig));

    QByteArray responseData;
    if (!sendRequest(QJsonDocument(request).toJson(QJsonDocument::Compact), &responseData, errorMsg)) {
        return false;
    }

    QJsonObject response;
    return parseResponse(responseData, &response, errorMsg);
#else
    Q_UNUSED(instName)
    Q_UNUSED(tomlConfig)
    if (errorMsg) {
        *errorMsg = "macOS helper is not available on this platform";
    }
    return false;
#endif
}

bool ETMacHelperClient::stopNetwork(const std::string &instName, std::string *errorMsg)
{
#ifdef Q_OS_MACOS
    ETMacHelperState state = queryHelperState(errorMsg);
    if (state.kind != ETMacHelperStateKind::Available) {
        if (errorMsg && errorMsg->empty()) {
            *errorMsg = state.errorMsg.empty()
                ? "QtEasyTier helper is not running"
                : state.errorMsg;
        }
        return false;
    }

    const std::string token = helperToken(errorMsg, false);
    if (token.empty()) {
        return false;
    }

    QJsonObject request = baseRequest(QStringLiteral("stop"), token);
    request.insert(QStringLiteral("instance"), QString::fromStdString(instName));

    QByteArray responseData;
    if (!sendRequest(QJsonDocument(request).toJson(QJsonDocument::Compact), &responseData, errorMsg)) {
        return false;
    }

    QJsonObject response;
    return parseResponse(responseData, &response, errorMsg);
#else
    Q_UNUSED(instName)
    if (errorMsg) {
        *errorMsg = "macOS helper is not available on this platform";
    }
    return false;
#endif
}

bool ETMacHelperClient::collectInfos(std::vector<ETMacHelperInfo> &infos, std::string *errorMsg)
{
#ifdef Q_OS_MACOS
    infos.clear();

    if (!QFile::exists(currentSocketPath())) {
        return true;
    }

    const std::string token = helperToken(errorMsg, false);
    if (token.empty()) {
        return false;
    }

    const QJsonObject request = baseRequest(QStringLiteral("list"), token);

    QByteArray responseData;
    if (!sendRequest(QJsonDocument(request).toJson(QJsonDocument::Compact), &responseData, errorMsg)) {
        return false;
    }

    QJsonObject response;
    if (!parseResponse(responseData, &response, errorMsg)) {
        return false;
    }

    const QJsonArray items = response.value(QStringLiteral("infos")).toArray();
    infos.reserve(static_cast<size_t>(items.size()));
    for (const QJsonValue &itemValue : items) {
        const QJsonObject item = itemValue.toObject();
        ETMacHelperInfo info;
        info.key = item.value(QStringLiteral("key")).toString().toStdString();
        info.value = item.value(QStringLiteral("value")).toString().toStdString();
        if (!info.key.empty()) {
            infos.push_back(std::move(info));
        }
    }

    return true;
#else
    Q_UNUSED(errorMsg)
    infos.clear();
    return true;
#endif
}

bool ETMacHelperClient::shutdown(std::string *errorMsg)
{
#ifdef Q_OS_MACOS
    if (!QFile::exists(currentSocketPath())) {
        return true;
    }

    const std::string token = helperToken(errorMsg, false);
    if (token.empty()) {
        return false;
    }

    const QJsonObject request = baseRequest(QStringLiteral("shutdown"), token);
    QByteArray responseData;
    if (!sendRequest(QJsonDocument(request).toJson(QJsonDocument::Compact), &responseData, errorMsg)) {
        return false;
    }

    QJsonObject response;
    return parseResponse(responseData, &response, errorMsg);
#else
    Q_UNUSED(errorMsg)
    return true;
#endif
}

bool ETMacHelperClient::prepareUninstall(std::string *errorMsg)
{
#ifdef Q_OS_MACOS
    const ETMacHelperState state = queryHelperState(errorMsg);
    if (state.kind == ETMacHelperStateKind::Missing || state.kind == ETMacHelperStateKind::Stopped) {
        return true;
    }
    if (state.kind != ETMacHelperStateKind::Available) {
        if (errorMsg && errorMsg->empty()) {
            *errorMsg = state.errorMsg.empty()
                ? "QtEasyTier helper state cannot be verified"
                : state.errorMsg;
        }
        return false;
    }
    if (state.status.ownedInstanceCount > 0) {
        if (errorMsg) {
            *errorMsg = "QtEasyTier helper still has running network instances";
        }
        return false;
    }

    const std::string token = helperToken(errorMsg, false);
    if (token.empty()) {
        return false;
    }

    const QJsonObject request = baseRequest(QStringLiteral("prepare_uninstall"), token);
    QByteArray responseData;
    if (!sendRequest(QJsonDocument(request).toJson(QJsonDocument::Compact), &responseData, errorMsg)) {
        return false;
    }

    QJsonObject response;
    return parseResponse(responseData, &response, errorMsg);
#else
    Q_UNUSED(errorMsg)
    return true;
#endif
}

bool ETMacHelperClient::ensureHelperAvailable(std::string *errorMsg)
{
#ifdef Q_OS_MACOS
    ETMacHelperState state = queryHelperState(errorMsg);
    if (state.kind == ETMacHelperStateKind::Available) {
        return true;
    }

    if (state.kind == ETMacHelperStateKind::Incompatible || state.kind == ETMacHelperStateKind::Unauthorized) {
        if (errorMsg && errorMsg->empty()) {
            *errorMsg = state.errorMsg.empty()
                ? "QtEasyTier helper is unavailable"
                : state.errorMsg;
        }
        return false;
    }

    if (!prototypeHelperAllowed()) {
        if (errorMsg && errorMsg->empty()) {
            *errorMsg = "QtEasyTier prototype helper fallback is disabled in this build";
        }
        return false;
    }

    return ensurePrototypeHelperRunning(errorMsg);
#else
    Q_UNUSED(errorMsg)
    return false;
#endif
}

bool ETMacHelperClient::ensurePrototypeHelperRunning(std::string *errorMsg)
{
#ifdef Q_OS_MACOS
#if !QTEASYTIER_ENABLE_PROTOTYPE_HELPER
    if (errorMsg) {
        *errorMsg = "QtEasyTier prototype helper fallback is disabled in this build";
    }
    return false;
#else
    const QString helperPath = QString::fromStdString(helperExecutablePath());
    if (!QFile::exists(helperPath)) {
        if (errorMsg) {
            *errorMsg = "QtEasyTier helper is missing from the app bundle";
        }
        return false;
    }

    std::string tokenError;
    const std::string token = helperToken(&tokenError, true);
    if (token.empty()) {
        if (errorMsg) {
            *errorMsg = tokenError;
        }
        return false;
    }

    QString escapedHelperPath = helperPath;
    QString escapedSocketPath = QDir(helperRuntimeDir(errorMsg)).filePath(QStringLiteral("helper.sock"));
    escapedHelperPath.replace(QStringLiteral("'"), QStringLiteral("'\\''"));
    escapedSocketPath.replace(QStringLiteral("'"), QStringLiteral("'\\''"));

    QString escapedTokenPath = helperTokenFilePath(errorMsg);
    if (escapedTokenPath.isEmpty()) {
        return false;
    }
    escapedTokenPath.replace(QStringLiteral("'"), QStringLiteral("'\\''"));

    const QString shellCommand = QStringLiteral(
        "nohup '%1' --daemon --socket '%2' --token-file '%3' --owner-uid '%4' --allow-shutdown >/dev/null 2>&1 &")
        .arg(escapedHelperPath,
            escapedSocketPath,
            escapedTokenPath,
            QString::number(static_cast<qulonglong>(getuid())));
    QString escapedShellCommand = shellCommand;
    escapedShellCommand.replace(QStringLiteral("\""), QStringLiteral("\\\""));

    const QString script = QStringLiteral("do shell script \"%1\" with administrator privileges")
        .arg(escapedShellCommand);

    QProcess process;
    process.start(QStringLiteral("/usr/bin/osascript"), {QStringLiteral("-e"), script});
    if (!process.waitForFinished(kAuthorizationTimeoutMs)) {
        process.kill();
        if (errorMsg) {
            *errorMsg = "Timed out while waiting for administrator authorization for QtEasyTier helper";
        }
        return false;
    }

    if (process.exitCode() != 0) {
        if (errorMsg) {
            const QByteArray stderrData = process.readAllStandardError();
            const QByteArray stdoutData = process.readAllStandardOutput();
            const QByteArray output = stderrData.isEmpty() ? stdoutData : stderrData;
            *errorMsg = output.isEmpty()
                ? "Administrator authorization was cancelled or failed"
                : output.toStdString();
        }
        return false;
    }

    for (int attempt = 0; attempt < kMaxConnectAttempts; ++attempt) {
        std::string stateError;
        ETMacHelperState state = queryHelperState(&stateError);
        if (state.kind == ETMacHelperStateKind::Available) {
            return true;
        }
        if (state.kind == ETMacHelperStateKind::Incompatible || state.kind == ETMacHelperStateKind::Unauthorized) {
            if (errorMsg) {
                *errorMsg = state.errorMsg.empty()
                    ? "QtEasyTier helper is unavailable after startup"
                    : state.errorMsg;
            }
            return false;
        }
        QThread::msleep(200);
    }

    if (errorMsg) {
        *errorMsg = "QtEasyTier helper did not become available";
    }
    return false;
#endif
#else
    Q_UNUSED(errorMsg)
    return false;
#endif
}

bool ETMacHelperClient::sendRequest(const QByteArray &requestData, QByteArray *responseData, std::string *errorMsg)
{
#ifdef Q_OS_MACOS
    QLocalSocket socket;
    socket.connectToServer(currentSocketPath());
    if (!socket.waitForConnected(kRequestTimeoutMs)) {
        if (errorMsg) {
            *errorMsg = "Unable to connect to QtEasyTier helper";
        }
        return false;
    }

    QByteArray payload = requestData;
    payload.append('\n');
    socket.write(payload);
    if (!socket.waitForBytesWritten(kRequestTimeoutMs)) {
        if (errorMsg) {
            *errorMsg = "Unable to send request to QtEasyTier helper";
        }
        return false;
    }

    if (!socket.waitForReadyRead(kRequestTimeoutMs)) {
        if (errorMsg) {
            *errorMsg = "Timed out waiting for QtEasyTier helper response";
        }
        return false;
    }

    QByteArray response;
    while (socket.bytesAvailable() > 0 || socket.waitForReadyRead(50)) {
        response.append(socket.readAll());
        if (response.endsWith('\n')) {
            break;
        }
    }

    if (responseData) {
        *responseData = response.trimmed();
    }
    return true;
#else
    Q_UNUSED(requestData)
    Q_UNUSED(responseData)
    if (errorMsg) {
        *errorMsg = "macOS helper is not available on this platform";
    }
    return false;
#endif
}

std::string ETMacHelperClient::helperToken(std::string *errorMsg, bool createIfMissing)
{
#ifdef Q_OS_MACOS
    const QString tokenPath = currentTokenFilePath(errorMsg);
    if (tokenPath.isEmpty()) {
        return {};
    }

    QFile tokenFile(tokenPath);
    if (tokenFile.exists()) {
        if (tokenFile.open(QIODevice::ReadOnly)) {
            return tokenFile.readAll().trimmed().toStdString();
        }
        if (errorMsg) {
            *errorMsg = "Unable to read QtEasyTier helper token";
        }
        return {};
    }

    if (!createIfMissing) {
        if (errorMsg) {
            *errorMsg = "QtEasyTier helper token is missing";
        }
        return {};
    }

    const QByteArray token = QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8();
    if (!tokenFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMsg) {
            *errorMsg = "Unable to write QtEasyTier helper token";
        }
        return {};
    }
    tokenFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    tokenFile.write(token);
    tokenFile.close();
    chmod(tokenPath.toLocal8Bit().constData(), 0600);
    return token.toStdString();
#else
    Q_UNUSED(errorMsg)
    return {};
#endif
}

std::string ETMacHelperClient::socketPath()
{
#ifdef Q_OS_MACOS
    const QString runtimeDir = helperRuntimeDir(nullptr);
    if (runtimeDir.isEmpty()) {
        return QStringLiteral("/tmp/qteasytier-helper-%1.sock").arg(getuid()).toStdString();
    }
    return QDir(runtimeDir).filePath(QStringLiteral("helper.sock")).toStdString();
#else
    return {};
#endif
}

std::string ETMacHelperClient::helperExecutablePath()
{
#ifdef Q_OS_MACOS
    const QDir appDir(QCoreApplication::applicationDirPath());
    return appDir.filePath(QStringLiteral("QtEasyTierHelper")).toStdString();
#else
    return {};
#endif
}
