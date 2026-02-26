#include <jsonrpc-c.h>
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
#include <pthread.h>
#include <time.h>
#include "rpc.h"
#include "led.h"
#include "dht11.h"
#include "v4l2.h"
#include "detect.h"

#define MAX_FRAME_LEN 150000    // 最大帧数据长度（根据实际分辨率调整）
#define MAX_CLIENTS 10          // 最大RPC客户端数
#define BUF_SIZE 500            // RPC请求接收缓冲区

static pthread_t detect_thread;                                  // 移动检测线程ID
static int g_detect_running = 0;                                 // 线程运行标志
static pthread_mutex_t detect_mutex = PTHREAD_MUTEX_INITIALIZER; // 状态互斥锁

static int g_led_status = 0;    // LED状态，0表示关闭，1表示打开
static int g_detect_status = 0; // 移动检测状态，0表示关闭，1表示打开
int g_shot = 0;                 // 连续检测到移动的帧数计数器（达到3时触发捕获）

static struct jrpc_server my_server; // JSON-RPC服务器实例
static pthread_t rpc_server_tid;     // RPC服务线程ID

// 移动检测线程函数：循环读取视频帧，解码为灰度图，执行移动检测，捕获并保存图片
void* detect_thread_func(void* arg) {
    static uint8_t prev_gray_buf[MAX_WIDTH * MAX_HEIGHT] = { 0 };
    uint8_t curr_gray_buf[MAX_WIDTH * MAX_HEIGHT] = { 0 };
    int frame_width = 800, frame_height = 600;
    int is_first_frame = 1;

    // 循环处理视频帧
    while (g_detect_running) {

		// 检测停止信号
        pthread_mutex_lock(&detect_mutex);
        if (!g_detect_running) {
            pthread_mutex_unlock(&detect_mutex);
			printf("检测停止信号已接收，准备退出线程...\n");
            break;
        }
        pthread_mutex_unlock(&detect_mutex);

        // 获取帧数据
        int curlen = 0;
        unsigned char frame_buf[MAX_FRAME_LEN] = { 0 };
        int read_ret=v4l2_read((char*)frame_buf, &curlen);
        if (read_ret < 0 || curlen <= 0 || curlen > MAX_FRAME_LEN) {
            printf("v4l2_read失败，返回值：%d，长度：%d\n", read_ret, curlen);
            usleep(33000);
            continue;
        }

		// 解码MJPEG为灰度图
        int decode_ret = decodeMJPEGToGray(frame_buf, curlen, curr_gray_buf, frame_width, frame_height);
        if (decode_ret != 0) {
            printf( "当前帧解码失败，跳过\n");
            usleep(33000);
            continue;
        }

		// 首帧特殊处理：仅缓存，不执行检测
        if (is_first_frame) {
            memcpy(prev_gray_buf, curr_gray_buf, frame_width * frame_height);
            is_first_frame = 0;
            usleep(33000);
            continue;
        }

        // 执行移动检测
        int motion_ret = detectMotion(curr_gray_buf, prev_gray_buf, frame_width, frame_height);
        if (motion_ret == 1) {
            g_shot++;
			if (g_shot == 3)            // 连续3帧检测到移动，触发捕获
            {
                printf("检测到移动！\n");
                char filename[64];      //按warning+日期生成文件名
                char timeStr[64];       // 存储格式化后的时间字符串
                time_t now = time(NULL);
                struct tm* tmInfo = localtime(&now);
                strftime(timeStr, sizeof(timeStr), "%Y%m%d%H%M%S", tmInfo);
				snprintf(filename, sizeof(filename), "/root/warning_%s.jpg", timeStr); // 生成文件名，格式为warning_年月日时分秒.jpg
                int fd_file = open(filename, O_RDWR | O_CREAT, 0666);                  //0666是八进制权限，可读可写
                if (fd_file < 0)
                {
                    printf("open %s failed\n", filename);
                }
                ssize_t write_len = write(fd_file, frame_buf, curlen);
                if (write_len != curlen) {
                    printf("写入文件 %s 失败（实际写入%zd字节，预期%d字节）\n", filename, write_len, curlen);
                }
                printf("capture to %s\n", filename);
                close(fd_file);
                g_shot = 0;
                usleep(300000);
            }
        }
        else {
            g_shot = 0; // 没有检测到移动，重置计数器
		}

        // 更新前一帧缓存
        memcpy(prev_gray_buf, curr_gray_buf, frame_width * frame_height);

        usleep(33000); // 近似30帧/秒
    }

    pthread_exit(NULL);
    return NULL;
}

