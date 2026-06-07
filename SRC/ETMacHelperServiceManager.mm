#include "ETMacHelperServiceManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#ifdef Q_OS_MACOS
#import <Foundation/Foundation.h>
#import <ServiceManagement/ServiceManagement.h>
#endif

namespace
{
constexpr const char *kHelperLabel = "io.github.qteasytier.QtEasyTier.Helper";
constexpr const char *kHelperPlistName = "io.github.qteasytier.QtEasyTier.Helper.plist";
constexpr const char *kHelperExecutableName = "QtEasyTierHelper";

QString appBundlePath()
{
#ifdef Q_OS_MACOS
    QDir dir(QCoreApplication::applicationDirPath());
    if (!dir.cdUp() || dir.dirName() != QStringLiteral("Contents")) {
        return {};
    }
    if (!dir.cdUp() || !dir.dirName().endsWith(QStringLiteral(".app"))) {
        return {};
    }
    return dir.absolutePath();
#else
    return {};
#endif
}

#ifdef Q_OS_MACOS
NSString *helperPlistNSString()
{
    return [NSString stringWithUTF8String:kHelperPlistName];
}

std::string nsErrorToString(NSError *error)
{
    if (!error) {
        return {};
    }
    NSString *message = error.localizedDescription ?: error.description;
    return message ? message.UTF8String : "Unknown ServiceManagement error";
}

ETMacHelperServiceRegistrationStateKind registrationKindFromStatus(SMAppServiceStatus status)
{
    switch (status) {
    case SMAppServiceStatusNotRegistered:
        return ETMacHelperServiceRegistrationStateKind::NotRegistered;
    case SMAppServiceStatusEnabled:
        return ETMacHelperServiceRegistrationStateKind::Enabled;
    case SMAppServiceStatusRequiresApproval:
        return ETMacHelperServiceRegistrationStateKind::RequiresApproval;
    case SMAppServiceStatusNotFound:
        return ETMacHelperServiceRegistrationStateKind::NotFound;
    default:
        return ETMacHelperServiceRegistrationStateKind::Unknown;
    }
}

SMAppService *helperService()
{
    if (@available(macOS 13.0, *)) {
        return [SMAppService daemonServiceWithPlistName:helperPlistNSString()];
    }
    return nil;
}
#endif
}

std::string ETMacHelperServiceManager::helperLabel()
{
    return kHelperLabel;
}

std::string ETMacHelperServiceManager::helperPlistName()
{
    return kHelperPlistName;
}

ETMacHelperServiceBundleState ETMacHelperServiceManager::queryBundleState()
{
    ETMacHelperServiceBundleState state;
    state.label = kHelperLabel;

#ifndef Q_OS_MACOS
    state.kind = ETMacHelperServiceBundleStateKind::Unsupported;
    state.errorMsg = "SMAppService helper lifecycle is only available on macOS";
    return state;
#else
    const QString bundlePath = appBundlePath();
    if (bundlePath.isEmpty()) {
        state.kind = ETMacHelperServiceBundleStateKind::NotBundled;
        state.errorMsg = "QtEasyTier is not running from a macOS app bundle";
        return state;
    }

    const QString helperPath = QDir(bundlePath).filePath(
        QStringLiteral("Contents/MacOS/%1").arg(QString::fromLatin1(kHelperExecutableName)));
    state.helperPath = helperPath.toStdString();

    QFileInfo helperInfo(helperPath);
    if (!helperInfo.isFile()) {
        state.kind = ETMacHelperServiceBundleStateKind::HelperExecutableMissing;
        state.errorMsg = "Bundled helper executable is missing";
        return state;
    }
    if (!helperInfo.isExecutable()) {
        state.kind = ETMacHelperServiceBundleStateKind::HelperExecutableNotExecutable;
        state.errorMsg = "Bundled helper executable is not executable";
        return state;
    }

    const QString plistPath = QDir(bundlePath).filePath(
        QStringLiteral("Contents/Library/LaunchDaemons/%1").arg(QString::fromLatin1(kHelperPlistName)));
    state.plistPath = plistPath.toStdString();

    QFileInfo plistInfo(plistPath);
    if (!plistInfo.isFile()) {
        state.kind = ETMacHelperServiceBundleStateKind::LaunchDaemonPlistMissing;
        state.errorMsg = "Bundled LaunchDaemon plist is missing";
        return state;
    }

    state.kind = ETMacHelperServiceBundleStateKind::ReadyForRegistration;
    return state;
#endif
}

