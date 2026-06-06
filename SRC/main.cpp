#include "qtetmain.h"
#include <iostream>
#include <QApplication>
#include <QStyleFactory>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QNetworkReply>
#include <QPalette>
#include <QColor>

void isAlreadyRunning(const QString& serverName, bool isAutoStart);

int main(int argc, char *argv[])
{
    bool isAutoStart = false;
    for (int i = 0; i < argc; ++i) {
        if (QString(argv[i]) == "--auto-start") {
            isAutoStart = true;
            break;
        }
    }
    QApplication app(argc, argv);

    // 检查是否已有实例运行（确保单实例）
    QString serverName = "QtEasyTier2-by-Myqfeng";
    isAlreadyRunning(serverName, isAutoStart);   // 有实例运行自动退出

    // 设置调色板高亮主题色为 #66ccff
    QPalette palette = app.palette();
    palette.setColor(QPalette::Highlight, QColor("#66ccff"));
    palette.setColor(QPalette::HighlightedText, QColor("#000000"));
    app.setPalette(palette);

    const QIcon appIcon(QStringLiteral(":/icons/icon.png"));
    app.setWindowIcon(appIcon);

    QtETMain qtetmain(nullptr);
    qtetmain.setWindowIcon(appIcon);

    if (!isAutoStart) {
        qtetmain.show();
    }

    return app.exec();
}

// 检查是否已有实例运行
void isAlreadyRunning(const QString& serverName, bool isAutoStart) {
    // 创建本地套接字，尝试连接指定名称的服务器
    QLocalSocket socket;
    socket.connectToServer(serverName);

    // 如果连接成功，说明已有实例
    if (socket.waitForConnected(5000)) {
        std::clog << "已有实例运行，本程序退出" << std::endl;
        if (!isAutoStart) {
            // 非自动启动，提示用户已有实例运行,设置图标
            QMessageBox msgBox;
            msgBox.setWindowIcon(QIcon(QStringLiteral(":/icons/icon.ico")));
            msgBox.setWindowTitle("Tip");
            msgBox.setText("QtEasyTier 已在运行");
            msgBox.exec();
        }
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

    // 处理后续实例的连接请求（用于唤醒已运行实例等功能）
    QObject::connect(localServer, &QLocalServer::newConnection, [localServer]() {
        // 接受连接但不做任何处理，仅用于检测实例存在
        QLocalSocket *clientConnection = localServer->nextPendingConnection();
        if (clientConnection) {
            clientConnection->deleteLater();
        }
    });
}
