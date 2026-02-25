TARGET=mqtt_device_wechat
CC=arm-buildroot-linux-gnueabihf-gcc

TOP_DIR=$(shell pwd)/../
PAHO_MQTT_DIR=${TOP_DIR}/paho.mqtt.c/

CFLAGS=-I${PAHO_MQTT_DIR}/build/src -O
LDFLAGS=-L${PAHO_MQTT_DIR}/build/src -lpaho-mqtt3c

c_files = mqtt_device_wechat.c cfg.c cJSON.c rpc_client.c

all:
	${CC} ${CFLAGS} -o ${TARGET} ${c_files} ${LDFLAGS}

clean:
	rm -f *.o ${TARGET}
	
