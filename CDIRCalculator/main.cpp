#include "cidrcalculator.h"
#include <QApplication>
#include <QStyleFactory>
#include <iostream>


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
#endif

    CIDRCalculator calculator;
    calculator.show();

    return app.exec();
}