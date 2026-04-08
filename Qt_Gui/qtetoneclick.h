#ifndef QTETONECLICK_H
#define QTETONECLICK_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QThread>
#include <QTimer>
#include <QProgressDialog>
#include <QJsonObject>
#include <QJsonArray>
#include <sstream>
#include <vector>
#include <string>
#include "qtetcheckbtn.h"
#include "qtetpushbtn.h"
#include "ETRunWorker.h"

class QtETOneClick : public QWidget
{
    Q_OBJECT

public:
    explicit QtETOneClick(QWidget *parent = nullptr);
    ~QtETOneClick() override;

    /// @brief 检查是否有网络正在运行
    [[nodiscard]] bool isRunning() const;

private:
    /// @brief 初始化界面
    void initUI();
    /// @brief 初始化标题区域
    void initTitleArea();
    /// @brief 初始化表单区域
    void initFormArea();
    /// @brief 初始化服务器地址区域
    void initServerArea();
    /// @brief 初始化底部按钮区域
    void initButtonArea();
    /// @brief 设置信号连接
    void setupConnections();
    /// @brief 初始化 Worker 线程
    void initWorkerThread();
    /// @brief 清理 Worker 线程
    void cleanupWorkerThread();
    /// @brief 更新界面状态
    void updateInterfaceState();
    /// @brief 显示进度对话框
    void showProgressDialog(const QString& title);
    /// @brief 关闭进度对话框
    void closeProgressDialog();
    /// @brief 停止当前运行的网络
    void stopCurrentNetwork();
    /// @brief 解析节点信息
    /// @param peers 直连节点数组（用于连接检测）
    /// @param routes 路由节点数组（用于计算联机人数和查找房主 IP）
    /// @param root JSON 根对象（用于获取本机信息）
    void parsePeerInfo(const QJsonArray& peers, const QJsonArray& routes, const QJsonObject& root);

    // ==================== 联机码相关 ====================
    /// @brief Base32 编码
    static QString base32Encode(const QByteArray& data);
    /// @brief Base32 解码
    static QByteArray base32Decode(const QString& encoded);
    /// @brief 生成房间凭证
    static QPair<QString, QString> generateRoomCredentials();
    /// @brief 编码联机码
    static QString encodeConnectionCode(const QString& networkId, const QString& password);
    /// @brief 解码联机码
    static QPair<QString, QString> decodeConnectionCode(const QString& code);

    // ==================== TOML 配置生成 ====================
    /// @brief 生成房主 TOML 配置
    std::string generateHostTomlConfig();
    /// @brief 生成房客 TOML 配置
    std::string generateGuestTomlConfig();

    // 主布局
    QVBoxLayout *m_mainLayout = nullptr;
    QVBoxLayout *m_contentLayout = nullptr;  ///< 滚动区域内容布局

    // 标题区域
    QWidget *m_titleWidget = nullptr;
    QLabel *m_iconLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_rightIconLabel = nullptr;

    // 表单区域
    QWidget *m_formWidget = nullptr;
    QLineEdit *m_roomIdEdit = nullptr;      ///< 联机码输入/显示框
    QLineEdit *m_hostIpEdit = nullptr;      ///< 房主虚拟 IP 显示框
    QLabel *m_roomIdLabel = nullptr;        ///< 联机码标签
    QLabel *m_hostIpLabel = nullptr;        ///< 房主虚拟 IP 标签

    // 一键联机按钮
    QtETPushBtn *m_oneClickBtn = nullptr;

    // 服务器地址区域
    QWidget *m_serverWidget = nullptr;
    QLineEdit *m_serverEdit = nullptr;      ///< 服务器地址输入框
    QtETPushBtn *m_addServerBtn = nullptr;  ///< 添加服务器按钮
    QListWidget *m_serverListWidget = nullptr; ///< 服务器列表
    QtETPushBtn *m_removeServerBtn = nullptr;  ///< 删除服务器按钮
    QtETPushBtn *m_publicServerBtn = nullptr;  ///< 公共服务器列表按钮

    // 底部按钮区域
    QWidget *m_bottomWidget = nullptr;
    QtETCheckBtn *m_hostModeCheckBox = nullptr;  ///< 我做房主开关
    QtETCheckBtn *m_latencyFirstCheckBox = nullptr; ///< 低延迟优先开关

    // ==================== 网络运行相关 ====================
    QThread *m_workerThread = nullptr;      ///< Worker 工作线程
    ETRunWorker *m_worker = nullptr;        ///< Worker 工作对象
    QProgressDialog *m_progressDialog = nullptr; ///< 进度对话框
    QTimer *m_monitorTimer = nullptr;       ///< 节点监测定时器
    QTimer *m_connectionTimer = nullptr;    ///< 连接超时定时器

    // 当前运行状态
    enum class UserRole {
        None,       ///< 未运行
        Host,       ///< 房主模式
        Guest,      ///< 房客模式
        Stopping    ///< 停止中
    };
    UserRole m_currentRole = UserRole::None;

    // 房间信息
    QString m_currentNetworkId;             ///< 当前网络号
    QString m_currentPassword;              ///< 当前密码
    QString m_lastHostIp;                   ///< 上次房主 IP（房客模式用）

    // 常量
    static constexpr int CONNECTION_TIMEOUT_MS = 20000; ///< 连接超时时间（毫秒）
    static constexpr int MONITOR_INTERVAL_MS = 2000;    ///< 节点监测间隔（毫秒）
    static constexpr int ONECLICK_MAX_WAIT_TIME = 30000;///< 最大等待时间

signals:
    /// @brief 网络状态变化信号
    void networkStateChanged(bool isRunning);

private slots:
    /// @brief 添加服务器按钮点击
    void onAddServer();
    /// @brief 删除服务器按钮点击
    void onRemoveServer();
    /// @brief 服务器列表选中项变化
    void onServerSelectionChanged();
    /// @brief 服务器地址输入框回车
    void onServerEditReturnPressed();
    /// @brief 一键联机按钮点击
    void onOneClickBtnClicked();
    /// @brief 房主模式开关变化
    void onHostModeChanged(bool checked);
    
    // Worker 信号槽
    /// @brief 网络启动完成
    void onNetworkStarted(const std::string &instName, bool success, const std::string &errorMsg);
    /// @brief 网络停止完成
    void onNetworkStopped(const std::string &instName, bool success, const std::string &errorMsg);
    /// @brief 网络信息收集完成
    void onInfosCollected(const std::vector<EasyTierFFI::KVPair> &infos);
    /// @brief 连接超时
    void onConnectionTimeout();
    /// @brief 节点监测定时器超时
    void onMonitorTimerTimeout();
};

#endif // QTETONECLICK_H