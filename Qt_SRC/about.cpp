#include "../Qt_HEAD/about.h"
#include "ui_about.h"

About::About(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::About)
{
    ui->setupUi(this);

    this->setAttribute(Qt::WA_DeleteOnClose, true);

    // 点击关闭按钮时，关闭对话框
    connect(ui->pushButton, &QPushButton::clicked, this, &About::close);
}

About::~About()
{
    delete ui;
}
