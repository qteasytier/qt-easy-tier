#include "qtetservers.h"
#include "qtetlabellist.h"

#include <QFont>
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>

#include "qtetsettings.h"

// ============================================================================
// ServerDialog 实现
// ============================================================================

ServerDialog::ServerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("服务器信息"));
    setMinimumWidth(350);

    auto *layout = new QFormLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(20, 20, 20, 20);

    // 服务器名称输入框
    m_nameEdit = new QtETLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("请输入服务器名称"));
    layout->addRow(tr("名称："), m_nameEdit);

    // 服务器地址输入框
    m_addressEdit = new QtETLineEdit(this);
    m_addressEdit->setPlaceholderText(tr("例如：tcp://example.com:27773"));
    layout->addRow(tr("地址："), m_addressEdit);

    // 按钮盒
    auto *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addRow(buttonBox);

    // 连接信号
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ServerDialog::setServerInfo(const ServerInfo &info)
{
    m_nameEdit->setText(info.name);
    m_addressEdit->setText(info.address);
}

ServerInfo ServerDialog::serverInfo() const
{
    ServerInfo info;
    info.name = m_nameEdit->text().trimmed();
    info.address = m_addressEdit->text().trimmed();
    return info;
}

// ============================================================================
// QtETServers 实现
// ============================================================================

QtETServers::QtETServers(QWidget *parent)
    : QWidget(parent)
{
    initUI();
    setupConnections();
    loadServers();
}

QtETServers::~QtETServers()
{
    saveServers();
}

void QtETServers::initUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);

    initTitleArea();
    initListArea();
    initButtonArea();
}

void QtETServers::initTitleArea()
{
    m_titleWidget = new QWidget(this);
    QHBoxLayout *titleLayout = new QHBoxLayout(m_titleWidget);
    titleLayout->setSpacing(15);
    titleLayout->setContentsMargins(16, 15, 16, 6);

    // 左侧图标
    m_iconLabel = new QLabel(m_titleWidget);
    m_iconLabel->setMaximumSize(48, 48);
    m_iconLabel->setPixmap(QPixmap(":/icons/icon.ico"));
    m_iconLabel->setScaledContents(true);

    // 标题文字
    m_titleLabel = new QLabel(m_titleWidget);
    QFont titleFont;
    titleFont.setPointSize(20);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setText(QStringLiteral("服务器收藏"));
    m_titleLabel->setAlignment(Qt::AlignCenter);

    // 右侧图标
    m_rightIconLabel = new QLabel(m_titleWidget);
    m_rightIconLabel->setMaximumSize(48, 48);
    m_rightIconLabel->setPixmap(QPixmap(":/icons/server.svg"));
    m_rightIconLabel->setScaledContents(true);

    titleLayout->addWidget(m_iconLabel, 0, Qt::AlignTop);
    titleLayout->addWidget(m_titleLabel, 0, Qt::AlignHCenter | Qt::AlignVCenter);
    titleLayout->addWidget(m_rightIconLabel);

    m_mainLayout->addWidget(m_titleWidget, 0, Qt::AlignHCenter | Qt::AlignTop);
}

void QtETServers::initListArea()
{
    // 服务器列表
    m_serverListWidget = new QtETLabelList(this);
    m_serverListWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 设置右键菜单策略
    m_serverListWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    // 创建右键菜单
    m_contextMenu = new QMenu(this);
    m_editAction = m_contextMenu->addAction(tr("编辑"));
    m_deleteAction = m_contextMenu->addAction(tr("删除"));

    // 设置图标
    m_editAction->setIcon(QIcon(":/icons/about.svg"));
    m_deleteAction->setIcon(QIcon(":/icons/eye-slash.svg"));

    m_mainLayout->addWidget(m_serverListWidget, 1);
}

void QtETServers::initButtonArea()
{
    m_bottomWidget = new QWidget(this);
    QHBoxLayout *bottomLayout = new QHBoxLayout(m_bottomWidget);
    bottomLayout->setContentsMargins(0, 0, 0, 0);

    // 添加弹簧让按钮居右
    bottomLayout->addStretch();

    // 添加服务器按钮
    m_addBtn = new QtETPushBtn(m_bottomWidget);
    m_addBtn->setMinimumWidth(120);
    QFont btnFont;
    btnFont.setPointSize(11);
    m_addBtn->setFont(btnFont);
    m_addBtn->setText(QStringLiteral("添加服务器"));

    bottomLayout->addWidget(m_addBtn);

    m_mainLayout->addWidget(m_bottomWidget);
}

