//
// Created by YMHuang on 2026/4/7.
//

#include "qtetsettings.h"
#include "qtetcheckbtn.h"
#include "qtetpushbtn.h"
#include "qtetdrawutils.h"
#ifdef Q_OS_MACOS
#include "ETMacHelperClient.h"
#include "ETMacHelperServiceManager.h"
#endif

#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QTimer>
#include <QPixmap>
#include <QFontDatabase>
#include <QScrollArea>
#include <QFrame>
#include <iostream>

#ifdef Q_OS_MACOS
namespace {
QString toQString(const std::string &value)
{
    return QString::fromStdString(value);
}
QString bundleStateText(ETMacHelperServiceBundleStateKind kind)
{
    switch (kind) {
    case ETMacHelperServiceBundleStateKind::Unsupported:
        return QObject::tr("当前平台不支持");
    case ETMacHelperServiceBundleStateKind::NotBundled:
        return QObject::tr("未从 .app Bundle 运行");
    case ETMacHelperServiceBundleStateKind::HelperExecutableMissing:
        return QObject::tr("缺少 helper 可执行文件");
    case ETMacHelperServiceBundleStateKind::HelperExecutableNotExecutable:
        return QObject::tr("helper 不可执行");
    case ETMacHelperServiceBundleStateKind::LaunchDaemonPlistMissing:
        return QObject::tr("缺少 LaunchDaemon plist");
    case ETMacHelperServiceBundleStateKind::ReadyForRegistration:
        return QObject::tr("Bundle 已就绪");
    case ETMacHelperServiceBundleStateKind::Unknown:
        return QObject::tr("Bundle 状态未知");
    }
    return QObject::tr("Bundle 状态未知");
}
QString registrationStateText(ETMacHelperServiceRegistrationStateKind kind)
{
    switch (kind) {
    case ETMacHelperServiceRegistrationStateKind::Unsupported:
        return QObject::tr("不支持 SMAppService");
    case ETMacHelperServiceRegistrationStateKind::NotBundled:
        return QObject::tr("未从 .app Bundle 运行");
    case ETMacHelperServiceRegistrationStateKind::BundleNotReady:
        return QObject::tr("Bundle 未就绪");
    case ETMacHelperServiceRegistrationStateKind::NotRegistered:
        return QObject::tr("未安装");
    case ETMacHelperServiceRegistrationStateKind::Enabled:
        return QObject::tr("已安装");
    case ETMacHelperServiceRegistrationStateKind::RequiresApproval:
        return QObject::tr("等待管理员批准");
    case ETMacHelperServiceRegistrationStateKind::NotFound:
        return QObject::tr("服务未找到");
    case ETMacHelperServiceRegistrationStateKind::Unknown:
        return QObject::tr("注册状态未知");
    }
    return QObject::tr("注册状态未知");
}
} // namespace
#endif

QtETSettings::QtETSettings(QWidget *parent)
    : QWidget(parent)
{
    // 加载设置
    loadSettings();
    
    // 初始化界面
    initUI();
    
    // 连接信号槽
    setupConnections();
}

// ==================== 初始化相关 ====================

