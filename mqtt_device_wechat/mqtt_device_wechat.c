#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "MQTTClient.h"
#include "cJSON.h"
#include "rpc_client.h"
#include "cfg.h"

// MQTT连接参数，QOS表示服务质量级别，TIMEOUT表示等待消息发布完成的超时时间
#define QOS         1
#define TIMEOUT     10000L

volatile MQTTClient_deliveryToken deliveredtoken;

//消息发送完成的回调函数，参数dt是一个标识符，用于确认消息的发送状态
void delivered(void* context, MQTTClient_deliveryToken dt)
{
    deliveredtoken = dt;
}

//消息到达的回调函数
// 参数context是用户定义的上下文信息，topicName是消息所属的主题名称，topicLen是主题名称的长度
// message是一个指向MQTTClient_message结构体的指针，包含了消息的内容和相关信息
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %.*s\n", message->payloadlen, (char*)message->payload);

	//解析消息内容，并根据消息中的参数调用相应的RPC方法
	cJSON *root = cJSON_Parse((char*)message->payload);
	cJSON *ptTemp = cJSON_GetObjectItem(root, "params");
	if (ptTemp)
	{

		cJSON *button = cJSON_GetObjectItem(ptTemp, "LED");
		if (button)
		{
			rpc_led_control(button->valueint);
		}
        cJSON* button1 = cJSON_GetObjectItem(ptTemp, "video");
        if (button1)
        {
			printf("video control: %d\n", button1->valueint);
            rpc_detect_control(button1->valueint);
        }
	}
	cJSON_Delete(root);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

//连接丢失的回调函数，当MQTT客户端与服务器的连接丢失时被调用，参数cause提供了连接丢失的原因
void connlost(void* context, char* cause)
{
    printf("\nConnection lost\n");
    if (cause)
    	printf("     cause: %s\n", cause);
}

