#ifndef DHT11_H
#define DHT11_H

void dht11_init(void);
int dht11_read(char *humi, char *temp);

#endif // DHT11_H