void QtETSettings::initUI()
{
    // 设置字体大小
    QFont font;
    font.setPointSize(10);
    setFont(font);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // ========== 标题栏区域 ==========
    auto *titleWidget = new QWidget(this);
    auto *titleLayout = new QHBoxLayout(titleWidget);
    titleLayout->setSpacing(15);
    titleLayout->setContentsMargins(16, 15, 16, 6);

    // 左侧图标
    auto *iconLabel = new QLabel(titleWidget);
    iconLabel->setMaximumSize(48, 48);
    iconLabel->setMinimumSize(48, 48);
    iconLabel->setPixmap(QPixmap(":/icons/icon.ico"));
    iconLabel->setScaledContents(true);

    // 标题文字
    auto *titleLabel = new QLabel(titleWidget);
    QFont titleFont;
    titleFont.setPointSize(20);
    titleLabel->setFont(titleFont);
    titleLabel->setText(tr("设置"));

    titleLabel->setAlignment(Qt::AlignCenter);

    titleLayout->addWidget(iconLabel, 0, Qt::AlignTop);
    titleLayout->addWidget(titleLabel, 0, Qt::AlignHCenter | Qt::AlignVCenter);

    mainLayout->addWidget(titleWidget, 0, Qt::AlignTop | Qt::AlignLeft);

    // ========== 滚动区域：设置选项 ==========
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QtETSmoothScroll::install(scrollArea);

    // 设置选项内容容器
    auto *settingsWidget = new QWidget();
    auto *settingsLayout = new QVBoxLayout(settingsWidget);
    settingsLayout->setSpacing(10);
    settingsLayout->setContentsMargins(20, 10, 20, 10);

    // 创建开关按钮
    m_hideOnTrayCheckBox = new QtETCheckBtn(tr("关闭时隐藏到托盘"), this);
    m_autoReconnectCheckBox = new QtETCheckBtn(tr("自动回连"), this);
    m_autoCheckUpdateCheckBox = new QtETCheckBtn(tr("自动检查更新"), this);
    m_autoStartCheckBox = new QtETCheckBtn(tr("开机自启"), this);

    // 设置字体
    QFont checkBoxFont;
    checkBoxFont.setPointSize(11);
    m_hideOnTrayCheckBox->setFont(checkBoxFont);
    m_autoReconnectCheckBox->setFont(checkBoxFont);
    m_autoCheckUpdateCheckBox->setFont(checkBoxFont);
    m_autoStartCheckBox->setFont(checkBoxFont);
    m_hideOnTrayCheckBox->setObjectName(QStringLiteral("settings.hideOnTrayCheckBox"));
    m_hideOnTrayCheckBox->setAccessibleName(QStringLiteral("settings.hideOnTrayCheckBox"));
    m_autoReconnectCheckBox->setObjectName(QStringLiteral("settings.autoReconnectCheckBox"));
    m_autoReconnectCheckBox->setAccessibleName(QStringLiteral("settings.autoReconnectCheckBox"));
    m_autoCheckUpdateCheckBox->setObjectName(QStringLiteral("settings.autoCheckUpdateCheckBox"));
    m_autoCheckUpdateCheckBox->setAccessibleName(QStringLiteral("settings.autoCheckUpdateCheckBox"));
    m_autoStartCheckBox->setObjectName(QStringLiteral("settings.autoStartCheckBox"));
    m_autoStartCheckBox->setAccessibleName(QStringLiteral("settings.autoStartCheckBox"));

    // 设置简要提示
    m_hideOnTrayCheckBox->setBriefTip(tr("关闭窗口时最小化到系统托盘而不是退出程序"));
    m_autoReconnectCheckBox->setBriefTip(tr("程序启动时自动连接上次退出时正在运行的网络"));
    m_autoCheckUpdateCheckBox->setBriefTip(tr("程序启动时自动检查是否有新版本"));
    m_autoStartCheckBox->setBriefTip(tr("系统开机时自动启动程序"));

    // 设置初始状态
    m_hideOnTrayCheckBox->setChecked(m_hideOnTray);
    m_autoReconnectCheckBox->setChecked(m_autoReconnect);
    m_autoCheckUpdateCheckBox->setChecked(m_autoCheckUpdate);
    m_autoStartCheckBox->setChecked(m_autoStart);

    // 版本信息区域
    auto *versionWidget = new QWidget(settingsWidget);
    auto *versionLayout = new QHBoxLayout(versionWidget);
    versionLayout->setSpacing(10);
    versionLayout->setContentsMargins(0, 0, 0, 0);

    // 构建版本字符串
    QString versionStr = tr("QtEasyTier 版本：") + QString::fromUtf8(PROJECT_VERSION);
#if IS_BETA_VERSION
    versionStr += " beta";
#endif
    versionStr += QString(" (EasyTier %1)").arg(ET_VERSION);

    m_versionLabel = new QLabel(versionStr, this);
    QFont versionFont;
    versionFont.setPointSize(10);
    m_versionLabel->setFont(versionFont);
    m_versionLabel->setAlignment(Qt::AlignLeft);

    m_checkUpdateBtn = new QtETPushBtn(tr("检查更新"), this);
    m_checkUpdateBtn->setObjectName(QStringLiteral("settings.checkUpdateButton"));
    m_checkUpdateBtn->setAccessibleName(QStringLiteral("settings.checkUpdateButton"));
    m_checkUpdateBtn->setFixedWidth(120);

    versionLayout->addWidget(m_versionLabel);
    versionLayout->addStretch();
    versionLayout->addWidget(m_checkUpdateBtn);

    // 添加到布局（垂直布局，一行一个）
#ifdef Q_OS_MACOS
    initMacHelperServiceSection(settingsLayout);
#endif
    settingsLayout->addWidget(m_hideOnTrayCheckBox);
    settingsLayout->addWidget(m_autoReconnectCheckBox);
    settingsLayout->addWidget(m_autoCheckUpdateCheckBox);
    settingsLayout->addWidget(m_autoStartCheckBox);
    settingsLayout->addWidget(versionWidget);
    settingsLayout->addStretch();  // 弹簧让内容向上对齐

    // 将 settingsWidget 放入 scrollArea
    scrollArea->setWidget(settingsWidget);

    mainLayout->addWidget(scrollArea, 1);  // stretch=1 让滚动区域占据剩余空间

    // ========== 底部区域：操作按钮 ==========
    auto *bottomWidget = new QWidget(this);
    auto *bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setSpacing(10);
    bottomLayout->setContentsMargins(20, 10, 20, 10);

    // 操作按钮区域
    auto *buttonWidget = new QWidget(bottomWidget);
    auto *buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setSpacing(10);
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    buttonLayout->addStretch();

    m_discardBtn = new QtETPushBtn(tr("丢弃"), this);
    m_discardBtn->setObjectName(QStringLiteral("settings.discardButton"));
    m_discardBtn->setAccessibleName(QStringLiteral("settings.discardButton"));
    m_discardBtn->setFixedWidth(80);
    m_discardBtn->setEnabled(false);

    m_saveBtn = new QtETPushBtn(tr("保存"), this);
    m_saveBtn->setObjectName(QStringLiteral("settings.saveButton"));
    m_saveBtn->setAccessibleName(QStringLiteral("settings.saveButton"));
    m_saveBtn->setFixedWidth(80);
    m_saveBtn->setEnabled(false);

    buttonLayout->addWidget(m_discardBtn);
    buttonLayout->addWidget(m_saveBtn);

    bottomLayout->addWidget(buttonWidget);

    mainLayout->addWidget(bottomWidget);
}

