#ifndef DHT11_THREAD_H
#define DHT11_THREAD_H
#include <qthread.h>
#include <QLabel>

class DHT11Thread : public QThread {
public:
    void run() override;
    void SetLabels(QLabel *labelHumi,QLabel *labelTemp);
private:
    QLabel *labelHumi;
    QLabel *labelTemp;
};

#endif // DHT11_THREAD_H
