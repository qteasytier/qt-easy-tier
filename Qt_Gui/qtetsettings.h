//
// Created by YMHuang on 2026/4/7.
//

#ifndef QTEASYTIER_QTETSETTINGS_H
#define QTEASYTIER_QTETSETTINGS_H

// ========== 版本相关宏定义 ==========
/// @brief EasyTier FFI 库版本号
#define ET_VERSION "2.6.0"
/// @brief 是否为 Beta 版本
#define IS_BETA_VERSION true
/// @brief QtEasyTier Slogan
#define SLOGAN "知不足而奋进，望远山而前行"


#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QProcess>
#include <QJsonObject>

class QtETCheckBtn;
class QtETPushBtn;

/**
 * @brief 设置页面类
 *
 * 嵌入到主窗口中的设置页面，提供以下功能：
 * - 隐藏到托盘
 * - 自动回连
 * - 自动检查更新
 * - 开机自启
 * - QtEasyTier 版本号显示
 * - 手动检查更新
 */
class QtETSettings : public QWidget
{
    Q_OBJECT

public:
    explicit QtETSettings(QWidget *parent = nullptr);
    ~QtETSettings() override = default;

    /// @brief 获取配置保存路径
    static QString getConfigPath();

    /// @brief 从配置文件加载设置
    void loadSettings();

    /// @brief 保存设置到配置文件
    void saveSettings();

    /// @brief 重置为上次保存的设置（丢弃更改）
    void discardChanges();

    /// @brief 检查是否有未保存的更改
    [[nodiscard]] bool hasUnsavedChanges() const;

    // ========== 静态方法：供外部直接读取设置 ==========

    /// @brief 获取自动回连状态
    static bool isAutoReconnect();

    /// @brief 获取开机自启状态
    static bool isAutoStart();

    /// @brief 是否隐藏到系统托盘
    static bool isHideOnTray();

    /// @brief 是否自动检查更新
    static bool isAutoCheckUpdate();

    /// @brief 设置开机自启
    /// @param enable 是否启用
    /// @return 是否成功
    static bool setAutoStart(bool enable);

    /// @brief 检测软件版本（网络请求）
    /// @param parent 父窗口
    /// @param silent 是否静默模式（不弹窗提示已是最新版本）
    static void checkForUpdate(QWidget *parent = nullptr, bool silent = false);

signals:
    /// @brief 设置已更改信号
    void settingsChanged();

    /// @brief 隐藏到托盘设置更改信号
    void hideOnTrayChanged(bool hideOnTray);

private slots:
    /// @brief 复选框状态变化
    void onCheckBoxStateChanged();

    /// @brief 保存按钮点击
    void onSaveButtonClicked();

    /// @brief 丢弃按钮点击
    void onDiscardButtonClicked();

    /// @brief 检查更新按钮点击
    void onCheckUpdateButtonClicked();

private:
    /// @brief 初始化界面
    void initUI();

    /// @brief 连接信号槽
    void setupConnections();

    /// @brief 更新按钮状态
    void updateButtonState();

    /// @brief 从配置文件读取设置（静态方法内部使用）
    static QJsonObject loadSettingsFromFile();

    /// @brief 保存设置到配置文件（静态方法内部使用）
    static bool saveSettingsToFile(const QJsonObject &settings);

    // ========== 设置项控件 ==========
    QtETCheckBtn *m_hideOnTrayCheckBox = nullptr;      ///< 关闭时隐藏到托盘
    QtETCheckBtn *m_autoReconnectCheckBox = nullptr;   ///< 自动回连
    QtETCheckBtn *m_autoCheckUpdateCheckBox = nullptr; ///< 自动检查更新
    QtETCheckBtn *m_autoStartCheckBox = nullptr;       ///< 开机自启

    // ========== 版本显示控件 ==========
    QLabel *m_versionLabel = nullptr;               ///< QtEasyTier 版本标签
    QtETPushBtn *m_checkUpdateBtn = nullptr;        ///< 检查更新按钮

    // ========== 操作按钮 ==========
    QtETPushBtn *m_discardBtn = nullptr;            ///< 丢弃按钮
    QtETPushBtn *m_saveBtn = nullptr;               ///< 保存按钮

    // ========== 设置值 ==========
    bool m_hideOnTray = true;           ///< 隐藏到托盘
    bool m_autoReconnect = false;       ///< 自动回连
    bool m_autoCheckUpdate = true;      ///< 自动检查更新
    bool m_autoStart = false;           ///< 开机自启

    // ========== 上次保存的值（用于检测更改） ==========
    bool m_lastSavedHideOnTray = true;
    bool m_lastSavedAutoReconnect = false;
    bool m_lastSavedAutoCheckUpdate = true;
    bool m_lastSavedAutoStart = false;
};

#endif //QTEASYTIER_QTETSETTINGS_H