void QtETSettings::setupConnections()
{
    // 复选框状态变化
    connect(m_hideOnTrayCheckBox, &QCheckBox::checkStateChanged, this, &QtETSettings::onCheckBoxStateChanged);
    connect(m_autoReconnectCheckBox, &QCheckBox::checkStateChanged, this, &QtETSettings::onCheckBoxStateChanged);
    connect(m_autoCheckUpdateCheckBox, &QCheckBox::checkStateChanged, this, &QtETSettings::onCheckBoxStateChanged);
    connect(m_autoStartCheckBox, &QCheckBox::checkStateChanged, this, &QtETSettings::onCheckBoxStateChanged);

    // 按钮点击
    connect(m_saveBtn, &QPushButton::clicked, this, &QtETSettings::onSaveButtonClicked);
    connect(m_discardBtn, &QPushButton::clicked, this, &QtETSettings::onDiscardButtonClicked);
    connect(m_checkUpdateBtn, &QPushButton::clicked, this, &QtETSettings::onCheckUpdateButtonClicked);
#ifdef Q_OS_MACOS
    connect(m_macHelperRefreshBtn, &QPushButton::clicked, this, &QtETSettings::onRefreshMacHelperClicked);
    connect(m_macHelperInstallBtn, &QPushButton::clicked, this, &QtETSettings::onInstallMacHelperClicked);
    connect(m_macHelperUninstallBtn, &QPushButton::clicked, this, &QtETSettings::onUninstallMacHelperClicked);
#endif
}

// ==================== UI 事件处理 ====================

void QtETSettings::onCheckBoxStateChanged()
{
    updateButtonState();
}

void QtETSettings::onSaveButtonClicked()
{
    // 处理开机自启设置变更
    bool newAutoStart = m_autoStartCheckBox->isChecked();
    if (m_lastSavedAutoStart != newAutoStart) {
        if (!setAutoStart(newAutoStart)) {
            // 设置失败，恢复复选框状态
            m_autoStartCheckBox->setChecked(m_lastSavedAutoStart);
            return;
        }
    }

    // 保存设置
    saveSettings();

    // 更新上次保存的值
    m_lastSavedHideOnTray = m_hideOnTray;
    m_lastSavedAutoReconnect = m_autoReconnect;
    m_lastSavedAutoCheckUpdate = m_autoCheckUpdate;
    m_lastSavedAutoStart = m_autoStart;

    // 更新按钮状态
    updateButtonState();

    // 发送设置更改信号
    emit settingsChanged();

    // 如果隐藏到托盘设置改变了，发送专门信号
    if (m_hideOnTray != m_lastSavedHideOnTray) {
        emit hideOnTrayChanged(m_hideOnTray);
    }

    // 显示保存成功提示
    QMessageBox::information(this, tr("保存成功"), tr("设置已保存"));
}

void QtETSettings::onDiscardButtonClicked()
{
    // 恢复到上次保存的设置
    discardChanges();
}

