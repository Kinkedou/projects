#ifndef _CFG_H
#define _CFG_H

#define CFG_FILE "/etc/wechat.cfg"

int read_cfg(char *URI, char *clientid, char *username, char *password, char *ProductID, char *DeviceName, char* pub_topic, char* sub_topic);

#endif /* _CFG_H */