// 处理LED控制的RPC方法：根据参数控制LED开关，或查询当前状态
cJSON * server_led_control(jrpc_context * ctx, cJSON * params, cJSON *id) {
	cJSON* status = cJSON_GetArrayItem(params, 0);
	int led_status = status->valueint;
	if (led_status == 1 || led_status == 0) // 1表示开，0表示关
    {
        led_control(led_status);
		g_led_status = led_status;
    }	
	else if (led_status == 3)               // 查询状态
    {
        return cJSON_CreateNumber(g_led_status);
    }
    return cJSON_CreateNumber(0);
}

// 处理移动检测控制的RPC方法：根据参数启动/停止检测，或查询当前状态
cJSON* server_detect_control(jrpc_context* ctx, cJSON* params, cJSON* id) {
	cJSON* status = cJSON_GetArrayItem(params, 0);
    int detect_status = status->valueint;

    // 加锁修改运行状态
    pthread_mutex_lock(&detect_mutex);
    if (detect_status == 1 && !g_detect_running) {  //1表示开启移动检测
        g_detect_running = 1;
		int ret = pthread_create(&detect_thread, NULL, detect_thread_func, NULL); // 创建移动检测线程
        if (ret != 0) {
            g_detect_running = 0;   // 创建失败回滚状态
            pthread_mutex_unlock(&detect_mutex);
            ctx->error_code = -2;
            ctx->error_message = "Failed to create detect thread";
            return cJSON_CreateNumber(-2);
        }
		pthread_mutex_unlock(&detect_mutex);
        printf("移动检测已启动\n");
		g_detect_status = detect_status; //更新状态为1，表示正在检测
    }
    else if (detect_status == 0 && g_detect_running) {  // 0表示停止检测：清除标志 + 等待线程退出

        g_detect_running = 0;
        pthread_mutex_unlock(&detect_mutex);
        pthread_join(detect_thread, NULL);
        printf("移动检测已停止\n");
		g_detect_status = detect_status; //更新状态为0，表示已停止
    }
    else if (detect_status == 3){   // 3表示查询状态
        pthread_mutex_unlock(&detect_mutex);
        return cJSON_CreateNumber(g_detect_status);
	}
    else {  // 重复操作（如已启动又请求启动）
        pthread_mutex_unlock(&detect_mutex);
        printf("移动检测状态无变化：当前状态=%d，请求状态=%d\n", g_detect_running, detect_status);
    }
    return cJSON_CreateNumber(0);
}

// 处理DHT11读取的RPC方法：调用dht11_read获取温湿度数据，返回一个包含湿度和温度的整数数组
cJSON * server_dht11_read(jrpc_context * ctx, cJSON * params, cJSON *id) {
    int array[2];
    array[0] = array[1] = 0;
    dht11_read((char *)&array[0], (char *)&array[1]);
    return cJSON_CreateIntArray(array, 2);
}

// 处理视频读取的RPC方法：调用v4l2_read获取帧数据，Base64编码后返回字符串
cJSON* server_video_read(jrpc_context* ctx, cJSON* params, cJSON* id) {
	int curlen=0;
    unsigned char frame_buf[MAX_FRAME_LEN] = { 0 };
    char base64_buf[MAX_FRAME_LEN] = { 0 };  // base64编码后长度

	//读取一帧视频数据到frame_buf，curlen返回实际数据长度
	v4l2_read((char*)frame_buf, &curlen);    

    // Base64编码
    int encode_len = base64_encode(frame_buf, curlen, base64_buf, MAX_FRAME_LEN);
    if (encode_len < 0) {
        printf("Base64 encode failed\n");
        return cJSON_CreateNull();
    }

    // 返回Base64编码后的字符串
    return cJSON_CreateString(base64_buf);
}

