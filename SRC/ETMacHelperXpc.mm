#include "ETMacHelperXpc.h"

#include <dispatch/dispatch.h>
#include <xpc/xpc.h>

namespace {

constexpr const char *kPayloadKey = "payload";
constexpr int kXpcTimeoutMs = 30000;
ETMacHelperXpcHandler g_handler;

std::string xpcErrorDescription(xpc_object_t error)
{
    if (!error) {
        return "Unknown XPC error";
    }

    const char *description = xpc_dictionary_get_string(error, XPC_ERROR_KEY_DESCRIPTION);
    return description ? description : "Unknown XPC error";
}

void setError(std::string *errorMsg, const std::string &message)
{
    if (errorMsg) {
        *errorMsg = message;
    }
}

void handleClientConnection(xpc_connection_t client)
{
    xpc_connection_set_event_handler(client, ^(xpc_object_t event) {
        if (xpc_get_type(event) == XPC_TYPE_ERROR) {
            return;
        }
        if (xpc_get_type(event) != XPC_TYPE_DICTIONARY) {
            xpc_connection_cancel(client);
            return;
        }

        size_t payloadLength = 0;
        const void *payload = xpc_dictionary_get_data(event, kPayloadKey, &payloadLength);
        if (!payload || payloadLength == 0) {
            xpc_connection_send_message(client, xpc_dictionary_create_empty());
            return;
        }

        QByteArray request(static_cast<const char *>(payload), static_cast<int>(payloadLength));
        const QByteArray response = g_handler ? g_handler(request) : QByteArray();

        xpc_object_t reply = xpc_dictionary_create_reply(event);
        if (!reply) {
            reply = xpc_dictionary_create_empty();
        }
        if (!response.isEmpty()) {
            xpc_dictionary_set_data(reply, kPayloadKey, response.constData(), static_cast<size_t>(response.size()));
        }
        xpc_connection_send_message(client, reply);
        xpc_release(reply);
    });
    xpc_connection_resume(client);
}

} // namespace

bool ETMacHelperXpcStartServer(
    const QString &serviceName,
    const std::string &clientRequirement,
    ETMacHelperXpcHandler handler,
    std::string *errorMsg)
{
    if (clientRequirement.empty()) {
        setError(errorMsg, "QtEasyTier helper XPC listener requires a client signing requirement");
        return false;
    }

    g_handler = std::move(handler);

    xpc_connection_t listener = xpc_connection_create_mach_service(
        serviceName.toUtf8().constData(),
        dispatch_get_main_queue(),
        XPC_CONNECTION_MACH_SERVICE_LISTENER);
    if (!listener) {
        setError(errorMsg, "Unable to create QtEasyTier helper XPC listener");
        return false;
    }

    if (@available(macOS 13.0, *)) {
        const int result = xpc_connection_set_peer_code_signing_requirement(
            listener,
            clientRequirement.c_str());
        if (result != 0) {
            xpc_release(listener);
            setError(errorMsg, "Unable to apply QtEasyTier helper XPC client signing requirement");
            return false;
        }
    } else {
        xpc_release(listener);
        setError(errorMsg, "QtEasyTier helper XPC client signing requirement requires macOS 13 or newer");
        return false;
    }

    xpc_connection_set_event_handler(listener, ^(xpc_object_t event) {
        if (xpc_get_type(event) == XPC_TYPE_CONNECTION) {
            handleClientConnection(static_cast<xpc_connection_t>(event));
        }
    });

    xpc_connection_resume(listener);
    return true;
}

bool ETMacHelperXpcSendRequest(
    const QString &serviceName,
    const QByteArray &requestData,
    QByteArray *responseData,
    std::string *errorMsg)
{
    if (responseData) {
        responseData->clear();
    }

    xpc_connection_t connection = xpc_connection_create_mach_service(
        serviceName.toUtf8().constData(),
        dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0),
        XPC_CONNECTION_MACH_SERVICE_PRIVILEGED);
    if (!connection) {
        setError(errorMsg, "Unable to create QtEasyTier helper XPC connection");
        return false;
    }

    __block bool finished = false;
    __block bool success = false;
    __block std::string blockError;
    __block QByteArray blockResponse;
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);

    xpc_connection_set_event_handler(connection, ^(xpc_object_t event) {
        if (xpc_get_type(event) == XPC_TYPE_ERROR && !finished) {
            blockError = xpcErrorDescription(event);
            finished = true;
            dispatch_semaphore_signal(semaphore);
        }
    });
    xpc_connection_resume(connection);

    xpc_object_t request = xpc_dictionary_create_empty();
    xpc_dictionary_set_data(request, kPayloadKey, requestData.constData(), static_cast<size_t>(requestData.size()));

    xpc_connection_send_message_with_reply(connection, request, dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^(xpc_object_t reply) {
        if (xpc_get_type(reply) == XPC_TYPE_ERROR) {
            blockError = xpcErrorDescription(reply);
            finished = true;
            dispatch_semaphore_signal(semaphore);
            return;
        }

        size_t payloadLength = 0;
        const void *payload = xpc_dictionary_get_data(reply, kPayloadKey, &payloadLength);
        if (!payload || payloadLength == 0) {
            blockError = "Invalid QtEasyTier helper XPC response";
            finished = true;
            dispatch_semaphore_signal(semaphore);
            return;
        }

        blockResponse = QByteArray(static_cast<const char *>(payload), static_cast<int>(payloadLength));
        success = true;
        finished = true;
        dispatch_semaphore_signal(semaphore);
    });
    xpc_release(request);

    const dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(kXpcTimeoutMs) * NSEC_PER_MSEC);
    const long waitResult = dispatch_semaphore_wait(semaphore, timeout);

    xpc_connection_cancel(connection);
    xpc_release(connection);

    if (waitResult != 0 || !finished) {
        setError(errorMsg, "Timed out waiting for QtEasyTier helper XPC response");
        return false;
    }
    if (!success) {
        setError(errorMsg, blockError.empty() ? "QtEasyTier helper XPC request failed" : blockError);
        return false;
    }

    if (responseData) {
        *responseData = blockResponse;
    }
    return true;
}
