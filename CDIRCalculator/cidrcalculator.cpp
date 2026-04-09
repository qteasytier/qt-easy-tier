#include "cidrcalculator.h"
#include <iostream>

CIDRCalculator::CIDRCalculator(QWidget *parent)
    : QMainWindow(parent)
{
    // 设置窗口属性
    setWindowTitle("CIDR计算器");
    setFixedSize(500, 320);
    //setAttribute(Qt::WA_DeleteOnClose); // 关闭即销毁
    setWindowIcon(QIcon(":/icons/icon.ico"));

    // 设置中央部件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 创建UI
    setupUI();

    // 设置输入验证 - 移到这里，在UI组件创建之后
    QString ipPattern = "^(?:[0-9]{1,3}\\.){3}[0-9]{1,3}$";
    QRegularExpression ipRegex(ipPattern);
    QRegularExpressionValidator *ipValidator = new QRegularExpressionValidator(ipRegex, this);

    rangeStartInput->setValidator(ipValidator);
    rangeEndInput->setValidator(ipValidator);

    // 连接信号槽
    connect(cidrCalculateBtn, &QPushButton::clicked, this, &CIDRCalculator::calculateFromCIDR);
    connect(rangeCalculateBtn, &QPushButton::clicked, this, &CIDRCalculator::calculateFromRange);
    connect(copyCIDRBtn, &QPushButton::clicked, this, &CIDRCalculator::copyCIDRResults);
    connect(cidrInput, &QLineEdit::textChanged, this, &CIDRCalculator::validateCIDRInput);
    connect(rangeStartInput, &QLineEdit::textChanged, this, &CIDRCalculator::validateIPInput);
    connect(rangeEndInput, &QLineEdit::textChanged, this, &CIDRCalculator::validateIPInput);
}

CIDRCalculator::~CIDRCalculator() {

}

void CIDRCalculator::setupUI()
{
    // 获取中央部件
    QWidget *centralWidget = this->centralWidget();

    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // 创建TabWidget
    tabWidget = new QtETTabWidget();

    // ========== 第一页：CIDR到范围 ==========
    cidrToRangeTab = new QWidget();
    QGridLayout *cidrLayout = new QGridLayout(cidrToRangeTab);

    // 输入组
    QGroupBox *inputGroup = new QGroupBox("输入CIDR表示法");
    QVBoxLayout *inputLayout = new QVBoxLayout(inputGroup);

    QLabel *cidrLabel = new QLabel("请输入CIDR (例如: 192.168.1.0/24):");
    cidrInput = new QtETLineEdit();
    cidrInput->setPlaceholderText("例如: 192.168.1.0/24");
    cidrCalculateBtn = new QtETPushBtn("计算");

    inputLayout->addWidget(cidrLabel);
    inputLayout->addWidget(cidrInput);
    inputLayout->addWidget(cidrCalculateBtn);

    // 输出组
    QGroupBox *outputGroup = new QGroupBox("计算结果");
    QGridLayout *outputLayout = new QGridLayout(outputGroup);

    outputLayout->addWidget(new QLabel("起始IP:"), 0, 0);
    startIPOutput = new QtETLineEdit();
    startIPOutput->setReadOnly(true);
    outputLayout->addWidget(startIPOutput, 0, 1);

    outputLayout->addWidget(new QLabel("结束IP:"), 1, 0);
    endIPOutput = new QtETLineEdit();
    endIPOutput->setReadOnly(true);
    outputLayout->addWidget(endIPOutput, 1, 1);

    outputLayout->addWidget(new QLabel("IP总数:"), 2, 0);
    totalIPsOutput = new QtETLineEdit();
    totalIPsOutput->setReadOnly(true);
    outputLayout->addWidget(totalIPsOutput, 2, 1);

    // 布局
    cidrLayout->addWidget(inputGroup, 0, 0);
    cidrLayout->addWidget(outputGroup, 1, 0);

    // ========== 第二页：范围到CIDR ==========
    rangeToCIDRTab = new QWidget();
    QGridLayout *rangeLayout = new QGridLayout(rangeToCIDRTab);

    // 输入组
    QGroupBox *rangeInputGroup = new QGroupBox("输入IP地址范围");
    QGridLayout *rangeInputLayout = new QGridLayout(rangeInputGroup);

    rangeInputLayout->addWidget(new QLabel("起始IP:"), 0, 0);
    rangeStartInput = new QtETLineEdit();
    rangeStartInput->setPlaceholderText("例如: 192.168.1.0");
    rangeInputLayout->addWidget(rangeStartInput, 0, 1);

    rangeInputLayout->addWidget(new QLabel("结束IP:"), 1, 0);
    rangeEndInput = new QtETLineEdit();
    rangeEndInput->setPlaceholderText("例如: 192.168.1.255");
    rangeInputLayout->addWidget(rangeEndInput, 1, 1);

    rangeCalculateBtn = new QtETPushBtn("计算CIDR");
    rangeInputLayout->addWidget(rangeCalculateBtn, 2, 0, 1, 2);

    // 输出组
    QGroupBox *rangeOutputGroup = new QGroupBox("CIDR结果");
    QVBoxLayout *rangeOutputLayout = new QVBoxLayout(rangeOutputGroup);

    cidrResultsOutput = new QtETLineEdit();
    cidrResultsOutput->setReadOnly(true);
    // 不设置border，让QtETLineEdit自定义绘制处理边框
    cidrResultsOutput->setStyleSheet("font-size: 12px;");
    cidrResultsOutput->setText("CIDR计算结果将显示在这里...");
    rangeOutputLayout->addWidget(cidrResultsOutput);

    copyCIDRBtn = new QtETPushBtn("复制CIDR");

    // 布局
    rangeLayout->addWidget(rangeInputGroup, 0, 0);
    rangeLayout->addWidget(rangeOutputGroup, 1, 0);
    rangeLayout->addWidget(copyCIDRBtn, 2, 0, Qt::AlignRight);

    // 添加标签页
    tabWidget->addTab(cidrToRangeTab, "CIDR → IP范围");
    tabWidget->addTab(rangeToCIDRTab, "IP范围 → CIDR");

    mainLayout->addWidget(tabWidget);

    // 设置默认值以便测试
    cidrInput->setText("192.168.1.0/24");
    rangeStartInput->setText("192.168.1.0");
    rangeEndInput->setText("192.168.1.255");
}

