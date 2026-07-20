/**
 * @file SettingsViewModel.h
 * @brief 全局应用设置 ViewModel
 *
 * 管理开机自启、本地设置和日志配置。
 * autoReconnect 不再本地持久化，改为通过 DaemonApi 读写后端状态。
 * 本地设置持久化到系统标准配置路径下的 settings3.json，
 * 不走 SQLite（app_settings 表已删除）。
 *
 * 作为 QML 单例注入，所有所有权归属于 QQmlApplicationEngine。
 */
#pragma once
#include "core/application/settings/AutoStartService.h"
#include "core/application/settings/SettingsStore.h"

#include <QFutureWatcher>
#include <QJsonObject>
#include <QObject>
#include <QString>

class DaemonApi;
class UpdateCheckService;

/**
 * @class SettingsViewModel
 * @brief 应用设置数据模型与持久化管理器
 *
 * 职责：
 *   - 管理 autoStart（开机自启）本地开关
 *   - 管理 autoReconnect（自动回连）后端开关（通过 DaemonApi）
 *   - 提供 load() / save() 对 settings3.json 的读写
 *   - setAutoStart() 委托 AutoStartService 管理系统级自启动项
 *   - Q_INVOKABLE 方法直接暴露给 QML 绑定和调用
 */
class SettingsViewModel : public QObject {
    Q_OBJECT

    /// 开机自启开关（true=开启，false=关闭），只读属性，通过 setAutoStart() 方法修改
    Q_PROPERTY(bool autoStart READ autoStart NOTIFY autoStartChanged FINAL)

    /// 自动回连开关（true=开启，false=关闭），只读属性，通过 setAutoReconnectEnabled() 方法修改
    Q_PROPERTY(bool autoReconnect READ autoReconnect NOTIFY autoReconnectChanged FINAL)

    /// 自动回连请求是否进行中，QML 用于禁用开关
    Q_PROPERTY(bool autoReconnectBusy READ autoReconnectBusy NOTIFY autoReconnectBusyChanged FINAL)

    /// 启动后自动检查更新开关
    Q_PROPERTY(bool autoCheckUpdates READ autoCheckUpdates WRITE setAutoCheckUpdates NOTIFY autoCheckUpdatesChanged FINAL)

    /// 更新检查请求是否进行中，QML 用于禁用按钮
    Q_PROPERTY(bool updateCheckBusy READ updateCheckBusy NOTIFY updateCheckBusyChanged FINAL)

    /// 退出前端程序前是否显示提示弹窗（true=显示，false=不再提示）
    Q_PROPERTY(bool showExitPrompt READ showExitPrompt WRITE setShowExitPrompt NOTIFY showExitPromptChanged FINAL)

    /// 隐藏服务节点开关（true=运行状态页不显示公共服务器节点）
    Q_PROPERTY(bool hideServerNodes READ hideServerNodes WRITE setHideServerNodes NOTIFY hideServerNodesChanged FINAL)

    Q_PROPERTY(int logLevel READ logLevel WRITE setLogLevel NOTIFY logLevelChanged FINAL)
    Q_PROPERTY(int maxLogEntries READ maxLogEntries WRITE setMaxLogEntries NOTIFY maxLogEntriesChanged FINAL)
    Q_PROPERTY(QString frontendVersion READ frontendVersion CONSTANT FINAL)
    Q_PROPERTY(QString easyTierVersion READ easyTierVersion CONSTANT FINAL)

public:
    explicit SettingsViewModel(DaemonApi *daemonApi, QObject *parent);
    explicit SettingsViewModel(DaemonApi *daemonApi = nullptr,
                               UpdateCheckService *updateCheckService = nullptr,
                               QObject *parent = nullptr);

    bool autoStart() const;

    bool autoReconnect() const;
    bool autoReconnectBusy() const;

    bool autoCheckUpdates() const;
    void setAutoCheckUpdates(bool value);
    bool updateCheckBusy() const;

    bool hideServerNodes() const;
    void setHideServerNodes(bool value);

    bool showExitPrompt() const;
    void setShowExitPrompt(bool value);

    /**
     * @brief 从 settings3.json 加载本地设置到内存
     *
     * 文件不存在时不报错，保持默认值。
     * 加载后会发射 autoStartChanged、hideServerNodesChanged 等信号。
     */
    Q_INVOKABLE void load();

    /**
     * @brief 将当前内存中的本地设置持久化到 settings3.json
     *
     * 自动创建父目录（若不存在）。
     * 以紧凑 JSON 格式写入（无缩进换行）。
     */
    Q_INVOKABLE void save();

    /**
     * @brief 从后端查询当前自动回连状态
     *
     * 发出 get_auto_reconnect 请求，成功后更新 m_autoReconnect 并发射信号。
     * 请求期间 autoReconnectBusy 为 true。
     */
    Q_INVOKABLE void refreshAutoReconnect();

    /**
     * @brief 向后端设置自动回连开关
     *
     * 发出 set_auto_reconnect 请求，成功后更新 m_autoReconnect 并发射信号。
     * 请求期间 autoReconnectBusy 为 true。
     *
     * @param enabled true 为启用，false 为禁用
     */
    Q_INVOKABLE void setAutoReconnectEnabled(bool enabled);

    /**
     * @brief 设置或取消开机自启动
     *
     * 流程：
     *   1. 状态未变则直接返回 true
     *   2. 调用 AutoStartService 系统级接口
     *   3. 成功则更新内存状态 + 发射信号 + 持久化
     *   4. 失败则保持原状态，输出警告日志
     *
     * @param enabled true 为开启，false 为取消
     * @return 操作是否成功
     */
    Q_INVOKABLE bool setAutoStart(bool enabled);

    Q_INVOKABLE void checkForUpdates();
    void checkForUpdatesOnStartup();

    Q_INVOKABLE bool isAutoStartEnabled() const;

    int logLevel() const;
    void setLogLevel(int value);
    int maxLogEntries() const;
    void setMaxLogEntries(int value);
    QString frontendVersion() const;
    QString easyTierVersion() const;

signals:
    void autoStartChanged();
    void autoReconnectChanged();
    void autoReconnectBusyChanged();
    void autoReconnectOperationFailed(const QString &message);
    void autoCheckUpdatesChanged();
    void updateCheckBusyChanged();
    void hideServerNodesChanged();
    void showExitPromptChanged();
    void logLevelChanged();
    void maxLogEntriesChanged();

private:
    SettingsStore::Settings settings() const;
    void applySettings(const SettingsStore::Settings &settings);
    void setBusy(bool busy);
    void setUpdateCheckBusy(bool busy);

    DaemonApi *m_daemonApi = nullptr;
    UpdateCheckService *m_updateCheckService = nullptr;
    AutoStartService m_autoStartService;
    SettingsStore m_store;
    bool m_autoStart = false;
    bool m_autoReconnect = false;
    bool m_autoReconnectBusy = false;
    bool m_autoCheckUpdates = true;
    bool m_updateCheckBusy = false;
    bool m_hideServerNodes = false;
    bool m_showExitPrompt = true;
    int m_logLevel = 1;
    int m_maxLogEntries = 100;
};
