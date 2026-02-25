#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/videodev2.h>
#include <errno.h>

static unsigned char* buffs[4];                                 // 缓冲队列
static unsigned char* buff;                                     // 当前帧数据缓冲区
static int buffcnt = 4;                                         // 缓冲区数量
static int maxlen;                                              // 每个buffer的最大长度
static int curindex;                                            // 当前帧数据所在buffer索引
static int curlen;                                              // 当前帧数据长度
static pthread_cond_t g_frame_cond = PTHREAD_COND_INITIALIZER;  // 新帧条件变量
static int g_has_new_frame = 0;                                 // 新帧标志（0：无新帧，1：有新帧）
static int iFd = -1;                                            // 视频设备文件描述符
static pthread_t g_v4l2_tid;                                    // 视频读取线程ID
static pthread_mutex_t g_buff_mutex = PTHREAD_MUTEX_INITIALIZER;// 保护buff的互斥锁
volatile int g_v4l2_exit_flag = 0;                              // 新增：线程退出标志

static int g_aiSupportedFormats[] = { V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_MJPEG, V4L2_PIX_FMT_RGB565 }; // 支持的像素格式列表

// Base64 编码表
static const char base64_table[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// 检查设备是否支持指定的像素格式
int isSupportThisFormat(int iPixelFormat)
{
    int i;
    for (i = 0; i < sizeof(g_aiSupportedFormats)/sizeof(g_aiSupportedFormats[0]); i++)
    {
        if (g_aiSupportedFormats[i] == iPixelFormat)
            return 1;
    }
    return 0;
}

// 将用过一帧数据放回队列
int V4l2PutFrameForStreaming()
{
    struct v4l2_buffer tV4l2Buf;
    int iError;

    memset(&tV4l2Buf, 0, sizeof(struct v4l2_buffer));
    tV4l2Buf.index = curindex;
    tV4l2Buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    tV4l2Buf.memory = V4L2_MEMORY_MMAP;
    iError = ioctl(iFd, VIDIOC_QBUF, &tV4l2Buf);//把buffer放入队列
    if (iError)
    {
        printf("Unable to queue buffer.\n");
        return -1;
    }
    return 0;
}

// 视频读取线程函数：使用poll等待新帧，获取帧数据，更新共享变量，并通知有新帧
void* V4l2_thread(void* arg)
{
    struct pollfd tFds[1];
    int iRet;
    struct v4l2_buffer tV4l2Buf;

    
    tFds[0].fd = iFd;
    tFds[0].events = POLLIN;    //POLLIN: 有数据可以读
    while (!g_v4l2_exit_flag)   //退出标志检测
    {
        iRet = poll(tFds, 1, 100);  //阻塞等待
        if (iRet < 0)
        {
            printf("poll error! errno\n");
            continue;
        }
        else if (iRet == 0)
        {
            continue;               // 超时，继续检测退出标志
        }
        
        memset(&tV4l2Buf, 0, sizeof(struct v4l2_buffer));
        tV4l2Buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        tV4l2Buf.memory = V4L2_MEMORY_MMAP;
		iRet = ioctl(iFd, VIDIOC_DQBUF, &tV4l2Buf);//从队列中取出一帧数据
        if (iRet < 0)
        {
            printf("Unable to dequeue buffer %d\n",errno);
            continue;
        }

        //互斥锁保护共享变量buff
        pthread_mutex_lock(&g_buff_mutex);
		curindex = tV4l2Buf.index;          // 更新当前帧所在buffer索引
		buff = buffs[tV4l2Buf.index];       // 更新当前帧数据指针
		curlen = tV4l2Buf.bytesused;        // 更新当前帧数据长度
		g_has_new_frame = 1;                // 设置新帧标志
		pthread_cond_signal(&g_frame_cond); //通知有新帧
        pthread_mutex_unlock(&g_buff_mutex);
		V4l2PutFrameForStreaming();         //把buffer放回队列，准备下一帧数据
    }
    printf("V4l2 thread exit\n");
    return NULL;
}

// 启动视频读取线程，开始捕获视频帧
int V4l2StartDevice()
{
    int iType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int iError;

    iError = ioctl(iFd, VIDIOC_STREAMON, &iType);
    if (iError)
    {
        printf("Unable to start capture.\n");
        return -1;
    }
    g_v4l2_exit_flag = 0;  // 重置退出标志
    return 0;
}

// 初始化视频设备：打开设备，查询能力，设置格式，申请buffer，并映射到用户空间
int v4l2_init()
{
    int i;
    int iError;
    
	struct v4l2_fmtdesc tFmtDesc;
    struct v4l2_format  tV4l2Fmt;
    struct v4l2_requestbuffers tV4l2ReqBuffs;
    struct v4l2_buffer tV4l2Buf;
    struct v4l2_capability tV4l2Cap;

    int iLcdWidth;
    int iLcdHeigt;
    int iLcdBpp;
    int format;

    //打开设备
	iFd = open("/dev/video1", O_RDWR);
    if (iFd < 0)
    {
        printf("can not open /dev/video1\n");
        return -1;
    }

    // 查询设备的能力，是否视频捕捉设备,支持哪种接口(streaming，read/write)
    memset(&tV4l2Cap, 0, sizeof(struct v4l2_capability));
	iError = ioctl(iFd, VIDIOC_QUERYCAP, &tV4l2Cap);
    if (iError) {
    	printf("Error opening device /dev/video1: unable to query device.\n");
    	goto err_exit;
    }
    if (!(tV4l2Cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
    	printf("/dev/video1 is not a video capture device\n");
        goto err_exit;
    }
	if (tV4l2Cap.capabilities & V4L2_CAP_STREAMING) {
	    printf("/dev/video1 supports streaming i/o\n");
	}
	if (tV4l2Cap.capabilities & V4L2_CAP_READWRITE) {
	    printf("/dev/video1 supports read i/o\n");
	}

	// 枚举设备支持的格式，找到第一个支持的格式并设置设备使用这个格式
	memset(&tFmtDesc, 0, sizeof(tFmtDesc));
	tFmtDesc.index = 0;
	tFmtDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                        
	while ((iError = ioctl(iFd, VIDIOC_ENUM_FMT, &tFmtDesc)) == 0) {
		if (isSupportThisFormat(tFmtDesc.pixelformat))                  
        {
            format = tFmtDesc.pixelformat;
            break;
        }
		tFmtDesc.index++;
	}
    if (!format)
    {
    	printf("can not support the format of this device\n");
        goto err_exit;        
    }

	// 设置格式，设置分辨率为800x600，像素格式为上面找到的支持的格式，字段类型为任意
    iLcdWidth = 800;
	iLcdHeigt = 600;
	iLcdBpp = 32;
    memset(&tV4l2Fmt, 0, sizeof(struct v4l2_format));
    tV4l2Fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    tV4l2Fmt.fmt.pix.pixelformat = format;
    tV4l2Fmt.fmt.pix.width       = iLcdWidth;
    tV4l2Fmt.fmt.pix.height      = iLcdHeigt;
    tV4l2Fmt.fmt.pix.field       = V4L2_FIELD_ANY;

	// 设置格式
	iError = ioctl(iFd, VIDIOC_S_FMT, &tV4l2Fmt); 
    if (iError) 
    {
    	printf("Unable to set format\n");
        goto err_exit;        
    }
    iLcdWidth = tV4l2Fmt.fmt.pix.width;
    iLcdHeigt = tV4l2Fmt.fmt.pix.height;

	// 申请缓冲区，使用mmap方式，申请buffcnt个缓冲区
    memset(&tV4l2ReqBuffs, 0, sizeof(struct v4l2_requestbuffers));
    tV4l2ReqBuffs.count = buffcnt;
    tV4l2ReqBuffs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    tV4l2ReqBuffs.memory = V4L2_MEMORY_MMAP;
    iError = ioctl(iFd, VIDIOC_REQBUFS, &tV4l2ReqBuffs);
    if (iError) 
    {
    	printf("Unable to allocate buffers.\n");
        goto err_exit;        
    }
    
	// 将申请到的缓冲区映射到用户空间，并保存映射地址
    for (i = 0; i < buffcnt; i++) 
    {
        memset(&tV4l2Buf, 0, sizeof(struct v4l2_buffer));
        tV4l2Buf.index = i;
        tV4l2Buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        tV4l2Buf.memory = V4L2_MEMORY_MMAP;
        iError = ioctl(iFd, VIDIOC_QUERYBUF, &tV4l2Buf);
        if (iError) 
        {
        	printf("Unable to query buffer.\n");
        	goto err_exit;
        }
        maxlen = tV4l2Buf.length;
        buffs[i] = mmap(0,tV4l2Buf.length, 
            PROT_READ, MAP_SHARED, iFd, tV4l2Buf.m.offset);
        if (buffs[i] == MAP_FAILED) 
        {
        	printf("Unable to map buffer\n");
        	goto err_exit;
        }
    }        

	// 将所有缓冲区放入队列，准备开始捕获
    for (i = 0; i <buffcnt; i++) 
    {
        memset(&tV4l2Buf, 0, sizeof(struct v4l2_buffer));
        tV4l2Buf.index = i;
        tV4l2Buf.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        tV4l2Buf.memory = V4L2_MEMORY_MMAP;
        iError = ioctl(iFd, VIDIOC_QBUF, &tV4l2Buf);
        if (iError)
        {
        	printf("Unable to queue buffer.\n");
        	goto err_exit;
        }
    }

	// 启动视频捕获线程
    V4l2StartDevice();

	// 创建视频读取线程，线程函数为V4l2_thread
    iError = pthread_create(&g_v4l2_tid, NULL, V4l2_thread, NULL);
    if (iError != 0)
    {
        printf("pthread_create failed, errno=%d\n", iError);
        goto err_exit;
    }
    return 0;
    
	// 错误处理：释放资源，关闭设备
    err_exit:
        for (i = 0; i < buffcnt; i++)
        {
            if (buffs[i])
            {
                munmap(buffs[i], maxlen);
                buffs[i] = NULL;
            }
        }
        if (iFd >= 0)
        {
            close(iFd);
            iFd = -1;
        }
        return -1;
}

// 停止视频捕获线程，停止视频流
int V4l2StopDevice()
{
    int iType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int iError;

    iError = ioctl(iFd, VIDIOC_STREAMOFF, &iType);
    if (iError)
    {
        printf("Unable to stop capture.\n");
        return -1;
    }
    g_v4l2_exit_flag = 1;
    return 0;
}

// 退出视频设备：停止线程，停止视频流，释放资源，关闭设备
int V4l2ExitDevice()
{
    g_v4l2_exit_flag = 1;
    pthread_join(g_v4l2_tid, NULL);

    V4l2StopDevice();

    int i;
    for (i = 0; i < buffcnt; i++)
    {
        if (buffs[i])
        {
            munmap(buffs[i], maxlen);
            buffs[i] = NULL;
        }
    }
    if (iFd >= 0)
    {
        close(iFd);
        iFd = -1;
    }
    return 0;
}
   
// 读取一帧视频数据：等待新帧，拷贝数据到调用者缓冲区，返回实际数据长度
int v4l2_read(char* buffer, int *buf_len)
{
    pthread_mutex_lock(&g_buff_mutex);                    //互斥锁保护共享变量buff
    if(g_has_new_frame == 0)                              //没有新帧，等待条件变量
    {
        pthread_cond_wait(&g_frame_cond, &g_buff_mutex);
	}
    memcpy(buffer, buff, curlen);                         //拷贝数据到调用者缓冲区
	g_has_new_frame = 0;                                  //重置新帧标志
	*buf_len = curlen;                                    //返回实际帧数据长度
    pthread_mutex_unlock(&g_buff_mutex);
    return curlen;                                        //返回实际拷贝的字节数
}

// Base64编码函数
int base64_encode(const unsigned char* in, int in_len, char* out, int out_len) {
    int i, j;
    unsigned char a3[3];
    unsigned char a4[4];

    if (in == NULL || out == NULL || out_len < (in_len * 4 + 2) / 3 + 1) {
        return -1; 
    }

    i = j = 0;
    while (in_len > 0) {
        a3[0] = in_len >= 1 ? in[i++] : 0;
        a3[1] = in_len >= 2 ? in[i++] : 0;
        a3[2] = in_len >= 3 ? in[i++] : 0;

        // 3字节转4字节
        a4[0] = (a3[0] & 0xfc) >> 2;
        a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
        a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
        a4[3] = a3[2] & 0x3f;

        // 根据输入长度补 =
        out[j++] = base64_table[a4[0]];
        out[j++] = base64_table[a4[1]];
        out[j++] = in_len > 1 ? base64_table[a4[2]] : '=';
        out[j++] = in_len > 2 ? base64_table[a4[3]] : '=';

        in_len -= 3;
    }
    out[j] = '\0'; // 字符串结束符
    return j;      // 返回编码后长度（不含结束符）
}







    




