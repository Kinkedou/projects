#ifndef VIDEO_THREAD_H
#define VIDEO_THREAD_H
#include <qthread.h>
#include <QLabel>
#include <QMutex>

class VideoThread : public QThread {
    Q_OBJECT
public:
    void run() override;
    int flag;
    static QMutex m_mutex;
    void canSend();
    void noSend();
signals:
    // 发送解析后的图像信号
    void updateVideo(const QImage &frame);
};

#endif // VIDEO_THREAD_H




