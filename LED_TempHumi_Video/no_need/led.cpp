#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <QDebug>

static int fd;

void led_init(void)
{

    fd = open("/dev/100ask_led", O_RDWR);//以读写方式打开设备文件，这样程序就可以向设备写入数据来控制LED的状态
    if (fd<0)
    {
        qDebug()<<"open /dev/100ask err";
    }
        write(fd,"131\n",4);
}

void led_control(int on)//控制LED的状态，参数on表示LED的状态，1表示打开LED，0表示关闭LED
{
    char buf[2];
    buf[0]=0;
    if(on)
    {
        buf[1]=0;
    }
    else
    {
        buf[1]=1;
    }
    write(fd,buf,2);
}
