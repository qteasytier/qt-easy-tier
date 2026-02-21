#ifndef SETTING_H
#define SETTING_H

#include <QDialog>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QThread>
#include <QApplication>

namespace Ui {
    class setting;
}

// 版本检测工作类
class VersionDetectionWorker : public QObject
{
    Q_OBJECT

public:
    explicit VersionDetectionWorker(QObject *parent = nullptr);

public slots:
    void detectVersions();

signals:
    void coreVersionDetected(const QString &version);
    void cliVersionDetected(const QString &version);
    void detectionFinished();

private:
    QString getExecutableVersion(const QString &executablePath);
};

// ==================================================

class setting : public QDialog
{
    Q_OBJECT

public:
    explicit setting(QWidget *parent = nullptr);
    ~setting();

    /// @brief 检测软件版本
    /// @param isFromInternal 是否来自内部调用
    /// @warning 外部调用时，检测完成后会自动delete该setting对象，无需再次释放
    void detectSoftwareVersion(const bool &isFromInternal =  false);

    /// @brief 获取自动回连状态
    /// @return 为true时表示设置项中启用了自动回连
    bool isAutoRun() const { return m_autoRun; }

    /// @brief 是否隐藏到系统托盘
    bool isHideOnTray() const { return m_isHideOnTray; }

    /// @brief 是否自动检查更新
    bool isAutoUpdate() const { return m_autoUpdate; }

    /// @brief 获取配置保存路径
    QString getConfigPath() {return m_configPath;};

    /// @brief 是否应该弹出赞助窗口
    /// @return 当启动次数达到50次且未弹出过赞助窗口时返回true
    bool shouldShowDonate() const { return m_shouldShowDonate; }

private slots:
    // 重新检测版本按钮点击事件
    void onDetAgainPushButtonClicked();
    // 打开核心目录按钮点击事件
    void onOpenFileBtnClicked();
    // 确定按钮点击事件
    void onButtonBoxAccepted();
    // 取消按钮点击事件
    void onButtonBoxRejected();
    // 检查更新按钮点击事件
    void onNewVerPushButtonClicked();

    // 版本检测结果处理
    void onCoreVersionDetected(const QString &version);
    void onCliVersionDetected(const QString &version);

private:
    Ui::setting *ui;

    // 配置保存路径
#if SAVE_CONF_IN_APP_DIR == true
    QString m_configPath = QApplication::applicationDirPath() + "/config";
#else
    QString m_configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);;
#endif

    // 启动版本检测线程
    void startVersionDetection();

    // 加载设置
    void loadSettings();
    // 保存设置
    void saveSettings();
    // 设置开机自启
    void setAutoStart(bool enable);

    // 增加启动计数（上限50）
    void incrementLaunchCount();
    // 标记赞助窗口已弹出
    void markDonateShown();

    // 设置项
    bool m_autoRun = false;   // 自动回连
    bool m_autoStart = false; // 是否开机自启
    bool m_isHideOnTray = true; //是否藏窗口到系统托盘
    bool m_autoUpdate = true; // 是否自动检查更新
    QString m_softwareVer = PROJECT_VERSION;

    // 启动计数相关
    int m_launchCount = 0;      // 启动次数计数
    bool m_shouldShowDonate = false; // 是否应该弹出赞助窗口

    // 线程相关
    QThread *m_versionThread =  nullptr;
    VersionDetectionWorker *m_versionWorker =  nullptr;

protected:
    void showEvent(QShowEvent *event) override
    {
        QDialog::showEvent(event);
        // 当窗口被打开的时候，启动版本检测线程
        startVersionDetection();
    }

signals:
    void finishDetectUpdate();
};

#endif // SETTING_H