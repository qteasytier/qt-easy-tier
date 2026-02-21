#include "mainwindow.h"
#include "setting.h"
#include <iostream>
#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>

void isAlreadyRunning(const QString& serverName);

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 检查是否已有实例运行（确保单实例）
    QString serverName = "QtEasyTierbyViahuang";
    isAlreadyRunning(serverName);   // 有实例运行自动退出

    // 使用Breeze主题
#ifdef WIN32
    QStringList availableStyles = QStyleFactory::keys();
    if (availableStyles.contains("Breeze")) {
        // 2. 加载并创建 Breeze 样式实例
        QStyle *breezeStyle = QStyleFactory::create("Breeze");
        app.setStyle(breezeStyle);
        std::clog << "成功加载并应用 Breeze 样式" << std::endl;
    } else {
        std::clog << "Breeze 样式不可用，使用默认样式" << std::endl;
    }
#endif

    MainWindow w;

    bool autoStart = false;
    for (int i = 0; i < argc; ++i) {
        if (QString(argv[i]) == "--auto-start") {
            autoStart = true;
            break;
        }
    }

    if (!autoStart) {
        w.show();
    }

    return app.exec();
}

// 检查是否已有实例运行
void isAlreadyRunning(const QString& serverName) {
    // 创建本地套接字，尝试连接指定名称的服务器
    QLocalSocket socket;
    socket.connectToServer(serverName);

    // 如果连接成功，说明已有实例
    if (socket.waitForConnected(500)) {
        std::clog << "已有实例运行，本程序退出" << std::endl;
        QMessageBox::information(nullptr, "Tip", "QtEasyTier 已在运行");
        std::exit(0);
    }

    // 连接失败，说明是第一个实例，启动本地服务器
    auto localServer = new QLocalServer(qApp);
    // 监听指定名称的本地套接字
    if (!localServer->listen(serverName)) {
        // 如果监听失败（可能是残留的套接字文件），先移除再重试
        QLocalServer::removeServer(serverName);
        localServer->listen(serverName);
    }
}
