#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "cfg.h"

/* 配置文件的内容格式:
 *
 * { "URI":"NBL71OPZ8I.iotcloud.tencentdevices.com",
 *       "clientid" : "NBL71OPZ8Ihome_control_01",
 *		 "username" : "NBL71OPZ8Ihome_control_01;12010126;WB2UN;1726024103",
 *		 "password" : "023215a9420a3d983d73a2cb4c0f7d13bdbcb7a9d97c3963a36f79ea76cfc172;hmacsha256"
 *       "ProductID": "NBL71OPZ8I",
 *       "DeviceName": "home_control_01"
 * }
 * 
 */
int read_cfg(char *URI, char *clientid, char *username, char *password, char *ProductID, char *DeviceName, char* pub_topic, char* sub_topic)
{
	char buf[1000];
	cJSON *ptTemp;
	
	int fd = open(CFG_FILE, O_RDONLY);
	
	if (fd < 0)
		return -1;

	int ret = read(fd, buf, sizeof(buf));
	if (ret <= 0)
		return -1;

	cJSON *root = cJSON_Parse(buf);

	ptTemp = cJSON_GetObjectItem(root, "URI");
	if (ptTemp)
	{
		strcpy(URI, ptTemp->valuestring);
	}

	ptTemp = cJSON_GetObjectItem(root, "clientid");
	if (ptTemp)
	{
		strcpy(clientid, ptTemp->valuestring);
	}


	ptTemp = cJSON_GetObjectItem(root, "username");
	if (ptTemp)
	{
		strcpy(username, ptTemp->valuestring);
	}

	ptTemp = cJSON_GetObjectItem(root, "password");
	if (ptTemp)
	{
		strcpy(password, ptTemp->valuestring);
	}

	ptTemp = cJSON_GetObjectItem(root, "ProductID");
	if (ptTemp)
	{
		strcpy(ProductID, ptTemp->valuestring);
	}

	ptTemp = cJSON_GetObjectItem(root, "DeviceName");
	if (ptTemp)
	{
		strcpy(DeviceName, ptTemp->valuestring);
	}

	ptTemp = cJSON_GetObjectItem(root, "pub_topic");
	if (ptTemp)
	{
		strcpy(pub_topic, ptTemp->valuestring);
	}

	ptTemp = cJSON_GetObjectItem(root, "sub_topic");
	if (ptTemp)
	{
		strcpy(sub_topic, ptTemp->valuestring);
	}

	cJSON_Delete(root);
	return 0;
}