void QtETServers::setupConnections()
{
    // 添加按钮
    connect(m_addBtn, &QPushButton::clicked, this, &QtETServers::onAddServer);

    // 列表双击
    connect(m_serverListWidget, &QListWidget::itemDoubleClicked,
            this, &QtETServers::onItemDoubleClicked);

    // 右键菜单
    connect(m_serverListWidget, &QWidget::customContextMenuRequested,
            this, &QtETServers::onShowContextMenu);

    // 右键菜单动作
    connect(m_editAction, &QAction::triggered, this, &QtETServers::onEditServer);
    connect(m_deleteAction, &QAction::triggered, this, &QtETServers::onDeleteServer);
}

void QtETServers::loadServers()
{
    QString filePath = configFilePath();
    QFile file(filePath);

    if (!file.exists()) {
        // 配置文件不存在，使用空列表
        m_servers.clear();
        updateList();
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Failed to open server config file for reading: %s", qPrintable(filePath));
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning("Failed to parse server config file: %s", qPrintable(parseError.errorString()));
        return;
    }

    if (!doc.isArray()) {
        qWarning("Server config file format error: expected array");
        return;
    }

    m_servers.clear();
    const QJsonArray array = doc.array();
    for (const auto &value : array) {
        if (value.isObject()) {
            m_servers.push_back(ServerInfo::fromJson(value.toObject()));
        }
    }

    updateList();
}

void QtETServers::saveServers()
{
    QString filePath = configFilePath();

    // 确保目录存在
    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QJsonArray array;
    for (const auto &server : m_servers) {
        array.append(server.toJson());
    }

    QJsonDocument doc(array);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("Failed to open server config file for writing: %s", qPrintable(filePath));
        return;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

void QtETServers::updateList()
{
    m_serverListWidget->clear();

    for (const auto &server : m_servers) {
        auto *item = new QtETLabelListItem();
        // 显示格式：名称 (地址)
        item->setText(QString("%1 (%2)").arg(server.name, server.address));
        item->setIcon(QIcon(":/icons/server.svg"));
        m_serverListWidget->addItem(item);
    }
}

QString QtETServers::configFilePath() const
{
    return QtETSettings::getConfigPath() + "/servers.json";
}

void QtETServers::onAddServer()
{
    ServerDialog dialog(this);
    dialog.setWindowTitle(tr("添加服务器"));

    if (dialog.exec() == QDialog::Accepted) {
        ServerInfo info = dialog.serverInfo();

        // 验证输入
        if (info.name.isEmpty()) {
            QMessageBox::warning(this, tr("警告"), tr("服务器名称不能为空"));
            return;
        }
        if (info.address.isEmpty()) {
            QMessageBox::warning(this, tr("警告"), tr("服务器地址不能为空"));
            return;
        }

        // 检查地址是否重复
        for (const auto &server : m_servers) {
            if (server.address == info.address) {
                QMessageBox::warning(this, tr("警告"), tr("该服务器地址已存在"));
                return;
            }
        }

        m_servers.push_back(info);
        updateList();
        saveServers();
    }
}

void QtETServers::onEditServer()
{
    int currentRow = m_serverListWidget->currentRow();
    if (currentRow < 0 || currentRow >= static_cast<int>(m_servers.size())) {
        return;
    }

    ServerDialog dialog(this);
    dialog.setWindowTitle(tr("编辑服务器"));
    dialog.setServerInfo(m_servers[currentRow]);

    if (dialog.exec() == QDialog::Accepted) {
        ServerInfo info = dialog.serverInfo();

        // 验证输入
        if (info.name.isEmpty()) {
            QMessageBox::warning(this, tr("警告"), tr("服务器名称不能为空"));
            return;
        }
        if (info.address.isEmpty()) {
            QMessageBox::warning(this, tr("警告"), tr("服务器地址不能为空"));
            return;
        }

        // 检查地址是否重复（排除自己）
        for (size_t i = 0; i < m_servers.size(); ++i) {
            if (static_cast<int>(i) != currentRow && m_servers[i].address == info.address) {
                QMessageBox::warning(this, tr("警告"), tr("该服务器地址已存在"));
                return;
            }
        }

        m_servers[currentRow] = info;
        updateList();
        saveServers();
    }
}

void QtETServers::onDeleteServer()
{
    int currentRow = m_serverListWidget->currentRow();
    if (currentRow < 0 || currentRow >= static_cast<int>(m_servers.size())) {
        return;
    }

    const ServerInfo &server = m_servers[currentRow];

    // 弹出确认对话框
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("确认删除"),
        tr("确定要删除服务器 \"%1\" 吗？").arg(server.name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_servers.erase(m_servers.begin() + currentRow);
        updateList();
        saveServers();
    }
}

void QtETServers::onItemDoubleClicked(QListWidgetItem *item)
{
    Q_UNUSED(item)
    onEditServer();
}

void QtETServers::onShowContextMenu(const QPoint &pos)
{
    // 检查是否有选中项
    QListWidgetItem *item = m_serverListWidget->itemAt(pos);
    if (item) {
        m_contextMenu->exec(m_serverListWidget->mapToGlobal(pos));
    }
}
