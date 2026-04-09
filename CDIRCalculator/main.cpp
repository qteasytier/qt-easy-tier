#include "cidrcalculator.h"
#include <QApplication>
#include <QStyleFactory>
#include <iostream>


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    CIDRCalculator calculator;
    calculator.show();

    return app.exec();
}