bool CIDRCalculator::isValidIP(const QString &ip)
{
    QStringList parts = ip.split('.');
    if (parts.size() != 4) return false;

    for (const QString &part : parts) {
        bool ok;
        int num = part.toInt(&ok);
        if (!ok || num < 0 || num > 255) return false;
    }
    return true;
}

bool CIDRCalculator::isValidCIDR(const QString &cidr)
{
    QStringList parts = cidr.split('/');
    if (parts.size() != 2) return false;

    if (!isValidIP(parts[0])) return false;

    bool ok;
    int prefix = parts[1].toInt(&ok);
    if (!ok || prefix < 0 || prefix > 32) return false;

    return true;
}

quint32 CIDRCalculator::ipToInt(const QString &ip)
{
    QStringList parts = ip.split('.');
    quint32 result = 0;

    for (int i = 0; i < 4; i++) {
        result = (result << 8) | parts[i].toUInt();
    }

    return result;
}

QString CIDRCalculator::intToIP(quint32 ip)
{
    QString result;
    for (int i = 3; i >= 0; i--) {
        quint8 octet = (ip >> (i * 8)) & 0xFF;
        result.append(QString::number(octet));
        if (i > 0) result.append('.');
    }
    return result.trimmed();
}

quint32 CIDRCalculator::getNetworkAddress(quint32 ip, int prefix)
{
    quint32 mask = (prefix == 0) ? 0 : ~((1 << (32 - prefix)) - 1);
    return ip & mask;
}

quint32 CIDRCalculator::getBroadcastAddress(quint32 ip, int prefix)
{
    if (prefix == 32) return ip;
    quint32 mask = (1 << (32 - prefix)) - 1;
    return getNetworkAddress(ip, prefix) | mask;
}

