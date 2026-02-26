#include "video_thread.h"
#include <QThread>
#include <QDebug>
#include <QByteArray>
#include "rpc_client.h"
#include <QMutex>

// 初始化互斥锁
QMutex VideoThread::m_mutex; 

// 视频采集线程
void VideoThread::run()
{
    unsigned char buffer[150000];
    int dataLen;
    {
        QMutexLocker locker(&m_mutex);
        flag = 0;
    }
    int video_socket = RPC_Client_Init();
    if (video_socket < 0) {
        qDebug() << "video线程初始化socket失败";
        return;
    }
    // 循环读取
    while(1){
        dataLen = rpc_video_read(buffer,&video_socket);
        if(dataLen>0)
        {
            // 将unsigned char数组转为QByteArray
            QByteArray jpegData(reinterpret_cast<char*>(buffer), dataLen);
            QImage frame;
            frame.loadFromData(jpegData, "JPEG");
            // 加锁判断是否更新到ui界面
            {
                QMutexLocker locker(&m_mutex);
                if(flag==1)
                emit updateVideo(frame);
            }
        }
        msleep(33);
    }
}

// 允许更新视频画面
void VideoThread::canSend()
{
    QMutexLocker locker(&m_mutex);
    flag = 1;
}

// 禁止更新视频画面
void VideoThread::noSend()
{
    QMutexLocker locker(&m_mutex);
    flag=0;
}
