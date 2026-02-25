#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

static int fd;

// LED控制函数：向设备文件写入控制命令，打开或关闭LED
void led_init(void)
{
    fd = open("/dev/100ask_led", O_RDWR);
    if (fd < 0)
    {
    }
}

// LED控制函数：根据参数on的值，向设备文件写入命令，控制LED开关
void led_control(int on)
{
    char buf[2];
    buf[0] = 0;

    if (on)
    {
        buf[1] = 0;
    }
    else
    {
        buf[1] = 1;
    }
    write(fd, buf, 2);
}
