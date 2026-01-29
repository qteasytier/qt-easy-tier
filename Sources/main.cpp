#include "mainwindow.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include <QSystemTrayIcon>

int main(int argc, char *argv[])
{

    QApplication app(argc, argv);

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
#elif __linux__
    QStyle *breezeStyle = QStyleFactory::create("Breeze");
    app.setStyle(breezeStyle);
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
