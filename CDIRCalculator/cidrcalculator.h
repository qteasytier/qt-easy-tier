#ifndef CIDRCALCULATOR_H
#define CIDRCALCULATOR_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QGroupBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QDebug>
#include <vector>

class CIDRCalculator : public QMainWindow
{
    Q_OBJECT

public:
    CIDRCalculator(QWidget *parent = nullptr);
    ~CIDRCalculator();

private slots:
    void calculateFromCIDR();
    void calculateFromRange();
    void copyCIDRResults();
    void validateIPInput();
    void validateCIDRInput();

private:
    // 创建UI组件
    void setupUI();

    // CIDR计算相关方法
    bool isValidCIDR(const QString &cidr);
    bool isValidIP(const QString &ip);
    quint32 ipToInt(const QString &ip);
    QString intToIP(quint32 ip);
    void calculateCIDRRange(const QString &cidr);

    // IP范围计算相关方法
    bool isValidIPRange(const QString &startIP, const QString &endIP);
    QString rangeToCIDR(const QString &startIP, const QString &endIP);
    quint32 getNetworkAddress(quint32 ip, int prefix);
    quint32 getBroadcastAddress(quint32 ip, int prefix);

    // UI组件
    QTabWidget *tabWidget;

    // CIDR到范围页面
    QWidget *cidrToRangeTab;
    QLineEdit *cidrInput;
    QPushButton *cidrCalculateBtn;
    QLineEdit *startIPOutput;
    QLineEdit *endIPOutput;
    QLineEdit *totalIPsOutput;

    // 范围到CIDR页面
    QWidget *rangeToCIDRTab;
    QLineEdit *rangeStartInput;
    QLineEdit *rangeEndInput;
    QPushButton *rangeCalculateBtn;
    QLineEdit *cidrResultsOutput;
    QPushButton *copyCIDRBtn;
};

#endif // CIDRCALCULATOR_H