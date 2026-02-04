#ifndef SETTING_H
#define SETTING_H

#include <QDialog>
#include <QProcess>
#include <QSettings>
#include <QJsonObject>
#include <QThread>

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

    ///@brief 外部调用检查版本更新
    static void detectSoftWareVersionExternal() {
        auto *tmp = new setting();
        tmp->detectSoftwareVersion();
    }

    QString getSoftwareVersion() { return m_softwareVer; }

private slots:
    // 重新检测版本按钮点击事件
    void on_detAgainPushButton_clicked();
    // 打开核心目录按钮点击事件
    void on_pushButton_2_clicked();
    // 确定按钮点击事件
    void on_buttonBox_accepted();
    // 取消按钮点击事件
    void on_buttonBox_rejected();
    // 检查更新按钮点击事件
    void on_newVerPushButton_clicked();

    ///@brief 检查软件版本
    ///@param isFromBtn 是否来自按钮点击
    ///@warning 值为false时，检查完毕后会自动销毁setting对象
    void detectSoftwareVersion(bool isFromBtn =  false);

    // 版本检测结果处理
    void onCoreVersionDetected(const QString &version);
    void onCliVersionDetected(const QString &version);

private:
    Ui::setting *ui;

    // 启动版本检测线程
    void startVersionDetection();

    // 加载设置
    void loadSettings();
    // 保存设置
    void saveSettings();
    // 设置开机自启
    void setAutoStart(bool enable);

    // 设置项
    bool m_autoStart; // 是否开机自启
    QString m_softwareVer = "1.0.4";

    // 线程相关
    QThread *m_versionThread;
    VersionDetectionWorker *m_versionWorker;

protected:
    void showEvent(QShowEvent *event) override
    {
        QDialog::showEvent(event);
        // 当窗口被打开的时候，启动版本检测线程
        startVersionDetection();
    }
};
inline bool g_autoRun; // 自动运行上次关闭前没退出的网络

#endif // SETTING_H