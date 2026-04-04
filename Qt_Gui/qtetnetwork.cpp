#include "qtetnetwork.h"

QtETNetwork::QtETNetwork(QWidget *parent)
    : QWidget(parent)
    , m_leftFrame(nullptr)
    , m_leftLayout(nullptr)
    , m_networksList(nullptr)
    , m_newNetworkBtn(nullptr)
    , m_tabWidget(nullptr)
    , m_basicSettingsTab(nullptr)
    , m_advancedSettingsTab(nullptr)
    , m_runningStatusTab(nullptr)
    , m_runningLogTab(nullptr)
    , m_mainLayout(nullptr)
{
    initUI();
}

QtETNetwork::~QtETNetwork() = default;

void QtETNetwork::initUI()
{
    // 创建主布局
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setSpacing(5);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);

    // 初始化左右面板
    initLeftPanel();
    initRightPanel();
}

void QtETNetwork::initLeftPanel()
{
    // 创建左侧面板容器
    m_leftFrame = new QFrame(this);
    m_leftFrame->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    // 创建左侧布局
    m_leftLayout = new QVBoxLayout(m_leftFrame);
    m_leftLayout->setSpacing(5);
    m_leftLayout->setContentsMargins(1, 0, 0, 0);

    // 创建网络列表
    m_networksList = new QtETListWidget(m_leftFrame);
    m_networksList->setMinimumSize(140, 0);
    m_networksList->setMaximumSize(140, QWIDGETSIZE_MAX);

    // 添加测试项
    new QtETListWidgetItem(QStringLiteral("114514"), m_networksList);
    new QtETListWidgetItem(QStringLiteral("222222"), m_networksList);
    new QtETListWidgetItem(QStringLiteral("222222"), m_networksList);
    new QtETListWidgetItem(QStringLiteral("222222"), m_networksList);
    new QtETListWidgetItem(QStringLiteral("222222"), m_networksList);

    m_leftLayout->addWidget(m_networksList);

    // 创建新建网络按钮
    m_newNetworkBtn = new QPushButton("新建网络" ,m_leftFrame);
    m_runNetworkBtn = new QPushButton("运行网络" ,m_leftFrame);
    m_importConfBtn = new QPushButton("导入配置" ,m_leftFrame);
    m_exportConfBtn = new QPushButton("导出配置" ,m_leftFrame);


    m_leftLayout->addWidget(m_newNetworkBtn);
    m_leftLayout->addWidget(m_runNetworkBtn);
    m_leftLayout->addWidget(m_importConfBtn);
    m_leftLayout->addWidget(m_exportConfBtn);

    // 将左侧面板添加到主布局
    m_mainLayout->addWidget(m_leftFrame);
}

void QtETNetwork::initRightPanel()
{
    // 创建选项卡容器
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setCurrentIndex(0);

    // 创建基础设置选项卡
    m_basicSettingsTab = new QWidget();
    m_tabWidget->addTab(m_basicSettingsTab, tr("基础设置"));

    // 创建高级设置选项卡
    m_advancedSettingsTab = new QWidget();
    m_tabWidget->addTab(m_advancedSettingsTab, tr("高级设置"));

    // 创建运行状态选项卡
    m_runningStatusTab = new QWidget();
    m_tabWidget->addTab(m_runningStatusTab, tr("运行状态"));

    // 创建运行日志选项卡
    m_runningLogTab = new QWidget();
    m_tabWidget->addTab(m_runningLogTab, tr("运行日志"));

    // 将选项卡添加到主布局
    m_mainLayout->addWidget(m_tabWidget);
}