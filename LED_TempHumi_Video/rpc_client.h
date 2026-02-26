#ifndef RPC_CLIENT_H
#define RPC_CLIENT_H

int rpc_detect_control(int on);
int rpc_led_control(int on);
int rpc_dht11_read(char *humi, char *temp,int* socket);
int RPC_Client_Init(void);
int rpc_video_read(unsigned char *video,int* socket);

#endif