// RPC请求处理函数：解析JSON-RPC请求，调用对应的业务函数，构造并发送响应
static void handle_rpc_request(int client_fd, char* req_buf) {
    cJSON* root = cJSON_Parse(req_buf);
    if (!root) {
		cJSON* resp = cJSON_CreateObject();
        cJSON_AddItemToObject(resp, "jsonrpc", cJSON_CreateString("2.0"));
        cJSON_AddItemToObject(resp, "error", cJSON_CreateObject());
        cJSON_AddItemToObject(cJSON_GetObjectItem(resp, "error"), "code", cJSON_CreateNumber(JRPC_PARSE_ERROR));
        cJSON_AddItemToObject(cJSON_GetObjectItem(resp, "error"), "message", cJSON_CreateString("Parse error"));
        cJSON_AddItemToObject(resp, "id", cJSON_CreateNull());

        char* resp_str = cJSON_Print(resp);
        write(client_fd, resp_str, strlen(resp_str));
        free(resp_str);
        cJSON_Delete(resp);
        return;
    }

    // 提取method、params、id
    cJSON* method = cJSON_GetObjectItem(root, "method");
    cJSON* params = cJSON_GetObjectItem(root, "params");
    cJSON* id = cJSON_GetObjectItem(root, "id");
    if (!method || !cJSON_IsString(method)) {
        cJSON* resp = cJSON_CreateObject();
        cJSON_AddItemToObject(resp, "jsonrpc", cJSON_CreateString("2.0"));
        cJSON_AddItemToObject(resp, "error", cJSON_CreateObject());
        cJSON_AddItemToObject(cJSON_GetObjectItem(resp, "error"), "code", cJSON_CreateNumber(JRPC_INVALID_REQUEST));
        cJSON_AddItemToObject(cJSON_GetObjectItem(resp, "error"), "message", cJSON_CreateString("Invalid Request"));
        cJSON_AddItemToObject(resp, "id", id ? cJSON_Duplicate(id, 1) : cJSON_CreateNull());

        char* resp_str = cJSON_Print(resp);
        write(client_fd, resp_str, strlen(resp_str));
        free(resp_str);
        cJSON_Delete(resp);
        cJSON_Delete(root);
        return;
    }

    // 匹配注册的RPC方法
    jrpc_function func = NULL;
    for (int i = 0; i < my_server.procedure_count; i++) {
		if (strcmp(my_server.procedures[i].name, method->valuestring) == 0) { // 找到匹配的方法，获取对应的函数指针
            func = my_server.procedures[i].function;
            break;
        }
    }
    if (!func) {
        cJSON* resp = cJSON_CreateObject();
        cJSON_AddItemToObject(resp, "jsonrpc", cJSON_CreateString("2.0"));
        cJSON_AddItemToObject(resp, "error", cJSON_CreateObject());
        cJSON_AddItemToObject(cJSON_GetObjectItem(resp, "error"), "code", cJSON_CreateNumber(JRPC_METHOD_NOT_FOUND));
        cJSON_AddItemToObject(cJSON_GetObjectItem(resp, "error"), "message", cJSON_CreateString("Method not found"));
        cJSON_AddItemToObject(resp, "id", id ? cJSON_Duplicate(id, 1) : cJSON_CreateNull());
        char* resp_str = cJSON_Print(resp);
        write(client_fd, resp_str, strlen(resp_str));
        free(resp_str);
        cJSON_Delete(resp);
        cJSON_Delete(root);
        return;
    }

	// 调用业务函数，传入上下文、参数和ID，返回结果对象
    jrpc_context ctx = { 0 };                                           // 初始化上下文
	cJSON* result = func(&ctx, params, id);                             // 调用RPC方法，传入上下文、参数和ID，获取结果
	cJSON* resp = cJSON_CreateObject();                                 // 构造响应对象
	cJSON_AddItemToObject(resp, "jsonrpc", cJSON_CreateString("2.0"));  // 添加jsonrpc版本字段
	if (ctx.error_code != 0) {                                          // 如果业务函数设置了错误码，返回错误响应
        cJSON_AddItemToObject(resp, "error", cJSON_CreateObject());
        cJSON_AddItemToObject(cJSON_GetObjectItem(resp, "error"), "code", cJSON_CreateNumber(ctx.error_code));
        cJSON_AddItemToObject(cJSON_GetObjectItem(resp, "error"), "message", cJSON_CreateString(ctx.error_message ? : "Internal error"));
    }
	else 
		cJSON_AddItemToObject(resp, "result", result);                                   // 添加结果字段，包含业务函数返回的结果对象
	cJSON_AddItemToObject(resp, "id", id ? cJSON_Duplicate(id, 1) : cJSON_CreateNull()); // 添加ID字段，保持与请求一致
	char* resp_str = cJSON_Print(resp);                                                  // 将响应对象转换为字符串
	write(client_fd, resp_str, strlen(resp_str));                                        // 将响应字符串发送给客户端

    // 释放资源
    free(resp_str);
    cJSON_Delete(resp);
    cJSON_Delete(root);
}

