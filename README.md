# 智慧家庭安防中心

> **一个基于 i.MX6ULL 的嵌入式 Linux 智能家居控制系统**  
> 实现本地触摸屏与远程手机 APP 双重控制，支持 LED、温湿度传感器、USB 摄像头的数据读写，具备移动监测和画面记录功能。

---

## 📦 仓库结构
.
├── drv/                    # 开发板驱动（LED GPIO ，DHT11 驱动，开发板提供）
├── LED_HumiTemp_Video/     # QT 触摸屏客户端源码（RPC Client）
├── mqtt_device_wechat/     # MQTT 客户端源码（RPC client，连接 OneNET 云平台）
├── rpc_server/             # RPC 服务端源码（RPC server）
├── lib/                    # 第三方库（jsonrpc-c, libev, paho.mqtt.c）
├── wechat.cfg              # MQTT 客户端配置文件示例
├── build-LED_TempHumi_Video-100ask-Debug/ # QT 客户端编译输出目录
└── README.md
