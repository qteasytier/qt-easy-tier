#include "ui_qtetmain.h"
#include "qtetmain.h"
#include "qtetcheckbtn.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QFontDatabase>
#include <QPalette>
#include <QStyle>
#include <QStyleHints>

QtETMain::QtETMain(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::QtETMain)
{
    ui->setupUi(this);

// =============== 初始化调色板 ===============
    // 设置一次调色板
    const Qt::ColorScheme &currentScheme = QGuiApplication::styleHints()->colorScheme();
    onSchemeChanged(currentScheme);
    // 监测系统调色板变化
    const QStyleHints *hints = QGuiApplication::styleHints();
    connect(hints, &QStyleHints::colorSchemeChanged, this, &QtETMain::onSchemeChanged);

// =============== 初始化欢迎界面 ===============
    initHelloPage();
}

QtETMain::~QtETMain()
{
    delete ui;
}

/// @brief 处理系统调色板变化
void QtETMain::onSchemeChanged(const Qt::ColorScheme &scheme)
{
    if (scheme == Qt::ColorScheme::Dark) {

        // 处理暗黑模式，设置sideWidget背景调色板为深色
        ui->sideBox->setPalette(QPalette(QColor("#131516")));
        ui->sideBox->setAutoFillBackground(true);
        // 强制更新一次应用的调色板
        qApp->style()->unpolish(qApp);
        qApp->style()->polish(qApp);
        qApp->processEvents();


    } else {
        // 处理亮色模式，设置sideWidget背景调色板为浅色
        ui->sideBox->setPalette(QPalette(QColor("#f6f7f7")));
        ui->sideBox->setAutoFillBackground(true);
        // 强制更新一次应用的调色板
        qApp->style()->unpolish(qApp);
        qApp->style()->polish(qApp);
        qApp->processEvents();
    }
}

/// @brief 初始化欢迎界面
void QtETMain::initHelloPage()
{
    //获取布局
    auto *mainLayout = qobject_cast<QVBoxLayout*>(m_helloPage->layout());
    mainLayout->setSpacing(10);

    // 主标题（图标 + 标题水平布局）
    QHBoxLayout *titleLayout = new QHBoxLayout();
    titleLayout->setSpacing(10);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QPixmap(":/icons/icon.ico").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    titleLayout->addWidget(iconLabel);

    QLabel *titleLabel = new QLabel("QtEasyTier", this);
    int fontId = QFontDatabase::addApplicationFont(":/icons/Ubuntu-B.ttf");
    QFont titleFont;
    if (fontId != -1) {
        QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty()) {
            titleFont = QFont(families.first(), 48);
        }
    } else {
        titleFont.setPointSize(48);
    }
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLayout->addWidget(titleLabel);

    titleLayout->setAlignment(Qt::AlignCenter);
    mainLayout->addLayout(titleLayout);

    // 副标题
    QLabel *subTitleLabel = new QLabel("简洁实用的异地组网工具 | 基于 EasyTier", this);
    QFont subTitleFont;
    subTitleFont.setPointSize(12);
    subTitleLabel->setFont(subTitleFont);
    subTitleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(subTitleLabel);

    // 纵向弹簧
    mainLayout->addStretch();

    // 测试QtETCheckBtn按钮功能
    QtETCheckBtn *testBtn = new QtETCheckBtn("测试按钮", this);
    testBtn->setToolTip("Genshin");
    mainLayout->addWidget(testBtn);

    // 功能按钮区域 (2x2 网格布局)
    QGridLayout *buttonGrid = new QGridLayout();
    buttonGrid->setSpacing(6);

    m_aboutUsBtn = new QPushButton("关于项目", this);
    m_aboutUsBtn->setFixedWidth(125);
    m_aboutETBtn = new QPushButton("关于 EasyTier", this);
    m_aboutETBtn->setFixedWidth(125);
    m_donateBtn = new QPushButton("捐赠", this);
    m_donateBtn->setFixedWidth(125);
    m_notClickBtn = new QPushButton("千万别点", this);
    m_notClickBtn->setFixedWidth(125);

    buttonGrid->addWidget(m_aboutUsBtn, 0, 0);
    buttonGrid->addWidget(m_aboutETBtn, 0, 1);
    buttonGrid->addWidget(m_donateBtn, 1, 0);
    buttonGrid->addWidget(m_notClickBtn, 1, 1);
    buttonGrid->setAlignment(Qt::AlignCenter);

    mainLayout->addLayout(buttonGrid);

    // 版权信息
    QLabel *copyrightLabel = new QLabel("Copyright © 2026 明月清风. All rights reserved.", this);
    QFont copyrightFont;
    copyrightFont.setPointSize(10);
    copyrightLabel->setFont(copyrightFont);
    copyrightLabel->setStyleSheet("color: #aaaaaa;");
    copyrightLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(copyrightLabel);

    m_mainStackedWidget->setCurrentWidget(m_helloPage);
}
