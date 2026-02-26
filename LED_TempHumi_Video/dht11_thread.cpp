#include <QThread>
#include <QDebug>
#include "dht11_thread.h"
#include "rpc_client.h"

void DHT11Thread::run()
{
    char humi;
    char temp;
    char buf[20];

    int dht_socket = RPC_Client_Init();
    if (dht_socket < 0) {
        qDebug() << "DHT11线程初始化socket失败";
        return;
    }

    //循环读取温湿度
    while(1){
        if (0 == rpc_dht11_read(&humi, &temp,&dht_socket))
        {
            sprintf(buf,"%d%%",humi);
            labelHumi->setText(buf);
            sprintf(buf,"%d",temp);
            labelTemp->setText(buf);
            msleep(1000);
        }
    }
}

// 获取ui界面温湿度标签
void DHT11Thread::SetLabels(QLabel *labelHumi,QLabel *labelTemp)
{
    this->labelHumi=labelHumi;
    this->labelTemp=labelTemp;
}