//主函数，程序的入口点，负责初始化MQTT客户端，连接服务器，订阅主题，并进入消息处理循环
int main(int argc, char* argv[])
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

	char URI[1000];
	char clientid[1000]; 
	char username[1000]; 
	char password[1000]; 
	char ProductID[1000]; 
	char DeviceName[1000];
	char address[1000];
	char pub_topic[1000];
	char sub_topic[1000];

    // 读配置文件 
	if ( 0 != read_cfg(URI, clientid, username, password, ProductID, DeviceName, pub_topic,sub_topic))
	{
		printf("read cfg err\n");
		return -1;
	}
	sprintf(address, "tcp://%s:1883", URI);
	//printf("URI %s\n", URI);
    //printf("clientid %s\n", clientid);
    //printf("username %s\n", username);
    //printf("password %s\n", password);
	//printf("ProductID %s\n", ProductID);
	//printf("DeviceName %s\n", DeviceName);
	//printf("pub_topic %s\n", pub_topic);
	//printf("sub_topic %s\n", sub_topic);
	//printf("address %s\n", address);


	// 初始化RPC客户端，建立与RPC服务器的连接，以便后续调用RPC方法获取设备状态和控制设备 
    if (-1 == RPC_Client_Init())
	{
		printf("RPC_Client_Init err\n");
		return -1;
	}
	
	// 创建MQTT客户端，指定连接地址、客户端ID、持久化方式等参数
    if ((rc = MQTTClient_create(&client, address, clientid,
		MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to create client, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto exit;
    }

	// 设置MQTT Client的回调函数，包括连接丢失、消息到达和消息发送完成等事件的处理函数
	if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to set callbacks, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto destroy_exit;
    }

	// 设置MQTT连接选项，包括保持连接的时间间隔、是否清除会话等参数
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = username;
    conn_opts.password = password;

	// 连接MQTT服务器，使用之前设置的连接选项，如果连接失败，打印错误信息并退出程序
    while (1)
    {
		if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
	    {
		printf("Failed to connect, return code %d\n", rc);
		rc = EXIT_FAILURE;
	    }
        else
        {
            break;
        }
    }

	// 订阅MQTT主题，指定主题名称和QoS级别，如果订阅失败，打印错误信息并退出程序
	if ((rc = MQTTClient_subscribe(client, sub_topic, QOS)) != MQTTCLIENT_SUCCESS)
    {
    	printf("Failed to subscribe, return code %d\n", rc);
    	rc = EXIT_FAILURE;
    }
    else
    {
        int cnt = 0;
        char buf_dht[1000];
        char buf_video[1000];
        char buf_led[1000];
        MQTTClient_deliveryToken token;

        MQTTClient_message pubmsg_dht = MQTTClient_message_initializer;
        MQTTClient_message pubmsg_led = MQTTClient_message_initializer;
        MQTTClient_message pubmsg_video = MQTTClient_message_initializer;

    	while (1)
    	{
			// 调用RPC方法获取DHT11传感器的温湿度数据，并将数据封装成JSON格式的字符串，发布到MQTT服务器上
			char humi, temp;
			
			while (0 !=rpc_dht11_read(&humi, &temp));

            sprintf(buf_dht, "\
				{\
                \"id\": \"123\",\
                \"version\" : \"1.0\",\
                \"params\" : {\
                    \"humi_value\": {\
                        \"value\": %d\
                    },\
                    \"temp_value\" : {\
                        \"value\": %d\
                    }\
                }\
            }", temp, humi);
			//设置要发布的消息内容，长度，QoS级别和是否保留等参数
            pubmsg_dht.payload = buf_dht;
            pubmsg_dht.payloadlen = (int)strlen(buf_dht);
            pubmsg_dht.qos = QOS;
            pubmsg_dht.retained = 0;
			// 发布消息
            if ((rc = MQTTClient_publishMessage(client, pub_topic, &pubmsg_dht, &token)) != MQTTCLIENT_SUCCESS)
            {
                 printf("Failed to publish message, return code %d\n", rc);
                 continue;
            }
            //等待消息发布完成，指定消息的交付令牌和超时时间
			rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);   
			//休眠一段时间，避免过于频繁地发布消息，给MQTT服务器和网络带来压力
            sleep(0.3);
            
			//调用RPC方法获取LED和视频检测的状态，并将状态封装成JSON格式的字符串，发布到MQTT服务器上
            char* status_val;
            if (rpc_led_control(3))
                status_val = "true";
            else
                status_val = "false";
            sprintf(buf_led, "\
				{\
                \"id\": \"123\",\
                \"version\" : \"1.0\",\
                \"params\" : {\
                    \"LED\" : {\
                        \"value\": %s\
                    }\
                }\
            }", status_val);
            pubmsg_led.payload = buf_led;
            pubmsg_led.payloadlen = (int)strlen(buf_led);
            pubmsg_led.qos = QOS;
            pubmsg_led.retained = 0;
            if ((rc = MQTTClient_publishMessage(client, pub_topic, &pubmsg_led, &token)) != MQTTCLIENT_SUCCESS)
            {
                printf("Failed to publish message, return code %d\n", rc);
                continue;
            }
            rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);//等待消息发布完成，指定消息的交付令牌和超时时间
            sleep(0.3);

			//调用RPC方法获取视频检测的状态，并将状态封装成JSON格式的字符串，发布到MQTT服务器上
            if (rpc_detect_control(3))
                status_val = "true";
            else
                status_val = "false";
            sprintf(buf_video, "\
				{\
                \"id\": \"123\",\
                \"version\" : \"1.0\",\
                \"params\" : {\
                    \"video\" : {\
                        \"value\": %s\
                    }\
                }\
            }", status_val);
            pubmsg_video.payload = buf_video;
            pubmsg_video.payloadlen = (int)strlen(buf_video);
            pubmsg_video.qos = QOS;
            pubmsg_video.retained = 0;
            if ((rc = MQTTClient_publishMessage(client, pub_topic, &pubmsg_video, &token)) != MQTTCLIENT_SUCCESS)
            {
                printf("Failed to publish message, return code %d\n", rc);
                continue;
            }
            rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);//等待消息发布完成，指定消息的交付令牌和超时时间
            sleep(0.3);
    	} 

		//取消订阅MQTT主题，指定要取消订阅的主题名称，如果取消订阅失败，打印错误信息
		if ((rc = MQTTClient_unsubscribe(client, sub_topic)) != MQTTCLIENT_SUCCESS){
        	printf("Failed to unsubscribe, return code %d\n", rc);
        	rc = EXIT_FAILURE;
        }
    }

	//断开与MQTT服务器的连接，指定断开连接的超时时间，如果断开连接失败，打印错误信息
	if ((rc = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS)
    {
    	printf("Failed to disconnect, return code %d\n", rc);
    	rc = EXIT_FAILURE;
    }
destroy_exit:
    MQTTClient_destroy(&client);
exit:
    return rc;
}

