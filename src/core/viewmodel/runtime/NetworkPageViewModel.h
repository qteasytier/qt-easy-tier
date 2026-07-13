/**
 * @file NetworkPageViewModel.h
 * @brief 网络页面 ViewModel，协调配置列表、编辑器、VPN 管理和页面状态
 *
 * 作为网络页面的顶层协调器，管理以下逻辑：
 * - 配置的选择、创建、删除、重命名、导入
 * - 配置的启动和停止（通过 VpnManager）
 * - 编辑器显示 / 运行时状态显示的页面切换
 * - 当前选中实例的运行状态查询
 */
#pragma once

#include <QObject>
#include <QString>

class BackendStatusViewModel;
class ConfigEditorViewModel;
class ConfigListModel;

/**
 * @brief 网络页面协调 ViewModel，聚合配置列表/编辑器/VPN 管理器，管理页面状态切换
 */
class NetworkPageViewModel : public QObject
{
    Q_OBJECT
    /// 当前选中的配置实例名称
    Q_PROPERTY(QString currentInstanceName READ currentInstanceName NOTIFY currentInstanceNameChanged FINAL)
    /// 当前选中的配置是否正在运行
    Q_PROPERTY(bool currentInstanceRunning READ currentInstanceRunning NOTIFY currentInstanceRunningChanged FINAL)
    /// 是否展示编辑器界面（无选中实例 或 选中实例未运行）
    Q_PROPERTY(bool showEditor READ showEditor NOTIFY pageStateChanged FINAL)
    /// 是否展示运行时状态界面（有选中实例且正在运行）
    Q_PROPERTY(bool showRuntimeStatus READ showRuntimeStatus NOTIFY pageStateChanged FINAL)

public:
    /**
     * @brief 构造网络页面 ViewModel
     * @param configListModel 配置列表模型
     * @param configEditorViewModel 配置编辑器 ViewModel
     * @param vpnManager VPN 管理器（通过 invokeMethod 调用其 startConfig/stopConfig）
     * @param backendStatusViewModel 后端状态 ViewModel
     * @param parent 父 QObject
     */
    explicit NetworkPageViewModel(ConfigListModel *configListModel,
                                  ConfigEditorViewModel *configEditorViewModel,
                                   QObject *vpnManager,
                                  BackendStatusViewModel *backendStatusViewModel,
                                  QObject *parent = nullptr);

    QString currentInstanceName() const;
    bool currentInstanceRunning() const;
    bool showEditor() const;
    bool showRuntimeStatus() const;

public slots:
    /// 刷新当前实例的运行状态（由 VPN 状态变更信号触发）
    void refreshRunning();

public:
    /// 选中一个配置实例：加载编辑器并刷新运行状态
    Q_INVOKABLE void selectConfig(const QString &instanceName);
    /// 创建新配置并自动选中
    Q_INVOKABLE QString createConfig();
    /// 删除指定配置（若为当前选中则清空编辑器）
    Q_INVOKABLE void deleteConfig(const QString &instanceName);
    /// 重命名指定配置的显示名称
    Q_INVOKABLE void renameConfig(const QString &instanceName, const QString &newDisplayName);
    /// 启动指定配置（自动选中 + 调用 VpnManager::startConfig）
    Q_INVOKABLE void startConfig(const QString &instanceName);
    /// 停止指定配置（自动选中 + 调用 VpnManager::stopConfig）
    Q_INVOKABLE void stopConfig(const QString &instanceName);
    /// 从文件导入配置
    Q_INVOKABLE void importConfigFile(const QString &filePath);
    /// 从 qtet:// URL 导入配置
    Q_INVOKABLE void importConfigUrl(const QString &url);

signals:
    /// 当前选中实例名称变化时发射
    void currentInstanceNameChanged();
    /// 当前选中实例运行状态变化时发射
    void currentInstanceRunningChanged();
    /// 编辑器/运行时页面切换时发射
    void pageStateChanged();

private:
    void setCurrentInstanceName(const QString &instanceName);
    void setCurrentInstanceRunning(bool running);

    ConfigListModel *m_configListModel = nullptr;              ///< 配置列表模型（非所有权）
    ConfigEditorViewModel *m_configEditorViewModel = nullptr;  ///< 配置编辑器（非所有权）
    QObject *m_vpnManager = nullptr;                           ///< VPN 管理器（非所有权，通过 invokeMethod 调用）
    BackendStatusViewModel *m_backendStatusViewModel = nullptr;///< 后端状态（非所有权）
    QString m_currentInstanceName;                             ///< 当前选中实例名缓存
    bool m_currentInstanceRunning = false;                     ///< 当前实例运行状态缓存
};
