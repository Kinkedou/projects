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

// 全局变量，保存RPC Client的socket描述符，供其他函数使用
static int g_iSocketClient;

// LED控制函数，发送jsonrpc请求，读取响应，解析响应，返回结果
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
		// 发送后读取jsonrpc响应，响应的格式和请求类似，包含result字段，表示方法的返回值
        while (1) 
        {
            iLen = read(iSocketClient, buf, sizeof(buf));
            buf[iLen] = 0;
            if (iLen == 1 && (buf[0] == '\r' || buf[0] == '\n'))
                continue;
            else
                break;
        } 
        
		// 解析jsonrpc响应，获取result字段的整数值，返回给调用者
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

// LED控制函数，发送jsonrpc请求，读取响应，解析响应，返回结果
int rpc_led_control(int on)
{
    char buf[100];
    unsigned int iLen;
    int ret = -1;
    int iSocketClient = g_iSocketClient;
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

// DHT11传感器读取函数，发送jsonrpc请求，读取响应，解析响应，返回结果
int rpc_dht11_read(char *humi, char *temp)
{
    char buf[300];
    unsigned int iLen;
    int iSocketClient=g_iSocketClient;
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

// RPC Client初始化函数，创建socket，连接RPC Server，返回socket描述符
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
	inet_aton("127.0.0.1", &tSocketServerAddr.sin_addr);
    //填充0，sin_zero是为了让sockaddr和sockaddr_in结构体大小相同
	memset(tSocketServerAddr.sin_zero, 0, 8);

	//连接RPC Server，成功返回0，失败返回-1
	iRet = connect(iSocketClient, (const struct sockaddr*)&tSocketServerAddr, sizeof(struct sockaddr));
    if (-1 == iRet)
    {
        printf("connect error!\n");
        return -1;
    }
    g_iSocketClient=iSocketClient;
	return iSocketClient;//返回socket描述符 
}


