#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "rpc.h"
#include "cJSON.h"

static int g_iSocketClient;

// base64解码表
static const unsigned char base64_decode_table[] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62,  255, 255, 255, 63,
    52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  255, 255, 255, 0,   255, 255,
    255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
    15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  255, 255, 255, 255, 255,
    255, 26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
    41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

// Base64解码函数
int base64_decode(const char *in, int in_len, unsigned char *out, int out_len) {
    int i, j;
    unsigned char a, b, c, d;
    unsigned int tmp;

    if (in == NULL || out == NULL || in_len <= 0 || out_len <= 0) {
        return -1;
    }

    // 跳过无效字符（仅保留Base64有效字符）
    char *filtered = (char *)malloc(in_len);
    int filtered_len = 0;
    for (i = 0; i < in_len; i++) {
        if (base64_decode_table[(unsigned char)in[i]] != 255 || in[i] == '=') {
            filtered[filtered_len++] = in[i];
        }
    }

    int ret = 0;
    for (i = 0; i < filtered_len; i += 4) {
        // 读取4个Base64字符（不足补0）
        a = (i < filtered_len) ? base64_decode_table[(unsigned char)filtered[i]] : 0;
        b = (i+1 < filtered_len) ? base64_decode_table[(unsigned char)filtered[i+1]] : 0;
        c = (i+2 < filtered_len) ? base64_decode_table[(unsigned char)filtered[i+2]] : 0;
        d = (i+3 < filtered_len) ? base64_decode_table[(unsigned char)filtered[i+3]] : 0;

        // 4个6位合并为3个8位
        tmp = (a << 18) | (b << 12) | (c << 6) | d;

        // 写入输出缓冲区（处理填充符=）
        j = i / 4 * 3;
        if (j >= out_len) {
            ret = -2; // 输出缓冲区不足
            break;
        }
        out[j] = (tmp >> 16) & 0xFF;
        ret++;

        if (filtered[i+2] != '=' && j+1 < out_len) {
            out[j+1] = (tmp >> 8) & 0xFF;
            ret++;
        }
        if (filtered[i+3] != '=' && j+2 < out_len) {
            out[j+2] = tmp & 0xFF;
            ret++;
        }
    }

    free(filtered);
    return ret;
}

// 读取视频的rpc请求
int rpc_video_read(unsigned char *video, int *socket)
{
    char buf[150000]={0};
    unsigned int iLen;
    int total_read=0;
    int iSocketClient=*socket;
    sprintf(buf, "{\"method\": \"video_read\"," \
                   "\"params\": [0], \"id\": \"4\" }");

    iLen = send(iSocketClient, buf, strlen(buf), 0);
    //发送成功，继续读取jsonrpc响应，响应的格式和请求类似，包含result字段，表示方法的返回值
    if (iLen == strlen(buf))
    {
        // 清空缓冲区，准备接收响应
        memset(buf, 0, sizeof(buf));
        total_read = 0;
        //循环读取直到获取完整的JSON响应
        while (1) {
            // 每次读取剩余的缓冲区空间，避免溢出
            iLen = read(iSocketClient, buf + total_read, sizeof(buf) - total_read - 1);
            total_read += iLen;
            buf[total_read] = '\0';

            // 跳过空行（换行/回车）
            if (total_read == 1 && (buf[0] == '\r' || buf[0] == '\n')) {
                total_read = 0;
                memset(buf, 0, sizeof(buf));
                continue;
            }

            // 判断JSON是否完整，以}结尾
            if (strchr(buf, '}') != NULL) {
                break;  // JSON完整，退出循环
            }
        }

        //对一帧字符串进行解码
        if (iLen > 0)
        {
            cJSON *root = cJSON_Parse(buf);
            cJSON *result = cJSON_GetObjectItem(root, "result");
            if (result)
            {
                const char* base64_str=result->valuestring;
                int base64_len = strlen(base64_str);
                int decode_len = base64_decode(base64_str, base64_len, video, 150000);
                if (decode_len <= 0) {
                    printf("base64 decode err: %d\n", decode_len);
                    cJSON_Delete(root);
                    return -1;
                }
                cJSON_Delete(root);
                return decode_len;
            }
            else
            {
                cJSON_Delete(root);
                return -1;
            }
        }
        else
        {
            printf("read rpc reply err : %d\n", iLen);
            return -1;
        }
    }
    else
    {
        printf("send rpc request err : %d, %s\n", iLen, strerror(errno));
        return -1;
    }
}

