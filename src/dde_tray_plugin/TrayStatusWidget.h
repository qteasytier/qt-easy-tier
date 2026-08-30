/** @file TrayStatusWidget.h @brief DDE 托盘状态面板声明 */
#pragma once

#include <QWidget>

#include "TrayStatusTypes.h"

class QLabel;
class QVBoxLayout;
class TrayStatusService;

class TrayStatusWidget final : public QWidget {
    Q_OBJECT
public:
    explicit TrayStatusWidget(TrayStatusService *service, QWidget *parent = nullptr);

private slots:
    void refreshView();

private:
    static QString stateText(ConfigRunState state);
    TrayStatusService *m_service = nullptr;
    QVBoxLayout *m_layout = nullptr;
};
