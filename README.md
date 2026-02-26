# 智慧家庭安防中心

> **一个基于 i.MX6ULL 的嵌入式 Linux 智能家居控制系统**  
> 实现本地触摸屏与远程手机 APP 双重控制，支持 LED、温湿度传感器、USB 摄像头的数据读写，具备移动监测和画面记录功能。

---

## 📦 仓库结构
| 目录/文件 | 说明 |
|----------|------|
| LED_HumiTemp_Video/ | QT 触摸屏客户端源码（RPC Client） |
| build-LED_TempHumi_Video-100ask-Debug/ | QT 客户端编译输出目录 |
| drv/ | 开发板驱动（LED, DHT11 驱动, 开发板提供） |
| lib/ | 第三方库（jsonrpc-c, libev, paho.mqtt.c） |
| mqtt_device_wechat/ | MQTT 客户端源码（RPC client, 连接 OneNET 云平台） |
| rpc_server/ | RPC 服务端源码（RPC server） |
| wechat.cfg | MQTT 客户端配置文件示例 |
| README.md | 项目说明文档 |

---

## 🚀 项目概述

本项目是一个完整的嵌入式 Linux 物联网应用，涵盖**底层硬件控制**、**本地图形界面**、**云平台接入**三大模块。系统采用 **RPC（远程过程调用）** 架构，运行在 i.MX6ULL 开发板上，负责与 LED、DHT11 温湿度传感器、USB 摄像头交互；两个客户端（QT 触摸屏客户端和 MQTT 客户端）通过 socket 与服务端通信，实现本地与远程的双重控制。

**核心功能**：
- 实时显示温湿度数据（DHT11）
- 控制 LED 开关（状态同步到本地和手机）
- 查看 USB 摄像头实时画面（MJPEG 格式）
- 移动监测：基于帧差法检测画面变化，自动保存触发时的图像
- 告警记录查看：浏览所有保存的监测图像
- 远程控制：通过手机 APP（基于 OneNET 云平台）查看温湿度，控制LED和移动监测

**标签**：`嵌入式Linux` `i.MX6ULL` `C` `C++` `Qt` `RPC` `MQTT` `OneNET` `V4L2` `多线程` `Socket` `JSON`

---

## 🧩 模块详解

### 1. QT 触摸屏客户端（本地 UI 操作界面）
- **位置**：`LED_HumiTemp_Video/`
- **界面&功能**：
  - 主页面：实时显示温湿度、LED 开关、移动监测开关、查看监控、查看告警记录
  - 监控副页面：实时视频画面，返回、记录（保存当前帧）、移动监测开关、上下翻页浏览所有告警记录
- **工作流程**：
  1. 与RPC server建立tcp连接
  2. 同时通过 socket 与本地 RPC Server 通信
  3. 触摸屏 UI 下发指令 ↔ RPC Server → 硬件执行
- **线程与 socket**：
  - UI 主线程：执行 UI 界面的操作，如按钮、视频帧显示等
  - 温湿度数据读取线程：循环发送温湿度读取指令，将返回的数据显示在UI界面
  - 视频帧读取线程：循环发送视频读取指令，将收到的字符串进行base64解码后，在 UI 界面播放连续的视频帧
  -  socket 连接：温湿度数据读取、视频帧读取、LED及移动监测控制 三个socket

### 2. MQTT 客户端（云平台接入）
- **位置**：`mqtt_device_wechat/`
- **功能**：作为 RPC Client 的同时，也作为 MQTT 客户端连接中国移动 OneNET 云平台。
- **工作流程**：
  1. 读取配置文件 `wechat.cfg`（包含设备 ID 、用户名、密码、订阅/发布主题 等）
  2. 与RPC server建立tcp连接，与 OneNET 建立 MQTT 连接
  3. 同时通过 socket 与本地 RPC Server 通信
  4. 手机 APP 下发指令 ↔ OneNET ↔ MQTT 客户端 ↔ RPC Server → 硬件执行
  5. 硬件状态变化实时上报到云平台，同步到手机 APP
- **线程与 socket**：一个线程，一个 socket 连接（复用，传输温湿度/LED/移动监测状态及控制指令）

### 3. RPC 服务端（硬件控制核心）
- **位置**：`rpc_server/`
- **功能**：运行于开发板底层，负责与硬件交互，响应客户端的 RPC 请求。
- **线程与 socket**：
  - 主线程：监听新的客户端连接（socket），有连接到来时创建新线程处理该连接
  - 温湿度读取线程：独立线程，定期读取 DHT11 传感器数据，更新共享内存
  - 视频采集线程：独立线程，使用 V4L2 采集 USB 摄像头 MJPEG 帧，放入缓冲区
  - 客户端通信线程：客户端的每个连接对应一个线程，处理该客户端的 RPC 请求（读/写 LED、移动监测开关、获取温湿度、获取视频帧等）
  - socket连接：支持多个客户端同时连接（QT 客户端和 MQTT 客户端），为每个客户端的socket分别创建多个线程
- **技术实现**：
  - 数据交互：服务端与客户端之间的所有 RPC 请求/响应均采用 **cJSON** 格式封装。
  - LED 和 DHT11 温湿度传感器：由各自的驱动程序完成GPIO的读写功能，将数据发送到各客户端
  - 视频流：通过 V4L2 采集 MJPEG 格式，base64编码为字符串后，发送到 QT 客户端
  - 移动监测：帧差法，检测到运动时自动保存图片（保存到开发板存储）


### 4. 整体数据流
 - 用户（触摸屏）   <---------------> QT 客户端 <-----------------> RPC Server <---> 硬件（LED、DHT11、摄像头）
 - 用户（手机 APP） <--> OneNET 云平台 <--> MQTT 客户端 <---> RPC Server <---> 硬件（LED、DHT11、摄像头）

---

## 🔧 编译与运行

### 依赖库
项目依赖以下第三方库（已放置在 `lib/` 目录，或需自行编译）：
- **jsonrpc-c**：用于构建 RPC 消息，依赖于libev库
- **libev**：事件循环库
- **paho.mqtt.c**：MQTT 客户端 C 库

交叉编译时需确保工具链已安装。

### 编译步骤
1. **编译 RPC 服务端**
   ```bash
   cd rpc_server
   make
   
生成可执行文件 rpc_server

3. **编译 MQTT 服务端**
   ```bash
   cd mqtt_device_wechat
   make

生成可执行文件 mqtt_client

5. **编译 QT 服务端**
   使用 Qt Creator 打开 LED_HumiTemp_Video/LED_HumiTemp_Video.pro，选择合适 kit（交叉编译工具链）进行编译。编译输出目录示例为 build-LED_TempHumi_Video-100ask-Debug/，生成的可执行文件为 LED_HumiTemp_Video

### 运行步骤
-启动服务端`./rpc_server`
-启动两个客户端（不分先后）`./LED_HumiTemp_Video` `./mqtt_device_wechat`
-建议编写开机自启动脚本（如 /etc/init.d/S99myqt），内容示例：

---

## 📌 后续计划
增加更多传感器和外设（如烟雾、人体红外、GPS）
部署更智能的检测算法，增加目标识别等功能
优化视频帧采集的性能和传输效率
编写简单的驱动程序，对现有驱动进行替换

---

## 🤝 贡献与反馈
本项目为基于韦东山老师开源项目的个人改进学习作品，欢迎交流学习！
