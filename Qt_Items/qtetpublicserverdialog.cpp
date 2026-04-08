//
// Created by YMHuang on 2026/4/7.
//

#include "qtetpublicserverdialog.h"

#include <QFile>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QDir>
#include <QScrollBar>
#include <QApplication>
#include <QStyleHints>

#include "qtetsettings.h"

QtETPublicServerDialog::QtETPublicServerDialog(QWidget *parent)
    : QDialog(parent)
{
    initUI();
    loadServers();
    updateServerList();
}

void QtETPublicServerDialog::initUI()
{
    setWindowTitle(tr("服务器收藏列表"));
    setMinimumSize(500, 400);
    resize(600, 500);

    // 默认字体设为10号
    QFont defaultFont;
    defaultFont.setPointSize(10);
    setFont(defaultFont);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);

    // 标题
    m_titleLabel = new QLabel(this);
    QFont titleFont;
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setText(tr("请选择要添加的服务器："));
    m_mainLayout->addWidget(m_titleLabel);

    // 滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    // 滚动区域内容
    m_scrollContent = new QWidget();
    m_scrollLayout = new QVBoxLayout(m_scrollContent);
    m_scrollLayout->setSpacing(8);
    m_scrollLayout->setContentsMargins(5, 5, 5, 5);
    m_scrollLayout->addStretch();  // 底部弹簧

    m_scrollArea->setWidget(m_scrollContent);
    m_mainLayout->addWidget(m_scrollArea, 1);

    // 全选/全不选按钮
    auto *selectionBtnLayout = new QHBoxLayout();
    selectionBtnLayout->setSpacing(6);

    m_selectAllBtn = new QtETPushBtn(tr("全选"), this);
    m_selectAllBtn->setMinimumWidth(80);
    m_deselectAllBtn = new QtETPushBtn(tr("全不选"), this);
    m_deselectAllBtn->setMinimumWidth(80);

    selectionBtnLayout->addStretch();
    selectionBtnLayout->addWidget(m_selectAllBtn);
    selectionBtnLayout->addWidget(m_deselectAllBtn);

    m_mainLayout->addLayout(selectionBtnLayout);

    // 底部按钮
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_mainLayout->addWidget(m_buttonBox);

    // 连接信号
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_selectAllBtn, &QPushButton::clicked, this, &QtETPublicServerDialog::onSelectAll);
    connect(m_deselectAllBtn, &QPushButton::clicked, this, &QtETPublicServerDialog::onDeselectAll);
}

void QtETPublicServerDialog::loadServers()
{
    QString filePath = serverFilePath();
    QFile file(filePath);

    if (!file.exists()) {
        qWarning("Server config file not found: %s", qPrintable(filePath));
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Failed to open server config file: %s", qPrintable(filePath));
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
            m_servers.push_back(ServerInfoData::fromJson(value.toObject()));
        }
    }
}

void QtETPublicServerDialog::updateServerList()
{
    // 清除旧的控件
    for (auto *checkBox : m_serverCheckBoxes) {
        m_scrollLayout->removeWidget(checkBox);
        checkBox->deleteLater();
    }
    m_serverCheckBoxes.clear();

    // 创建新的控件
    for (const auto &server : m_servers) {
        auto *checkBox = new QtETCheckBtn(m_scrollContent);
        checkBox->setText(server.name);
        checkBox->setBriefTip(server.address);

        // 检查是否在初始选中列表中
        if (m_initiallySelectedAddresses.contains(server.address)) {
            checkBox->setChecked(true);
        }

        m_scrollLayout->insertWidget(m_scrollLayout->count() - 1, checkBox);  // 在 stretch 之前插入
        m_serverCheckBoxes.append(checkBox);
    }
}

QString QtETPublicServerDialog::serverFilePath() const
{
    return QtETSettings::getConfigPath() + "/servers.json";
}

void QtETPublicServerDialog::setSelectedServers(const QStringList &selectedAddresses)
{
    m_initiallySelectedAddresses = selectedAddresses;

    // 更新已有控件的选中状态
    for (int i = 0; i < static_cast<int>(m_servers.size()) && i < m_serverCheckBoxes.size(); ++i) {
        if (m_initiallySelectedAddresses.contains(m_servers[i].address)) {
            m_serverCheckBoxes[i]->setChecked(true);
        }
    }
}

QStringList QtETPublicServerDialog::selectedServers() const
{
    QStringList result;

    for (int i = 0; i < static_cast<int>(m_servers.size()) && i < m_serverCheckBoxes.size(); ++i) {
        if (m_serverCheckBoxes[i]->isChecked()) {
            result.append(m_servers[i].address);
        }
    }

    return result;
}

void QtETPublicServerDialog::onSelectAll()
{
    for (auto *checkBox : m_serverCheckBoxes) {
        checkBox->setChecked(true);
    }
}

void QtETPublicServerDialog::onDeselectAll()
{
    for (auto *checkBox : m_serverCheckBoxes) {
        checkBox->setChecked(false);
    }
}