#ifndef QTETSERVERS_H
#define QTETSERVERS_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QMenu>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>

#include "qtetlabellist.h"
#include "qtetpushbtn.h"
#include "qtetlineedit.h"

/// @brief 服务器信息结构体
struct ServerInfo
{
    QString name;       ///< 服务器名称
    QString address;    ///< 服务器地址

    ServerInfo() = default;
    ServerInfo(QString n, QString addr) : name(std::move(n)), address(std::move(addr)) {}

    /// @brief 转换为 JSON 对象
    [[nodiscard]] QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"] = name;
        obj["address"] = address;
        return obj;
    }

    /// @brief 从 JSON 对象解析
    static ServerInfo fromJson(const QJsonObject &obj) {
        ServerInfo info;
        info.name = obj["name"].toString();
        info.address = obj["address"].toString();
        return info;
    }
};

/// @brief 添加/编辑服务器对话框
class ServerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ServerDialog(QWidget *parent = nullptr);
    ~ServerDialog() override = default;

    /// @brief 设置服务器信息（编辑模式）
    void setServerInfo(const ServerInfo &info);
    /// @brief 获取服务器信息
    [[nodiscard]] ServerInfo serverInfo() const;

private:
    QtETLineEdit *m_nameEdit = nullptr;        ///< 服务器名称输入框
    QtETLineEdit *m_addressEdit = nullptr;     ///< 服务器地址输入框
};

/// @brief 服务器收藏页面
class QtETServers : public QWidget
{
    Q_OBJECT

public:
    explicit QtETServers(QWidget *parent = nullptr);
    ~QtETServers() override;

    /// @brief 加载服务器列表（从配置文件）
    void loadServers();
    /// @brief 保存服务器列表（到配置文件）
    void saveServers();

private:
    /// @brief 初始化界面
    void initUI();
    /// @brief 初始化标题区域
    void initTitleArea();
    /// @brief 初始化列表区域
    void initListArea();
    /// @brief 初始化底部按钮区域
    void initButtonArea();
    /// @brief 设置信号连接
    void setupConnections();
    /// @brief 更新列表显示
    void updateList();
    /// @brief 获取配置文件路径
    [[nodiscard]] QString configFilePath() const;

    // 主布局
    QVBoxLayout *m_mainLayout = nullptr;

    // 标题区域
    QWidget *m_titleWidget = nullptr;
    QLabel *m_iconLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_rightIconLabel = nullptr;

    // 列表区域
    QtETLabelList *m_serverListWidget = nullptr;

    // 底部按钮区域
    QWidget *m_bottomWidget = nullptr;
    QtETPushBtn *m_addBtn = nullptr;        ///< 添加服务器按钮

    // 右键菜单
    QMenu *m_contextMenu = nullptr;
    QAction *m_editAction = nullptr;        ///< 编辑动作
    QAction *m_deleteAction = nullptr;      ///< 删除动作

    // 服务器数据
    std::vector<ServerInfo> m_servers;      ///< 服务器列表

private slots:
    /// @brief 添加服务器
    void onAddServer();
    /// @brief 编辑服务器
    void onEditServer();
    /// @brief 删除服务器
    void onDeleteServer();
    /// @brief 列表项双击
    void onItemDoubleClicked(QListWidgetItem *item);
    /// @brief 显示右键菜单
    void onShowContextMenu(const QPoint &pos);
};

#endif // QTETSERVERS_H