// 移动监测控制函数，发送jsonrpc请求，读取响应，解析响应，返回结果
int rpc_detect_control(int on)
{
    int iSocketClient=g_iSocketClient;
    char buf[100];
    unsigned int iLen;
    int ret = -1;
    sprintf(buf, "{\"method\": \"detect_control\", \"params\": [%d], \"id\": \"3\" }", on);
    iLen = send(iSocketClient, buf, strlen(buf), 0);
    if (iLen == strlen(buf))
    {
        while (1)
        {
            iLen = read(iSocketClient, buf, sizeof(buf));
            buf[iLen] = 0;
            if (iLen == 1 && (buf[0] == '\r' || buf[0] == '\n'))
                continue;
            else
                break;
        }
        if (iLen > 0)
        {
            cJSON* root = cJSON_Parse(buf);
            cJSON* result = cJSON_GetObjectItem(root, "result");
            ret = result->valueint;
            cJSON_Delete(root);
            return ret;
        }
        else
        {
            printf("read rpc reply err : %d\n", iLen);
            return -1;
        }
    }
    else
    {
        printf("send rpc request err : %d, %s\n", iLen, strerror(errno));
        return -1;
    }
}

//LED控制函数
int rpc_led_control(int on)
{
    char buf[100];
    unsigned int iLen;
    int ret = -1;
    int iSocketClient=g_iSocketClient;
    sprintf(buf, "{\"method\": \"led_control\", \"params\": [%d], \"id\": \"2\" }", on);
	iLen = send(iSocketClient, buf, strlen(buf), 0);
	if (iLen == strlen(buf))
    {
        while (1) 
        {
			iLen = read(iSocketClient, buf, sizeof(buf));
            buf[iLen] = 0;
			if (iLen == 1 && (buf[0] == '\r' || buf[0] == '\n'))
                continue;
            else
                break;
        } 
        if (iLen > 0)
        {
			cJSON* root = cJSON_Parse(buf);
			cJSON* result = cJSON_GetObjectItem(root, "result");
			ret = result->valueint;
			cJSON_Delete(root);
            return ret;
        }
        else
        {
            printf("read rpc reply err : %d\n", iLen);
            return -1;
        }
    }
    else
    {
        printf("send rpc request err : %d, %s\n", iLen, strerror(errno));
        return -1;
    }
}

//温湿度读取函数
int rpc_dht11_read(char *humi, char *temp,int* socket)
{
    char buf[300];
    unsigned int iLen;
    int iSocketClient=*socket;
    sprintf(buf, "{\"method\": \"dht11_read\"," \
                   "\"params\": [0], \"id\": \"5\" }");
            
    iLen = send(iSocketClient, buf, strlen(buf), 0);
	if (iLen == strlen(buf))
    {
        while (1) 
        {
            iLen = read(iSocketClient, buf, sizeof(buf));
            buf[iLen] = 0;
            if (iLen == 1 && (buf[0] == '\r' || buf[0] == '\n'))
                continue;
            else
                break;
        } 
        
        if (iLen > 0)
        {
            cJSON *root = cJSON_Parse(buf);
            cJSON *result = cJSON_GetObjectItem(root, "result");
            if (result)
            {
                cJSON * a = cJSON_GetArrayItem(result,0);
                cJSON * b = cJSON_GetArrayItem(result,1);

                *humi = a->valueint;
                *temp = b->valueint;
                
                cJSON_Delete(root);
                return 0;
            }
            else
            {
                cJSON_Delete(root);
                return -1;
            }
        }
        else
        {
            printf("read rpc reply err : %d\n", iLen);
            return -1;
        }
    }
    else
    {
        printf("send rpc request err : %d, %s\n", iLen, strerror(errno));
        return -1;
    }
}

//初始化rpc客户端
int RPC_Client_Init(void) 
{
    int iSocketClient;
    struct sockaddr_in tSocketServerAddr;
    int iRet;

    //创建socket，AF_INET:ipv4,SOCK_STREAM:tcp，0:默认协议
	iSocketClient = socket(AF_INET, SOCK_STREAM, 0);

	//设置RPC Server地址，AF_INET:ipv4, htons(PORT):端口号，inet_aton:将点分十进制IP地址转换为网络字节序的二进制形式
    tSocketServerAddr.sin_family      = AF_INET;
    tSocketServerAddr.sin_port        = htons(PORT);
	inet_aton("127.0.0.1", &tSocketServerAddr.sin_addr);//本地回环地址
	memset(tSocketServerAddr.sin_zero, 0, 8);//填充0，sin_zero是为了让sockaddr和sockaddr_in结构体大小相同，实际没有使用

    //连接RPC Server
	iRet = connect(iSocketClient, (const struct sockaddr*)&tSocketServerAddr, sizeof(struct sockaddr));
    if (-1 == iRet)
    {
        printf("connect error!\n");
        return -1;
    }
    if(!g_iSocketClient)
    g_iSocketClient=iSocketClient;
	return iSocketClient; 
}

