/**
 * @file DangerousOperationViewModel.cpp
 * @brief DangerousOperationViewModel 实现（薄壳）
 *
 * 所有属性和操作全部委托 DangerousOperationService，
 * 构造时转发其信号为 QML 可绑定的属性通知。
 */
#include "DangerousOperationViewModel.h"

#include "core/application/dangerous/DangerousOperationService.h"

DangerousOperationViewModel::DangerousOperationViewModel(DangerousOperationService *service, QObject *parent)
    : QObject(parent)
    , m_service(service)
{
    if (!m_service)
        return;

    connect(m_service, &DangerousOperationService::busyChanged,
            this, &DangerousOperationViewModel::busyChanged);
    connect(m_service, &DangerousOperationService::daemonStatusChanged,
            this, &DangerousOperationViewModel::daemonStatusChanged);
    connect(m_service, &DangerousOperationService::operationFinished,
            this, &DangerousOperationViewModel::operationFinished);
    connect(m_service, &DangerousOperationService::quitRequested,
            this, &DangerousOperationViewModel::quitRequested);
}

bool DangerousOperationViewModel::busy() const
{
    return m_service && m_service->busy();
}

bool DangerousOperationViewModel::daemonInstalled() const
{
    return m_service && m_service->daemonInstalled();
}

bool DangerousOperationViewModel::daemonOperationEnabled() const
{
    return m_service && m_service->daemonOperationEnabled();
}

void DangerousOperationViewModel::refreshDaemonStatus()
{
    if (m_service)
        m_service->refreshDaemonStatus();
}

void DangerousOperationViewModel::performDaemonOperation()
{
    if (m_service)
        m_service->performDaemonOperation();
}

void DangerousOperationViewModel::clearAllData()
{
    if (m_service)
        m_service->clearAllData();
}