void CIDRCalculator::calculateCIDRRange(const QString &cidr)
{
    QStringList parts = cidr.split('/');
    QString ipStr = parts[0];
    int prefix = parts[1].toInt();

    quint32 ip = ipToInt(ipStr);
    quint32 network = getNetworkAddress(ip, prefix);
    quint32 broadcast = getBroadcastAddress(ip, prefix);

    // 网络地址和广播地址之间的就是可用地址
    quint32 startIP = network;
    quint32 endIP = broadcast;

    // 计算可用主机数
    quint64 totalIPs;
    if (prefix == 32) {
        totalIPs = 1;
    } else {
        totalIPs = 1ULL << (32 - prefix);
    }

    // 如果是网络地址，通常从网络地址+1开始
    if (prefix <= 30) {
        startIP = network + 1;
        endIP = broadcast - 1;
        totalIPs -= 2; // 减去网络地址和广播地址
    }

    startIPOutput->setText(intToIP(startIP));
    endIPOutput->setText(intToIP(endIP));
    totalIPsOutput->setText(QString::number(totalIPs));
}

QString CIDRCalculator::rangeToCIDR(const QString &startIPStr, const QString &endIPStr)
{
    quint32 start = ipToInt(startIPStr);
    quint32 end = ipToInt(endIPStr);

    if (start > end) {
        std::swap(start, end);
    }

    // 找到包含整个IP范围的最小CIDR前缀
    int prefix = 32;
    while (prefix >= 0) {
        quint32 network = getNetworkAddress(start, prefix);
        quint32 broadcast = getBroadcastAddress(start, prefix);
        
        // 检查这个CIDR是否包含整个IP范围
        if (network <= start && broadcast >= end) {
            // 尝试更大的前缀（更小的网络）是否也能包含
            int testPrefix = prefix + 1;
            while (testPrefix <= 32) {
                quint32 testNetwork = getNetworkAddress(start, testPrefix);
                quint32 testBroadcast = getBroadcastAddress(start, testPrefix);
                
                // 如果测试的前缀也包含整个范围，则使用更大的前缀
                if (testNetwork <= start && testBroadcast >= end) {
                    prefix = testPrefix;
                    testPrefix++;
                } else {
                    break;
                }
            }
            break;
        }
        prefix--;
    }

    // 构建CIDR表示法
    return intToIP(getNetworkAddress(start, prefix)) + "/" + QString::number(prefix);
}

bool CIDRCalculator::isValidIPRange(const QString &startIP, const QString &endIP)
{
    if (!isValidIP(startIP) || !isValidIP(endIP)) return false;

    quint32 start = ipToInt(startIP);
    quint32 end = ipToInt(endIP);

    return start <= end;
}

void CIDRCalculator::calculateFromCIDR()
{
    QString cidr = cidrInput->text().trimmed();

    if (!isValidCIDR(cidr)) {
        QMessageBox::warning(this, "输入错误",
            "请输入有效的CIDR表示法！\n"
            "格式: xxx.xxx.xxx.xxx/yy (yy范围: 0-32)\n"
            "例如: 192.168.1.0/24");
        return;
    }

    try {
        calculateCIDRRange(cidr);
    } catch (...) {
        QMessageBox::critical(this, "计算错误", "CIDR计算过程中发生错误！");
    }
}

void CIDRCalculator::calculateFromRange()
{
    QString startIP = rangeStartInput->text().trimmed();
    QString endIP = rangeEndInput->text().trimmed();

    if (!isValidIPRange(startIP, endIP)) {
        QMessageBox::warning(this, "输入错误",
            "请输入有效的IP地址范围！\n"
            "起始IP必须小于等于结束IP\n"
            "IP地址格式: xxx.xxx.xxx.xxx (0-255)");
        return;
    }

    try {
        QString cidrResults = rangeToCIDR(startIP, endIP);
        cidrResultsOutput->setText(cidrResults);
    } catch (...) {
        QMessageBox::critical(this, "计算错误", "IP范围计算过程中发生错误！");
    }
}
void CIDRCalculator::copyCIDRResults()
{
    QString cidrResults = cidrResultsOutput->text().trimmed();
    if (!cidrResults.isEmpty()) {
        QApplication::clipboard()->setText(cidrResults);
        QMessageBox::information(this, "复制成功", QApplication::clipboard()->text() + "已复制！");
    } else {
        QMessageBox::warning(this, "复制失败", "没有内容可复制！");
    }
}

void CIDRCalculator::validateIPInput()
{
    QString startIP = rangeStartInput->text();
    QString endIP = rangeEndInput->text();

    bool startValid = isValidIP(startIP);
    bool endValid = isValidIP(endIP);

}

void CIDRCalculator::validateCIDRInput()
{
    QString cidr = cidrInput->text();
    bool valid = isValidCIDR(cidr);
}