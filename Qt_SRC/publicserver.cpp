#include "../Qt_HEAD/publicserver.h"
#include "ui_publicserver.h"
#include <QFile>
#include <QJsonDocument>
#include <QMessageBox>
#include <QApplication>
#include <QDesktopServices>

PublicServer::PublicServer(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PublicServer)
{
    ui->setupUi(this);

    // 设置表头
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers); // 设置表格不可编辑
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows); // 设置整行选择
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true); // 最后一列自动扩展
    // 设置列宽
    ui->tableWidget->setColumnWidth(0, 300);

    // 连接信号槽
    connect(ui->tableWidget, &QTableWidget::cellClicked,
            this, &PublicServer::onTableCellClicked);
    connect(ui->disclaimerBtn, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(QUrl("https://gitee.com/viagrahuang/qt-easy-tier/blob/master/docs/disclaimer.md"));
    });
    // 加载服务器数据
    loadServerData();
    populateTable();
}

PublicServer::~PublicServer()
{
    delete ui;
}

void PublicServer::loadServerData()
{
    // 尝试从publicserver.json加载数据，如果不存在则使用默认数据
    QString jsonFilePath = QApplication::applicationDirPath() + "/publicserver.json";
    QFile jsonFile(jsonFilePath);

    if (jsonFile.exists()) {
        if (jsonFile.open(QIODevice::ReadOnly)) {
            QByteArray jsonData = jsonFile.readAll();
            jsonFile.close();

            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

            if (parseError.error == QJsonParseError::NoError) {
                if (doc.isArray()) {
                    serverData = doc.array();
                }
            } else {
                QMessageBox::warning(this, "警告", "解析publicserver.json失败: " + parseError.errorString());
            }
        }
    }


}

void PublicServer::populateTable()
{
    // 清空现有数据
    ui->tableWidget->setRowCount(0);

    // 设置表格行数
    ui->tableWidget->setRowCount(serverData.size());

    // 填充数据
    for (int i = 0; i < serverData.size(); ++i) {
        QJsonValue serverValue = serverData[i];
        QJsonObject serverObj = serverValue.toObject();

        //QTableWidgetItem *propertyItem = new QTableWidgetItem(serverObj["property"].toString());
        QTableWidgetItem *urlItem = new QTableWidgetItem(serverObj["url"].toString());
        QTableWidgetItem *contributorItem = new QTableWidgetItem(serverObj["contributor"].toString());

        //propertyItem->setTextAlignment(Qt::AlignCenter);
        contributorItem->setTextAlignment(Qt::AlignCenter);

        // 检查此URL是否已被选中
        QString url = serverObj["url"].toString();
        if (isUrlSelected(url)) {
            urlItem->setForeground(QBrush(QColor(0, 128, 0))); // 绿色
        }

        //ui->tableWidget->setItem(i, 0, propertyItem);
        ui->tableWidget->setItem(i, 0, urlItem);
        ui->tableWidget->setItem(i, 1, contributorItem);
    }
}

void PublicServer::onTableCellClicked(int row, int column)
{
    // 只处理网址列的点击
    if (column == 0) {
        QTableWidgetItem *item = ui->tableWidget->item(row, column);
        QString url = item->text();

        toggleUrlSelection(url);

        // 更新表格显示
        if (isUrlSelected(url)) {
            item->setForeground(QBrush(QColor(102, 204, 255)));
        } else {
            item->setForeground(QBrush()); // 默认颜色
        }
    }
    //清空表格selected状态，以便显示字体颜色
    ui->tableWidget->clearSelection();
}

void PublicServer::toggleUrlSelection(const QString &url)
{
    if (isUrlSelected(url)) {
        // 如果已选择，则移除
        selectedUrls.removeAll(url);
    } else {
        // 如果未选择，则添加
        selectedUrls.append(url);
    }
}

bool PublicServer::isUrlSelected(const QString &url) const
{
    return selectedUrls.contains(url);
}

QStringList PublicServer::getSelectedServers()
{
    return selectedUrls;
}
