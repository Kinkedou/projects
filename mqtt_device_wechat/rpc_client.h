#ifndef RPC_CLIENT_H
#define RPC_CLIENT_H

int rpc_detect_control(int on);
int rpc_led_control(int on);
int rpc_dht11_read(char *humi, char *temp);
int RPC_Client_Init(void);
static void print_usage(char *exec);

#endif





