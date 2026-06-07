#ifndef QTEASYTIER_ETMACHELPERSERVICEMANAGER_H
#define QTEASYTIER_ETMACHELPERSERVICEMANAGER_H

#include <string>

enum class ETMacHelperServiceBundleStateKind
{
    Unsupported,
    NotBundled,
    HelperExecutableMissing,
    HelperExecutableNotExecutable,
    LaunchDaemonPlistMissing,
    ReadyForRegistration,
    Unknown
};

enum class ETMacHelperServiceRegistrationStateKind
{
    Unsupported,
    NotBundled,
    BundleNotReady,
    NotRegistered,
    Enabled,
    RequiresApproval,
    NotFound,
    Unknown
};

struct ETMacHelperServiceBundleState
{
    ETMacHelperServiceBundleStateKind kind = ETMacHelperServiceBundleStateKind::Unknown;
    std::string label;
    std::string helperPath;
    std::string plistPath;
    std::string errorMsg;
};

struct ETMacHelperServiceRegistrationState
{
    ETMacHelperServiceRegistrationStateKind kind = ETMacHelperServiceRegistrationStateKind::Unknown;
    ETMacHelperServiceBundleState bundleState;
    std::string label;
    std::string errorMsg;
};

class ETMacHelperServiceManager
{
public:
    static std::string helperLabel();
    static std::string helperPlistName();
    static ETMacHelperServiceBundleState queryBundleState();
    static ETMacHelperServiceRegistrationState queryRegistrationState();
    static bool registerService(std::string *errorMsg = nullptr);
    static bool unregisterService(std::string *errorMsg = nullptr);
};

#endif // QTEASYTIER_ETMACHELPERSERVICEMANAGER_H
