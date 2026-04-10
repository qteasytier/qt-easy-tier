//
// Created by YMHuang on 2026/4/7.
//

#include "qtetserversdialog.h"

#include <QFile>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QDir>
#include <QScrollBar>
#include <QApplication>
#include <QStyleHints>
#include <QPainter>
#include <QPainterPath>
#include <QHBoxLayout>

#include "qtetsettings.h"

QtETServersDialog::QtETServersDialog(QWidget *parent)
    : QDialog(parent)
{
    initUI();
    loadServers();
    updateServerList();
    updateColorScheme();
}

void QtETServersDialog::initUI()
{
    setWindowTitle(tr("服务器收藏列表"));
    setMinimumSize(500, 400);
    resize(600, 500);

    // 对话框保留系统边框，但自定义内部背景绘制

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
    // 设置滚动区域背景透明
    m_scrollArea->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; }"));
    m_scrollArea->viewport()->setStyleSheet(QStringLiteral("background: transparent;"));

    // 滚动区域内容
    m_scrollContent = new QWidget();
    m_scrollContent->setAttribute(Qt::WA_TranslucentBackground, true);
    m_scrollLayout = new QVBoxLayout(m_scrollContent);
    m_scrollLayout->setSpacing(8);
    m_scrollLayout->setContentsMargins(5, 5, 5, 5);
    m_scrollLayout->addStretch();  // 底部弹簧

    m_scrollArea->setWidget(m_scrollContent);
    m_mainLayout->addWidget(m_scrollArea, 1);

    // 底部按钮（自定义按钮）
    auto *bottomBtnLayout = new QHBoxLayout();
    bottomBtnLayout->setSpacing(10);
    bottomBtnLayout->addStretch();

    m_okBtn = new QtETPushBtn(tr("确定"), this);
    m_okBtn->setMinimumWidth(80);
    m_cancelBtn = new QtETPushBtn(tr("取消"), this);
    m_cancelBtn->setMinimumWidth(80);

    m_selectAllBtn = new QtETPushBtn(tr("全选"), this);
    m_selectAllBtn->setMinimumWidth(80);
    m_deselectAllBtn = new QtETPushBtn(tr("全不选"), this);
    m_deselectAllBtn->setMinimumWidth(80);

    bottomBtnLayout->addWidget(m_selectAllBtn);
    bottomBtnLayout->addWidget(m_deselectAllBtn);
    bottomBtnLayout->addWidget(m_okBtn);
    bottomBtnLayout->addWidget(m_cancelBtn);

    m_mainLayout->addLayout(bottomBtnLayout);

    // 连接信号
    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_selectAllBtn, &QPushButton::clicked, this, &QtETServersDialog::onSelectAll);
    connect(m_deselectAllBtn, &QPushButton::clicked, this, &QtETServersDialog::onDeselectAll);

    // 监听主题变化
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        updateColorScheme();
        update();
    });
}

void QtETServersDialog::updateColorScheme()
{
    const bool isDark = (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark);

    if (isDark) {
        m_bgColor = QColor(45, 45, 45);
        m_borderColor = QColor(70, 70, 70);

        // 更新标题颜色
        if (m_titleLabel) {
            QPalette pal = m_titleLabel->palette();
            pal.setColor(QPalette::WindowText, QColor(220, 220, 220));
            m_titleLabel->setPalette(pal);
        }
    } else {
        m_bgColor = QColor(240, 240, 240);
        m_borderColor = QColor(180, 180, 180);

        // 更新标题颜色
        if (m_titleLabel) {
            QPalette pal = m_titleLabel->palette();
            pal.setColor(QPalette::WindowText, QColor(30, 30, 30));
            m_titleLabel->setPalette(pal);
        }
    }

    // 滚动区域背景始终透明
    // 无需在这里设置，已在 initUI 中通过 setStyleSheet 设置

    update();
}

void QtETServersDialog::loadServers()
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

void QtETServersDialog::updateServerList()
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

QString QtETServersDialog::serverFilePath() const
{
    return QtETSettings::getConfigPath() + "/servers.json";
}

void QtETServersDialog::setSelectedServers(const QStringList &selectedAddresses)
{
    m_initiallySelectedAddresses = selectedAddresses;

    // 更新已有控件的选中状态
    for (int i = 0; i < static_cast<int>(m_servers.size()) && i < m_serverCheckBoxes.size(); ++i) {
        if (m_initiallySelectedAddresses.contains(m_servers[i].address)) {
            m_serverCheckBoxes[i]->setChecked(true);
        }
    }
}

QStringList QtETServersDialog::selectedServers() const
{
    QStringList result;

    for (int i = 0; i < static_cast<int>(m_servers.size()) && i < m_serverCheckBoxes.size(); ++i) {
        if (m_serverCheckBoxes[i]->isChecked()) {
            result.append(m_servers[i].address);
        }
    }

    return result;
}

void QtETServersDialog::onSelectAll()
{
    for (auto *checkBox : m_serverCheckBoxes) {
        checkBox->setChecked(true);
    }
}

void QtETServersDialog::onDeselectAll()
{
    for (auto *checkBox : m_serverCheckBoxes) {
        checkBox->setChecked(false);
    }
}

void QtETServersDialog::paintEvent(QPaintEvent *event)
{
    // 调用父类 paintEvent 以确保对话框正常绘制
    QDialog::paintEvent(event);
}