// 客户端处理线程函数：循环读取客户端请求，调用handle_rpc_request处理，直到客户端断开连接
static void* rpc_client_worker(void* arg) {
    int client_fd = *(int*)arg;
    free(arg);
    char buf[BUF_SIZE] = { 0 };
    ssize_t n;

	// 循环读取客户端请求，每次读取一个JSON-RPC请求，调用handle_rpc_request处理，直到客户端断开连接（read返回0或负值）
    while ((n = read(client_fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
		handle_rpc_request(client_fd, buf);
        memset(buf, 0, sizeof(buf));
    }
    if (n == 0) {
        printf("[RPC] Client fd:%d disconnected normally (read return 0)\n", client_fd);
    }
    else if (n < 0) {
        printf("[RPC] Client fd:%d disconnected abnormally: %s (errno:%d)\n",
            client_fd, strerror(errno), errno);
    }
    close(client_fd);
    printf("[RPC] Client disconnected, fd: %d\n", client_fd);
    pthread_exit(NULL);
}

// RPC服务器初始化函数：初始化JSON-RPC服务器，注册方法，监听客户端连接，创建线程处理请求
int RPC_Server_Init(void) {
    int err, listen_fd;
    pthread_t rpc_tid;
    int client_count = 0;
    
	// 初始化JSON-RPC服务器，绑定到指定端口
	err = jrpc_server_init(&my_server, PORT);
    if (err)
    {
        printf("jrpc_server_init err : %d\n", err);
        return -1;
    }
    
	// 注册RPC方法，将方法名与对应的处理函数关联起来，供客户端调用
	jrpc_register_procedure(&my_server, server_led_control, "led_control", NULL);
	jrpc_register_procedure(&my_server, server_dht11_read, "dht11_read", NULL);
    jrpc_register_procedure(&my_server, server_detect_control, "detect_control", NULL);
    jrpc_register_procedure(&my_server, server_video_read, "video_read", NULL);
	
    //jrpc_server_run(&my_server);      //运行服务器，jrpc_server_run在jsonrpc-c库中定义
	//jrpc_server_destroy(&my_server);  //销毁服务器，jrpc_server_destroy在jsonrpc-c库中定义


	// 获取监听socket的文件描述符，准备接受客户端连接
    listen_fd = my_server.listen_watcher.fd;
    if (listen_fd < 0) {
        printf("[RPC] Get listen fd from server failed\n");
        jrpc_server_destroy(&my_server);
        return -1;
    }
    printf("[RPC] Reuse listen fd: %d, port: %d\n", listen_fd, PORT);

    // 循环接受客户端连接
    while (1) {
        int* client_fd = malloc(sizeof(int));       
		*client_fd = accept(listen_fd, NULL, NULL);  // 接受客户端连接，返回新的客户端socket文件描述符
        if (*client_fd < 0) {
            printf("[RPC] Accept failed: %s\n", strerror(errno));
            free(client_fd);
            continue;
        }
		if (client_count >= MAX_CLIENTS) {  // 超过最大客户端数，拒绝连接
            printf("[RPC] Reject client, max connections (%d) reached\n", MAX_CLIENTS);
            close(*client_fd);
            free(client_fd);
            continue;
        }
        printf("[RPC] New client connected, fd: %d\n", *client_fd);
        client_count++;

		// 创建客户端线程，每个客户端一个线程，rpc_client_worker是处理该客户端请求的函数，传入client_fd作为参数
        if (pthread_create(&rpc_tid, NULL, rpc_client_worker, client_fd) != 0) {
            printf("[RPC] Create client thread failed\n");
            close(*client_fd);
            free(client_fd);
            client_count--;
            continue;
        }

		pthread_detach(rpc_tid);// 分离线程，自动回收资源，无需pthread_join
    }

    close(listen_fd);
    jrpc_server_destroy(&my_server);
    return 0;
}

// 主函数：初始化LED、DHT11、视频设备，启动RPC服务器
int main(int argc, char **argv)
{
    led_init();
    dht11_init();
    v4l2_init();
    RPC_Server_Init();   
    return 0;
}


