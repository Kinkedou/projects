#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtDebug>
#include "rpc_client.h"
#include <QPixmap>
#include <QThread>
#include "video_thread.h"

extern VideoThread g_vthread;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    p_swindow=new secondwindow(nullptr);
    p_swindow->hide();
    //通过findChild函数找到界面上对应的标签，并将其赋值给labelHumi和labelTemp
    labelHumi = this->findChild<QLabel*>("label");
    labelTemp = this->findChild<QLabel*>("label_2");
    // 连接 pushButton 的点击信号到 LED 槽函数
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::Button_LED_0n_Clicked);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &MainWindow::Button_Video_0n_Clicked);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &MainWindow::Button_gaojing_0n_Clicked);
    connect(ui->pushButton_4, &QPushButton::clicked, this, &MainWindow::Button_detect_0n_Clicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

QLabel* MainWindow::GetHumiLabel()//获取湿度标签的函数实现
{
    return labelHumi;
}
QLabel* MainWindow::GetTempLabel()//获取温度标签的函数实现
{
    return labelTemp;
}

//监控按钮
void MainWindow::Button_Video_0n_Clicked()
{
    g_vthread.canSend();
    p_swindow->show();

}

//移动监测按钮
void MainWindow::Button_detect_0n_Clicked()
{
    p_swindow->Button_detect_0n_Clicked();
}


//告警记录按钮
void MainWindow::Button_gaojing_0n_Clicked()
{
    p_swindow->show();
    p_swindow->Button_Looked_0n_Clicked();
}

//照明按钮
void MainWindow::Button_LED_0n_Clicked()
{
    static int status = 1;//定义一个静态变量status，初始值为1，用于记录LED的状态
    if(status)
    {
        qDebug()<<"LED cilcked on";
    }
    else
        qDebug()<<"LED clicked off";
       rpc_led_control(status);
       status = !status;
}

