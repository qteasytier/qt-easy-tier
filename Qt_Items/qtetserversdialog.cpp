#include "qtetserversdialog.h"
#include "qtettheme.h"
#include "qtetsettings.h"
#include "qtetdrawutils.h"

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

    QFont defaultFont;
    defaultFont.setPointSize(10);
    setFont(defaultFont);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);

    m_titleLabel = new QLabel(this);
    QFont titleFont;
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setText(tr("请选择要添加的服务器："));
    m_mainLayout->addWidget(m_titleLabel);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    QtETSmoothScroll::install(m_scrollArea);

    // 使用 QPalette 替代 QSS 设置透明背景
    QPalette scrollPalette = m_scrollArea->palette();
    scrollPalette.setColor(QPalette::Window, Qt::transparent);
    m_scrollArea->setPalette(scrollPalette);
    m_scrollArea->setAttribute(Qt::WA_TranslucentBackground, true);

    if (m_scrollArea->viewport()) {
        QPalette vpPalette = m_scrollArea->viewport()->palette();
        vpPalette.setColor(QPalette::Window, Qt::transparent);
        vpPalette.setColor(QPalette::Base, Qt::transparent);
        m_scrollArea->viewport()->setPalette(vpPalette);
        m_scrollArea->viewport()->setAttribute(Qt::WA_TranslucentBackground, true);
    }

    m_scrollContent = new QWidget();
    m_scrollContent->setAttribute(Qt::WA_TranslucentBackground, true);
    m_scrollLayout = new QVBoxLayout(m_scrollContent);
    m_scrollLayout->setSpacing(8);
    m_scrollLayout->setContentsMargins(5, 5, 5, 5);
    m_scrollLayout->addStretch();

    m_scrollArea->setWidget(m_scrollContent);
    m_mainLayout->addWidget(m_scrollArea, 1);

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

    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_selectAllBtn, &QPushButton::clicked, this, &QtETServersDialog::onSelectAll);
    connect(m_deselectAllBtn, &QPushButton::clicked, this, &QtETServersDialog::onDeselectAll);

    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        updateColorScheme();
        update();
    });
}

void QtETServersDialog::updateColorScheme()
{
    auto *theme = QtETTheme::instance();
    m_bgColor = theme->widgetBackgroundColor();
    m_borderColor = theme->normalBorderColor();

    if (m_titleLabel) {
        QPalette pal = m_titleLabel->palette();
        pal.setColor(QPalette::WindowText, theme->textColor());
        m_titleLabel->setPalette(pal);
    }

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
            m_servers.append(ServerInfoData::fromJson(value.toObject()));
        }
    }
}

void QtETServersDialog::clearServerCheckboxes()
{
    for (auto *checkBox : m_serverCheckBoxes) {
        m_scrollLayout->removeWidget(checkBox);
        delete checkBox;
    }
    m_serverCheckBoxes.clear();
}

void QtETServersDialog::updateServerList()
{
    clearServerCheckboxes();

    for (const auto &server : m_servers) {
        auto *checkBox = new QtETCheckBtn(m_scrollContent);
        checkBox->setText(server.name);
        checkBox->setBriefTip(server.address);

        if (m_initiallySelectedAddresses.contains(server.address)) {
            checkBox->setChecked(true);
        }

        m_scrollLayout->insertWidget(m_scrollLayout->count() - 1, checkBox);
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

    for (int i = 0; i < m_servers.size() && i < m_serverCheckBoxes.size(); ++i) {
        if (m_initiallySelectedAddresses.contains(m_servers[i].address)) {
            m_serverCheckBoxes[i]->setChecked(true);
        }
    }
}

QStringList QtETServersDialog::selectedServers() const
{
    QStringList result;

    for (int i = 0; i < m_servers.size() && i < m_serverCheckBoxes.size(); ++i) {
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
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(rect().adjusted(1, 1, -1, -1), 8, 8);
    painter.fillPath(path, m_bgColor);
    painter.setPen(QPen(m_borderColor, 1));
    painter.drawPath(path);

    QDialog::paintEvent(event);
}
