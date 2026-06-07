#ifndef QTEASYTIER_ETMACHELPERXPC_H
#define QTEASYTIER_ETMACHELPERXPC_H

#include <QByteArray>
#include <QString>

#include <functional>
#include <string>

using ETMacHelperXpcHandler = std::function<QByteArray(const QByteArray &)>;

bool ETMacHelperXpcStartServer(
    const QString &serviceName,
    const std::string &clientRequirement,
    ETMacHelperXpcHandler handler,
    std::string *errorMsg = nullptr);

bool ETMacHelperXpcSendRequest(
    const QString &serviceName,
    const QByteArray &requestData,
    QByteArray *responseData,
    std::string *errorMsg = nullptr);

#endif // QTEASYTIER_ETMACHELPERXPC_H
