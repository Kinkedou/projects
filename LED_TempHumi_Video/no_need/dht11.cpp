#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <QDebug>

static int fd;

void dht11_init(void)
{

    fd = open("/dev/mydht11", O_RDWR | O_NONBLOCK);//以读写方式打开设备文件，并且设置为非阻塞模式，这样在读取数据时，如果没有数据可读，程序不会被阻塞住，而是可以继续执行其他操作
    if (fd<0)
    {
        qDebug()<<"open /dev/mydht11 err";
    }
}

//读取DHT11传感器的数据，并将湿度和温度分别存储在humi和temp指针所指向的变量中
int dht11_read(char *humi, char *temp)
{
    char buf[2];

    if(2==read(fd,buf,2))
    {
        *humi=buf[0];
        *temp=buf[1];
        return 0;
    }
    else
    {
        return -1;
    }
}