void QtETSettings::onCheckUpdateButtonClicked()
{
    checkForUpdate(this, false);
}

#ifdef Q_OS_MACOS
void QtETSettings::onRefreshMacHelperClicked()
{
    refreshMacHelperServiceState();
}
void QtETSettings::onInstallMacHelperClicked()
{
    QString preflightMessage;
    if (!canChangeMacHelperService(&preflightMessage)) {
        QMessageBox::warning(this, tr("无法变更辅助服务"), preflightMessage);
        refreshMacHelperServiceState();
        return;
    }
    const ETMacHelperServiceRegistrationState state = ETMacHelperServiceManager::queryRegistrationState();
    if (state.kind == ETMacHelperServiceRegistrationStateKind::Enabled
        || state.kind == ETMacHelperServiceRegistrationStateKind::RequiresApproval) {
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this,
            tr("重新安装辅助服务"),
            tr("将先卸载当前辅助服务注册信息，然后重新注册当前 App Bundle 内的 helper。继续吗？"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            return;
        }
        QString prepareMessage;
        if (!prepareMacHelperForUnregister(&prepareMessage)) {
            QMessageBox::warning(this, tr("无法准备辅助服务"), prepareMessage);
            refreshMacHelperServiceState();
            return;
        }
        std::string unregisterError;
        if (!ETMacHelperServiceManager::unregisterService(&unregisterError)) {
            QMessageBox::warning(
                this,
                tr("卸载失败"),
                tr("无法卸载当前辅助服务：%1").arg(toQString(unregisterError)));
            refreshMacHelperServiceState();
            return;
        }
    }
    std::string registerError;
    if (!ETMacHelperServiceManager::registerService(&registerError)) {
        QMessageBox::warning(
            this,
            tr("安装失败"),
            tr("无法注册辅助服务：%1").arg(toQString(registerError)));
        refreshMacHelperServiceState();
        return;
    }
    refreshMacHelperServiceState();
    QMessageBox::information(
        this,
        tr("辅助服务已注册"),
        tr("辅助服务已提交给 macOS。若状态显示“等待管理员批准”，请在系统设置中批准 QtEasyTier 后再使用 TUN 网络。"));
}
void QtETSettings::onUninstallMacHelperClicked()
{
    QString preflightMessage;
    if (!canChangeMacHelperService(&preflightMessage)) {
        QMessageBox::warning(this, tr("无法卸载辅助服务"), preflightMessage);
        refreshMacHelperServiceState();
        return;
    }
    const QMessageBox::StandardButton answer = QMessageBox::question(
        this,
        tr("卸载辅助服务"),
        tr("卸载后 QtEasyTier 将不能通过正式辅助服务创建 TUN 网络，直到重新安装。继续吗？"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    QString prepareMessage;
    if (!prepareMacHelperForUnregister(&prepareMessage)) {
        QMessageBox::warning(this, tr("无法准备辅助服务"), prepareMessage);
        refreshMacHelperServiceState();
        return;
    }
    std::string unregisterError;
    if (!ETMacHelperServiceManager::unregisterService(&unregisterError)) {
        QMessageBox::warning(
            this,
            tr("卸载失败"),
            tr("无法卸载辅助服务：%1").arg(toQString(unregisterError)));
        refreshMacHelperServiceState();
        return;
    }
    refreshMacHelperServiceState();
    QMessageBox::information(this, tr("辅助服务已卸载"), tr("辅助服务注册信息已移除。"));
}
#endif
void QtETSettings::updateButtonState()
{
    bool hasChanges = hasUnsavedChanges();
    m_saveBtn->setEnabled(hasChanges);
    m_discardBtn->setEnabled(hasChanges);
}

#ifdef Q_OS_MACOS
void QtETSettings::initMacHelperServiceSection(QVBoxLayout *settingsLayout)
{
    m_macHelperFrame = new QFrame(this);
    m_macHelperFrame->setObjectName(QStringLiteral("settings.macHelperFrame"));
    m_macHelperFrame->setAccessibleName(QStringLiteral("settings.macHelperFrame"));
    m_macHelperFrame->setFrameShape(QFrame::StyledPanel);
    auto *helperLayout = new QVBoxLayout(m_macHelperFrame);
    helperLayout->setSpacing(8);
    helperLayout->setContentsMargins(12, 10, 12, 10);
    auto *titleLabel = new QLabel(tr("macOS TUN 辅助服务"), m_macHelperFrame);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    m_macHelperStateLabel = new QLabel(m_macHelperFrame);
    m_macHelperStateLabel->setObjectName(QStringLiteral("settings.macHelperStateLabel"));
    m_macHelperStateLabel->setAccessibleName(QStringLiteral("settings.macHelperStateLabel"));
    m_macHelperStateLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_macHelperDetailLabel = new QLabel(m_macHelperFrame);
    m_macHelperDetailLabel->setObjectName(QStringLiteral("settings.macHelperDetailLabel"));
    m_macHelperDetailLabel->setAccessibleName(QStringLiteral("settings.macHelperDetailLabel"));
    m_macHelperDetailLabel->setWordWrap(true);
    m_macHelperDetailLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto *buttonRow = new QWidget(m_macHelperFrame);
    auto *buttonLayout = new QHBoxLayout(buttonRow);
    buttonLayout->setSpacing(8);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    m_macHelperRefreshBtn = new QtETPushBtn(tr("刷新"), buttonRow);
    m_macHelperRefreshBtn->setObjectName(QStringLiteral("settings.macHelperRefreshButton"));
    m_macHelperRefreshBtn->setAccessibleName(QStringLiteral("settings.macHelperRefreshButton"));
    m_macHelperRefreshBtn->setFixedWidth(90);
    m_macHelperInstallBtn = new QtETPushBtn(tr("安装/升级"), buttonRow);
    m_macHelperInstallBtn->setObjectName(QStringLiteral("settings.macHelperInstallButton"));
    m_macHelperInstallBtn->setAccessibleName(QStringLiteral("settings.macHelperInstallButton"));
    m_macHelperInstallBtn->setFixedWidth(110);
#if QTEASYTIER_ENABLE_PROTOTYPE_HELPER
    m_macHelperInstallBtn->hide();
#endif
    m_macHelperUninstallBtn = new QtETPushBtn(tr("卸载"), buttonRow);
    m_macHelperUninstallBtn->setObjectName(QStringLiteral("settings.macHelperUninstallButton"));
    m_macHelperUninstallBtn->setAccessibleName(QStringLiteral("settings.macHelperUninstallButton"));
    m_macHelperUninstallBtn->setFixedWidth(90);
#if QTEASYTIER_ENABLE_PROTOTYPE_HELPER
    m_macHelperUninstallBtn->hide();
#endif
    buttonLayout->addWidget(m_macHelperRefreshBtn);
    buttonLayout->addWidget(m_macHelperInstallBtn);
    buttonLayout->addWidget(m_macHelperUninstallBtn);
    buttonLayout->addStretch();
    helperLayout->addWidget(titleLabel);
    helperLayout->addWidget(m_macHelperStateLabel);
    helperLayout->addWidget(m_macHelperDetailLabel);
    helperLayout->addWidget(buttonRow);
    settingsLayout->addWidget(m_macHelperFrame);
    refreshMacHelperServiceState();
}
void QtETSettings::refreshMacHelperServiceState()
{
    if (!m_macHelperStateLabel || !m_macHelperDetailLabel) {
        return;
    }
    const ETMacHelperServiceRegistrationState registrationState =
        ETMacHelperServiceManager::queryRegistrationState();
    const ETMacHelperServiceBundleState &bundleState = registrationState.bundleState;
    QString statusText = tr("状态：%1").arg(registrationStateText(registrationState.kind));
    QStringList details;
    details << tr("Bundle：%1").arg(bundleStateText(bundleState.kind));
    details << tr("Label：%1").arg(toQString(registrationState.label));
    if (!bundleState.helperPath.empty()) {
        details << tr("Helper：%1").arg(toQString(bundleState.helperPath));
    }
    if (!bundleState.plistPath.empty()) {
        details << tr("Plist：%1").arg(toQString(bundleState.plistPath));
    }
    if (!registrationState.errorMsg.empty()) {
        details << tr("错误：%1").arg(toQString(registrationState.errorMsg));
    } else if (!bundleState.errorMsg.empty()) {
        details << tr("错误：%1").arg(toQString(bundleState.errorMsg));
    }
    if (registrationState.kind == ETMacHelperServiceRegistrationStateKind::Enabled) {
        details << tr("运行：已注册，未在设置页加载时同步查询运行态。");
        details << tr("提示：安装、升级或卸载辅助服务前会检查运行中的 helper 实例。");
    }
#if QTEASYTIER_ENABLE_PROTOTYPE_HELPER
    details << tr("社区测试包使用临时 helper 创建 TUN 网络，不提供正式辅助服务安装/卸载操作。");
#endif
    if (registrationState.kind == ETMacHelperServiceRegistrationStateKind::RequiresApproval) {
        details << tr("请在系统设置中批准 QtEasyTier 的后台项目/登录项后再使用 TUN 网络。");
    }
    const bool bundleReady = bundleState.kind == ETMacHelperServiceBundleStateKind::ReadyForRegistration;
    const bool lifecycleAvailable =
        registrationState.kind != ETMacHelperServiceRegistrationStateKind::Unsupported
        && registrationState.kind != ETMacHelperServiceRegistrationStateKind::NotBundled
        && registrationState.kind != ETMacHelperServiceRegistrationStateKind::BundleNotReady;
    const bool registered = registrationState.kind == ETMacHelperServiceRegistrationStateKind::Enabled
        || registrationState.kind == ETMacHelperServiceRegistrationStateKind::RequiresApproval;
    m_macHelperStateLabel->setText(statusText);
    const QString detailText = details.join(QStringLiteral("\n"));
    m_macHelperDetailLabel->setText(detailText);
    m_macHelperStateLabel->setAccessibleDescription(statusText);
    m_macHelperDetailLabel->setAccessibleDescription(detailText);
    if (m_macHelperFrame) {
        m_macHelperFrame->setAccessibleDescription(statusText + QStringLiteral("\n") + detailText);
    }
#if QTEASYTIER_ENABLE_PROTOTYPE_HELPER
    m_macHelperInstallBtn->setEnabled(false);
    m_macHelperUninstallBtn->setEnabled(false);
#else
    m_macHelperInstallBtn->setEnabled(bundleReady && lifecycleAvailable);
    m_macHelperUninstallBtn->setEnabled(registered);
#endif
}
bool QtETSettings::canChangeMacHelperService(QString *message) const
{
    const ETMacHelperServiceRegistrationState registrationState =
        ETMacHelperServiceManager::queryRegistrationState();
    if (registrationState.kind != ETMacHelperServiceRegistrationStateKind::Enabled) {
        return true;
    }
    ETMacHelperClient helperClient;
    std::string errorMsg;
    const ETMacHelperState helperState = helperClient.queryHelperState(&errorMsg);
    if (helperState.kind != ETMacHelperStateKind::Available) {
        return true;
    }
    if (helperState.status.ownedInstanceCount > 0) {
        if (message) {
            *message = tr("当前辅助服务仍有 %1 个运行中的网络实例。请先停止这些网络，再安装、升级或卸载辅助服务。")
                .arg(helperState.status.ownedInstanceCount);
        }
        return false;
    }
    return true;
}
bool QtETSettings::prepareMacHelperForUnregister(QString *message) const
{
    const ETMacHelperServiceRegistrationState registrationState =
        ETMacHelperServiceManager::queryRegistrationState();
    if (registrationState.kind != ETMacHelperServiceRegistrationStateKind::Enabled) {
        return true;
    }
    ETMacHelperClient helperClient;
    std::string errorMsg;
    if (helperClient.prepareUninstall(&errorMsg)) {
        return true;
    }
    std::string queryError;
    const ETMacHelperState helperState = helperClient.queryHelperState(&queryError);
    if (helperState.kind != ETMacHelperStateKind::Available) {
        return true;
    }
    if (message) {
        *message = errorMsg.empty()
            ? tr("无法确认辅助服务是否仍有运行中的网络实例。请先停止所有网络，或重启后再试。")
            : tr("无法准备辅助服务变更：%1").arg(toQString(errorMsg));
    }
    return false;
}
#endif
// ==================== 设置读写 ====================

QString QtETSettings::getConfigPath()
{
#if SAVE_CONF_IN_APP_DIR == true
    return QCoreApplication::applicationDirPath() + "/config";
#else
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
#endif
}

QString QtETSettings::getPublicServersPath()
{
    return QCoreApplication::applicationDirPath() + "/publicservers.json";
}

QJsonObject QtETSettings::loadSettingsFromFile()
{
    QString configPath = getConfigPath();
    QDir dir(configPath);

    // 确保目录存在
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString settingsFile = configPath + "/settings2.json";

    QFile file(settingsFile);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return QJsonObject();
    }

    return doc.object();
}

bool QtETSettings::saveSettingsToFile(const QJsonObject &settings)
{
    QString configPath = getConfigPath();
    QDir dir(configPath);

    // 确保目录存在
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            return false;
        }
    }

    QString settingsFile = configPath + "/settings2.json";
    QJsonDocument doc(settings);

    QFile file(settingsFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

void QtETSettings::loadSettings()
{
    QJsonObject settings = loadSettingsFromFile();

    m_hideOnTray = settings.value("hide_on_tray").toBool(true);
    m_autoReconnect = settings.value("auto_reconnect").toBool(false);
    m_autoCheckUpdate = settings.value("auto_check_update").toBool(true);
    m_autoStart = settings.value("auto_start").toBool(false);

    // 更新上次保存的值
    m_lastSavedHideOnTray = m_hideOnTray;
    m_lastSavedAutoReconnect = m_autoReconnect;
    m_lastSavedAutoCheckUpdate = m_autoCheckUpdate;
    m_lastSavedAutoStart = m_autoStart;
}

void QtETSettings::saveSettings()
{
    // 从 UI 读取当前值
    m_hideOnTray = m_hideOnTrayCheckBox->isChecked();
    m_autoReconnect = m_autoReconnectCheckBox->isChecked();
    m_autoCheckUpdate = m_autoCheckUpdateCheckBox->isChecked();
    m_autoStart = m_autoStartCheckBox->isChecked();

    // 保存到文件
    QJsonObject settings = loadSettingsFromFile();
    settings["hide_on_tray"] = m_hideOnTray;
    settings["auto_reconnect"] = m_autoReconnect;
    settings["auto_check_update"] = m_autoCheckUpdate;
    settings["auto_start"] = m_autoStart;

    if (!saveSettingsToFile(settings)) {
        QMessageBox::warning(this, tr("错误"), tr("无法保存设置"));
    }
}

void QtETSettings::discardChanges()
{
    // 恢复复选框状态
    m_hideOnTrayCheckBox->setChecked(m_lastSavedHideOnTray);
    m_autoReconnectCheckBox->setChecked(m_lastSavedAutoReconnect);
    m_autoCheckUpdateCheckBox->setChecked(m_lastSavedAutoCheckUpdate);
    m_autoStartCheckBox->setChecked(m_lastSavedAutoStart);

    // 恢复内部值
    m_hideOnTray = m_lastSavedHideOnTray;
    m_autoReconnect = m_lastSavedAutoReconnect;
    m_autoCheckUpdate = m_lastSavedAutoCheckUpdate;
    m_autoStart = m_lastSavedAutoStart;

    // 更新按钮状态
    updateButtonState();
}

bool QtETSettings::hasUnsavedChanges() const
{
    return m_hideOnTrayCheckBox->isChecked() != m_lastSavedHideOnTray ||
           m_autoReconnectCheckBox->isChecked() != m_lastSavedAutoReconnect ||
           m_autoCheckUpdateCheckBox->isChecked() != m_lastSavedAutoCheckUpdate ||
           m_autoStartCheckBox->isChecked() != m_lastSavedAutoStart;
}

// ==================== 静态方法 ====================

bool QtETSettings::isHideOnTray()
{
    const QJsonObject settings = loadSettingsFromFile();
    return settings.value("hide_on_tray").toBool(true);
}

bool QtETSettings::isAutoReconnect()
{
    const QJsonObject settings = loadSettingsFromFile();
    return settings.value("auto_reconnect").toBool(false);
}

bool QtETSettings::isAutoStart()
{
    const QJsonObject settings = loadSettingsFromFile();
    return settings.value("auto_start").toBool(false);
}

bool QtETSettings::isAutoCheckUpdate()
{
    const QJsonObject settings = loadSettingsFromFile();
    return settings.value("auto_check_update").toBool(true);
}

bool QtETSettings::setAutoStart(bool enable)
{
#ifdef Q_OS_WIN
    // Windows 平台：使用任务计划程序实现开机自启
    const QString appName = "QtEasyTier";
    const QString appPath = QCoreApplication::applicationFilePath().replace("/", "\\");
    QProcess process;

    QStringList args;
    if (enable) {
        // 创建开机自启任务
        args << "/create"
             << "/tn" << appName
             << "/tr" << QString("\"%1\" --auto-start").arg(appPath)
             << "/sc" << "onlogon"
             << "/rl" << "highest"
             << "/f";
    } else {
        // 删除开机自启任务
        args << "/delete"
             << "/tn" << appName
             << "/f";
    }

    process.start("schtasks.exe", args);

    if (!process.waitForFinished(5000)) {
        std::clog << QString("开机自启任务失败：命令执行超时").toStdString() << std::endl;
        return false;
    }

    if (process.exitCode() != 0) {
        QString error = QString::fromLocal8Bit(process.readAllStandardError());
        std::clog << QString("开机自启任务失败：%1").arg(error).toStdString() << std::endl;
        return false;
    }

    std::clog << QString("%1自启任务成功").arg(enable ? "创建" : "删除").toStdString() << std::endl;

#elif defined(Q_OS_LINUX)
    // Linux 平台：使用 freedesktop.org autostart 规范
    const QString appName = "QtEasyTier";
    const QString appPath = QCoreApplication::applicationFilePath();

    // 获取 autostart 目录路径
    QString configHome = qEnvironmentVariable("XDG_CONFIG_HOME");
    if (configHome.isEmpty()) {
        configHome = QDir::homePath() + "/.config";
    }
    QString autostartDir = configHome + "/autostart";
    QString desktopFilePath = autostartDir + "/" + appName + ".desktop";

    if (enable) {
        // 创建 autostart 目录（如果不存在）
        QDir().mkpath(autostartDir);

        // 创建 .desktop 文件
        QString desktopContent = QString(
            "[Desktop Entry]\n"
            "Type=Application\n"
            "Name=%1\n"
            "Exec=%2 --auto-start\n"
            "Icon=%1\n"
            "Comment=QtEasyTier - EasyTier GUI Client\n"
            "Terminal=false\n"
            "Categories=Network;\n"
        ).arg(appName, appPath);

        QFile desktopFile(desktopFilePath);
        if (!desktopFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }
        desktopFile.write(desktopContent.toUtf8());
        desktopFile.close();
        std::clog << "[AutoStart] Created autostart entry: " << desktopFilePath.toStdString() << std::endl;
    } else {
        // 删除 .desktop 文件
        if (QFile::exists(desktopFilePath)) {
            if (!QFile::remove(desktopFilePath)) {
                return false;
            }
        }
        std::clog << "[AutoStart] Removed autostart entry: " << desktopFilePath.toStdString() << std::endl;
    }

#else
    // 其他平台：暂不支持
    Q_UNUSED(enable)
    return false;
#endif

    return true;
}

void QtETSettings::saveLastPage(const QString &pageName)
{
    QJsonObject settings = loadSettingsFromFile();
    settings["last_page"] = pageName;
    saveSettingsToFile(settings);
}

QString QtETSettings::loadLastPage()
{
    const QJsonObject settings = loadSettingsFromFile();
    return settings.value("last_page").toString("hello");
}

void QtETSettings::checkForUpdate(QWidget *parent, bool silent)
{
    const QUrl url("https://gitee.com/api/v5/repos/myqfeng/qt-easy-tier/releases/latest");

    // 创建网络管理器
    auto *networkManager = new QNetworkAccessManager(parent);
    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    reply->setParent(networkManager);

    // 设置超时定时器（10 秒）
    auto *timeoutTimer = new QTimer(networkManager);
    timeoutTimer->setSingleShot(true);
    timeoutTimer->start(10000);

    // 超时处理：中止请求
    QObject::connect(timeoutTimer, &QTimer::timeout, reply, &QNetworkReply::abort);

    // 请求完成处理
    QObject::connect(reply, &QNetworkReply::finished, parent, [=]() {
        // 停止并删除超时定时器
        if (timeoutTimer) {
            timeoutTimer->stop();
            timeoutTimer->deleteLater();
        }

        // 检查请求结果
        if (reply->error() == QNetworkReply::NoError) {
            // 解析 JSON 响应
            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            QJsonObject jsonObj = jsonDoc.object();

            QString latestVersion = jsonObj["tag_name"].toString();

            // 对比版本号
            if (!latestVersion.isEmpty() && latestVersion > QString::fromUtf8(PROJECT_VERSION)) {
                QString msg = QString(tr("发现新版本：%1\n当前版本：%2\n是否前往下载？"))
                              .arg(latestVersion).arg(QString::fromUtf8(PROJECT_VERSION));

                QMessageBox::StandardButton ret = QMessageBox::question(
                    parent, tr("检查更新"), msg, QMessageBox::Yes | QMessageBox::No);

                if (ret == QMessageBox::Yes) {
                    QDesktopServices::openUrl(QUrl("https://gitee.com/myqfeng/qt-easy-tier/releases"));
                }
            } else if (!silent) {
                QMessageBox::information(parent, tr("检查更新"), tr("当前已是最新版本！"));
            }
        } else if (reply->error() != QNetworkReply::OperationCanceledError) {
            // 非超时错误才显示提示
            QMessageBox::warning(parent, tr("检查更新"),
                QString(tr("网络请求失败：%1")).arg(reply->errorString()));
        } else {
            // 超时提示
            QMessageBox::warning(parent, tr("检查更新"), tr("网络请求超时，请稍后重试！"));
        }

        // 删除网络管理器
        networkManager->deleteLater();
    });

}
