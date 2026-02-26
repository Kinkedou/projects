#include "mainwindow.h"
#include "rpc_client.h"
#include "dht11_thread.h"
#include <QApplication>
#include "video_thread.h"
#include "secondwindow.h"
#include <stdio.h>

// 视频采集线程是全局变量，供其他文件调用
VideoThread g_vthread;

int main(int argc, char *argv[])
{
    RPC_Client_Init();
    QLabel *labelHumi;
    QLabel *labelTemp;

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    labelHumi = w.GetHumiLabel();
    labelTemp = w.GetTempLabel();

    // 创建温湿度读取线程并启动
    DHT11Thread thread;
    thread.SetLabels(labelHumi, labelTemp);
    thread.start();

    // 启动视频采集线程
    QObject::connect(&g_vthread, &VideoThread::updateVideo, w.p_swindow, &secondwindow::updateVideoLabel);
    g_vthread.start();
    return a.exec();

}
