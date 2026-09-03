/** @file TrayStatusWidget.cpp @brief DDE 托盘状态面板实现 */
#include "TrayStatusWidget.h"

#include "TrayStatusService.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {

/// 指示灯圆点颜色：与 Theme.qml 状态色一致；停止态取系统次级灰自适应深浅主题
QColor dotColor(ConfigRunState state, const QPalette &palette)
{
    switch (state) {
    case ConfigRunState::Running: return QColor(QStringLiteral("#4caf50"));
    case ConfigRunState::Starting:
    case ConfigRunState::Stopping: return QColor(QStringLiteral("#ff9800"));
    case ConfigRunState::Error: return QColor(QStringLiteral("#f44336"));
    case ConfigRunState::Stopped: return palette.mid().color();
    }
    return palette.mid().color();
}

/// 创建指示灯圆点
QLabel *createStatusDot(ConfigRunState state, const QPalette &palette)
{
    auto *dot = new QLabel;
    dot->setObjectName(QStringLiteral("statusDot"));
    dot->setFixedSize(12, 12);
    dot->setStyleSheet(QStringLiteral("background: %1; border-radius: 6px;")
                           .arg(dotColor(state, palette).name()));
    return dot;
}

} // namespace

TrayStatusWidget::TrayStatusWidget(TrayStatusService *service, QWidget *parent)
    : QWidget(parent), m_service(service)
{
    setWindowTitle(tr("QtEasyTier 网络状态"));
    setMinimumWidth(360);
    // 面板整体最大高度固定为 500px，实例较多时由列表滚动区承载
    setMaximumHeight(500);
    setStyleSheet(QStringLiteral(R"(
QLabel#panelTitle {
    font-size: 22px;
    font-weight: bold;
    color: palette(highlight);
}
QLabel#cardTitle {
    font-size: 14px;
    font-weight: bold;
}
QLabel#cardStatus {
    font-size: 12px;
    color: palette(placeholderText);
}
QLabel#panelSummary {
    font-size: 12px;
    color: palette(placeholderText);
}
QLabel#emptyHint {
    font-size: 13px;
    color: palette(placeholderText);
}
)"));
    m_layout = new QVBoxLayout(this);
    connect(m_service, &TrayStatusService::snapshotChanged,
            this, &TrayStatusWidget::refreshView);
    refreshView();
}

QString TrayStatusWidget::stateText(ConfigRunState state)
{
    switch (state) {
    case ConfigRunState::Starting: return tr("启动中");
    case ConfigRunState::Running: return tr("运行中");
    case ConfigRunState::Stopping: return tr("停止中");
    case ConfigRunState::Error: return tr("运行错误");
    case ConfigRunState::Stopped: return tr("已停止");
    }
    return tr("未知");
}