ETMacHelperServiceRegistrationState ETMacHelperServiceManager::queryRegistrationState()
{
    ETMacHelperServiceRegistrationState state;
    state.label = kHelperLabel;
    state.bundleState = queryBundleState();

#ifndef Q_OS_MACOS
    state.kind = ETMacHelperServiceRegistrationStateKind::Unsupported;
    state.errorMsg = "SMAppService helper lifecycle is only available on macOS";
    return state;
#else
    if (state.bundleState.kind == ETMacHelperServiceBundleStateKind::Unsupported) {
        state.kind = ETMacHelperServiceRegistrationStateKind::Unsupported;
        state.errorMsg = state.bundleState.errorMsg;
        return state;
    }
    if (state.bundleState.kind == ETMacHelperServiceBundleStateKind::NotBundled) {
        state.kind = ETMacHelperServiceRegistrationStateKind::NotBundled;
        state.errorMsg = state.bundleState.errorMsg;
        return state;
    }
    if (state.bundleState.kind != ETMacHelperServiceBundleStateKind::ReadyForRegistration) {
        state.kind = ETMacHelperServiceRegistrationStateKind::BundleNotReady;
        state.errorMsg = state.bundleState.errorMsg;
        return state;
    }

    if (@available(macOS 13.0, *)) {
        SMAppService *service = helperService();
        state.kind = registrationKindFromStatus(service.status);
        return state;
    }

    state.kind = ETMacHelperServiceRegistrationStateKind::Unsupported;
    state.errorMsg = "SMAppService daemon registration requires macOS 13 or newer";
    return state;
#endif
}

bool ETMacHelperServiceManager::registerService(std::string *errorMsg)
{
#ifndef Q_OS_MACOS
    if (errorMsg) {
        *errorMsg = "SMAppService helper lifecycle is only available on macOS";
    }
    return false;
#else
    const ETMacHelperServiceBundleState bundleState = queryBundleState();
    if (bundleState.kind != ETMacHelperServiceBundleStateKind::ReadyForRegistration) {
        if (errorMsg) {
            *errorMsg = bundleState.errorMsg.empty()
                ? "QtEasyTier helper is not ready for registration"
                : bundleState.errorMsg;
        }
        return false;
    }

    if (@available(macOS 13.0, *)) {
        NSError *error = nil;
        const BOOL ok = [helperService() registerAndReturnError:&error];
        if (!ok && errorMsg) {
            *errorMsg = nsErrorToString(error);
        }
        return ok;
    }

    if (errorMsg) {
        *errorMsg = "SMAppService daemon registration requires macOS 13 or newer";
    }
    return false;
#endif
}

bool ETMacHelperServiceManager::unregisterService(std::string *errorMsg)
{
#ifndef Q_OS_MACOS
    if (errorMsg) {
        *errorMsg = "SMAppService helper lifecycle is only available on macOS";
    }
    return false;
#else
    if (@available(macOS 13.0, *)) {
        NSError *error = nil;
        const BOOL ok = [helperService() unregisterAndReturnError:&error];
        if (!ok && errorMsg) {
            *errorMsg = nsErrorToString(error);
        }
        return ok;
    }

    if (errorMsg) {
        *errorMsg = "SMAppService daemon registration requires macOS 13 or newer";
    }
    return false;
#endif
}
