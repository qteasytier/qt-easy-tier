#ifndef SETTING_H
#define SETTING_H

#include <QDialog>
#include <QProcess>
#include <QSettings>
#include <QJsonObject>

namespace Ui {
    class setting;
}

class setting : public QDialog
{
    Q_OBJECT

public:
    explicit setting(QWidget *parent = nullptr);
    ~setting();

private slots:
    // 重新检测版本按钮点击事件
    void on_detAgainPushButton_clicked();
    // 打开核心目录按钮点击事件
    void on_pushButton_2_clicked();
    // 检查更新按钮点击事件
    void on_newVerPushButton_clicked();
    // 确定按钮点击事件
    void on_buttonBox_accepted();
    // 取消按钮点击事件
    void on_buttonBox_rejected();

private:
    Ui::setting *ui;

    // 检测EasyTier版本
    void detectEasyTierVersions();
    // 获取可执行文件版本
    QString getExecutableVersion(const QString &executablePath);
    // 加载设置
    void loadSettings();
    // 保存设置
    void saveSettings();
    // 设置开机自启
    void setAutoStart(bool enable);

    // 设置项
    bool m_autoRun; // 自动运行上次关闭前没退出的网络
    bool m_autoStart; // 是否开机自启
};

#endif // SETTING_H
