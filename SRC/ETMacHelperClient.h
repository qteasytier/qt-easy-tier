#ifndef QTEASYTIER_ETMACHELPERCLIENT_H
#define QTEASYTIER_ETMACHELPERCLIENT_H

#include <QByteArray>

#include <string>
#include <vector>

struct ETMacHelperInfo
{
    std::string key;
    std::string value;
};

struct ETMacHelperStatus
{
    bool available = false;
    int protocolVersion = 0;
    std::string helperVersion;
    int ownedInstanceCount = 0;
    bool clientIdentityRequired = false;
};

enum class ETMacHelperStateKind
{
    Missing,
    Stopped,
    Available,
    Incompatible,
    Unauthorized,
    Unknown
};

struct ETMacHelperState
{
    ETMacHelperStateKind kind = ETMacHelperStateKind::Unknown;
    ETMacHelperStatus status;
    std::string errorMsg;
};

class ETMacHelperClient
{
public:
    static bool shouldUseHelper(const std::string &tomlConfig);
    static bool isUsingPrototypeRuntime();

    ETMacHelperState queryHelperState(std::string *errorMsg = nullptr);
    bool status(ETMacHelperStatus &status, std::string *errorMsg = nullptr);
    bool startNetwork(const std::string &instName, const std::string &tomlConfig, std::string *errorMsg = nullptr);
    bool stopNetwork(const std::string &instName, std::string *errorMsg = nullptr);
    bool collectInfos(std::vector<ETMacHelperInfo> &infos, std::string *errorMsg = nullptr);
    bool shutdown(std::string *errorMsg = nullptr);
    bool prepareUninstall(std::string *errorMsg = nullptr);

private:
    bool ensureHelperAvailable(std::string *errorMsg);
    bool ensurePrototypeHelperRunning(std::string *errorMsg);
    bool sendRequest(const QByteArray &requestData, QByteArray *responseData, std::string *errorMsg);
    static std::string helperToken(std::string *errorMsg, bool createIfMissing = true);
    static std::string socketPath();
    static std::string helperExecutablePath();
};

#endif // QTEASYTIER_ETMACHELPERCLIENT_H