void TrayStatusWidget::refreshView()
{
    while (QLayoutItem *item = m_layout->takeAt(0)) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    const auto snapshot = m_service->snapshot();
    const bool daemonConnected =
        snapshot.daemonState == DaemonClient::ConnectionState::Connected;

    // 标题栏（仿主程序实例列表页标题样式，左侧带应用图标，上下间距放宽）
    auto *header = new QWidget(this);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 8, 0, 12);
    headerLayout->setSpacing(8);
    auto *logo = new QLabel(header);
    logo->setPixmap(QPixmap(QStringLiteral(":/dde-tray/qtet.png"))
                        .scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    headerLayout->addWidget(logo);
    auto *title = new QLabel(tr("QtEasyTier 托盘插件"), header);
    title->setObjectName(QStringLiteral("panelTitle"));
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    m_layout->addWidget(header);

    // 实例列表滚动区：实例较多时在固定高度面板内滚动展示
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 滚动区及其内容背景透明，融入面板底色，仅实例卡片保留蒙版背景
    scroll->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; border: none; }"));
    auto *listContent = new QWidget(scroll);
    listContent->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *listLayout = new QVBoxLayout(listContent);
    listLayout->setContentsMargins(0, 0, 0, 0);
    scroll->setWidget(listContent);
    m_layout->addWidget(scroll, 1);

    // 空状态提示（纯文字不可点击）
    if (snapshot.instances.isEmpty()) {
        listLayout->addSpacing(8);
        auto *empty = new QLabel(tr("暂无运行实例"), listContent);
        empty->setObjectName(QStringLiteral("emptyHint"));
        empty->setAlignment(Qt::AlignHCenter);
        listLayout->addWidget(empty);
    }

    // 实例卡片列表（仿主程序 InstanceList 卡片式设计，置于滚动区内）
    for (const auto &instance : snapshot.instances) {
        auto *card = new QFrame(listContent);
        card->setObjectName(QStringLiteral("instanceCard"));
        // 背景：主题 base 色 30% 透明蒙版；
        const QColor baseColor = palette().color(QPalette::Base);
        const QColor textColor = palette().color(QPalette::WindowText);
        card->setStyleSheet(QStringLiteral(
            "QFrame#instanceCard {"
            " background: rgba(%1, %2, %3, 0.3);"
            " border: 1px solid rgba(%4, %5, %6, 0.15);"
            " border-radius: 6px;"
            "}")
            .arg(baseColor.red()).arg(baseColor.green()).arg(baseColor.blue())
            .arg(textColor.red()).arg(textColor.green()).arg(textColor.blue()));
        auto *cardLayout = new QHBoxLayout(card);
        cardLayout->setContentsMargins(12, 10, 12, 10);
        cardLayout->setSpacing(10);

        // 指示灯圆点
        cardLayout->addWidget(createStatusDot(instance.state, palette()));

        // 名称 + 状态文案列
        auto *infoLayout = new QVBoxLayout;
        infoLayout->setSpacing(2);
        auto *nameLabel = new QLabel(instance.displayName, card);
        nameLabel->setObjectName(QStringLiteral("cardTitle"));
        // 外部实例名称用高亮色突出显示（与主程序一致）
        if (!instance.local)
            nameLabel->setStyleSheet(QStringLiteral("color: palette(highlight);"));
        infoLayout->addWidget(nameLabel);

        // 状态文案：运行中显示节点连接数（节点数未就绪时退化为纯状态文本）
        QString statusText;
        if (instance.state == ConfigRunState::Running) {
            const QString nodePart = instance.nodeCount
                ? tr(" · %1 个节点连接").arg(*instance.nodeCount) : QString();
            statusText = instance.local ? tr("运行中") + nodePart
                                        : tr("运行中 · 外部实例") + nodePart;
        } else {
            statusText = stateText(instance.state);
        }
        auto *statusLabel = new QLabel(statusText, card);
        statusLabel->setObjectName(QStringLiteral("cardStatus"));
        infoLayout->addWidget(statusLabel);
        cardLayout->addLayout(infoLayout);
        cardLayout->addStretch();

        // 启停按钮：内部 Stopped 可启动，Running 可停止；过渡状态无按钮
        if (instance.state == ConfigRunState::Stopped && instance.local) {
            auto *button = new QPushButton(tr("启动"), card);
            button->setEnabled(daemonConnected);
            connect(button, &QPushButton::clicked, this, [this, name = instance.instanceName] {
                m_service->startInstance(name);
            });
            cardLayout->addWidget(button);
        } else if (instance.state == ConfigRunState::Running) {
            auto *button = new QPushButton(tr("停止"), card);
            button->setEnabled(daemonConnected);
            connect(button, &QPushButton::clicked, this, [this, name = instance.instanceName] {
                m_service->stopInstance(name);
            });
            cardLayout->addWidget(button);
        }
        listLayout->addWidget(card);
    }

    listLayout->addStretch();

    // 后端与运行实例汇总，置于滚动区下方（不显示总连接数）
    m_layout->addSpacing(12);
    const QString daemonText = snapshot.daemonState == DaemonClient::ConnectionState::Connected
        ? tr("已连接")
        : snapshot.daemonState == DaemonClient::ConnectionState::Connecting
            ? tr("连接中") : tr("未连接");
    int running = 0;
    for (const auto &instance : snapshot.instances) {
        if (instance.state == ConfigRunState::Running)
            ++running;
    }
    auto *summary = new QLabel(tr("后端：%1 · 运行实例：%2 个")
                                   .arg(daemonText).arg(running), this);
    summary->setObjectName(QStringLiteral("panelSummary"));
    m_layout->addWidget(summary);
}
