//
// Created by YMHuang on 2026/4/7.
//

#ifndef QTEASYTIER_QTETPUBLICSERVERDIALOG_H
#define QTEASYTIER_QTETPUBLICSERVERDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>
#include "qtetcheckbtn.h"
#include "qtetpushbtn.h"

/// @brief 服务器信息结构体（与 QtETServers 中的一致）
struct ServerInfoData
{
    QString name;       ///< 服务器名称
    QString address;    ///< 服务器地址

    ServerInfoData() = default;
    ServerInfoData(QString n, QString a) : name(std::move(n)), address(std::move(a)) {}

    /// @brief 从 JSON 对象解析
    static ServerInfoData fromJson(const QJsonObject &obj) {
        ServerInfoData info;
        info.name = obj["name"].toString();
        info.address = obj["address"].toString();
        return info;
    }
};

/// @brief 服务器收藏选择对话框
/// 从 servers.json 读取用户收藏的服务器列表，用户可以选择要添加的服务器
class QtETPublicServerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QtETPublicServerDialog(QWidget *parent = nullptr);
    ~QtETPublicServerDialog() override = default;

    /// @brief 设置已选择的服务器地址列表（用于初始化选中状态）
    /// @param selectedAddresses 已选择的服务器地址列表
    void setSelectedServers(const QStringList &selectedAddresses);

    /// @brief 获取用户选择的服务器地址列表
    [[nodiscard]] QStringList selectedServers() const;

private:
    /// @brief 初始化界面
    void initUI();
    /// @brief 加载服务器收藏列表
    void loadServers();
    /// @brief 更新服务器列表显示
    void updateServerList();
    /// @brief 获取 servers.json 文件路径
    [[nodiscard]] QString serverFilePath() const;

    // 主布局
    QVBoxLayout *m_mainLayout = nullptr;

    // 标题标签
    QLabel *m_titleLabel = nullptr;

    // 滚动区域
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_scrollContent = nullptr;
    QVBoxLayout *m_scrollLayout = nullptr;

    // 底部按钮
    QDialogButtonBox *m_buttonBox = nullptr;
    QtETPushBtn *m_selectAllBtn = nullptr;
    QtETPushBtn *m_deselectAllBtn = nullptr;

    // 服务器数据
    std::vector<ServerInfoData> m_servers;      ///< 服务器收藏列表
    QList<QtETCheckBtn*> m_serverCheckBoxes;    ///< 服务器开关按钮列表

    // 已选择的服务器（用于初始化选中状态）
    QStringList m_initiallySelectedAddresses;

private slots:
    /// @brief 全选按钮点击
    void onSelectAll();
    /// @brief 全不选按钮点击
    void onDeselectAll();
};

#endif //QTEASYTIER_QTETPUBLICSERVERDIALOG_H
