# ESP32S3 管理系统开发日志

## 阶段0-2：核心服务、Web与动态仪表盘

### 设计说明与开发进度

本阶段主要目标是搭建ESP32S3管理系统的基础骨架，实现Web服务器和前端资源流水线，并最终实现一个功能性的动态仪表盘，能够按需获取并展示准确的系统状态。

**已完成工作：**

1.  **硬件与环境验证**
    *   确认ESP32-S3单片机 (esp32s3r8n8)双核、8MB Octal PSRAM、8MB Quad Flash硬件配置正常。
    *   验证PlatformIO环境配置为espressif32 v6.11.0，内置Arduino Core v3.2.0以上版本。
    *   自定义板型 `esp32s3_lckfb_N8R8` (继承 `esp32-s3-devkitc-1`) 和自定义分区表 `my_8MB.csv` 已验证正常工作。
    *   通过 `main.cpp` 中的诊断函数，验证了CPU信息、Flash/PSRAM大小及模式、堆内存分配情况，以及分区表与 `my_8MB.csv` 的一致性。

2.  **内存管理模块 (`Sys_MemoryManager`)**
    *   实现了 `Sys_MemoryManager` 单例类，用于高效管理PSRAM大块内存。
    *   在 `setup()` 函数中初始化了内存管理器，并预分配了4个1MB的PSRAM内存块，通过简单的分配和释放测试验证了其功能。

3.  **文件系统模块 (`Sys_Filesystem`)**
    *   实现了 `Sys_Filesystem` 单例类，用于统一管理LittleFS和FFat文件系统。
    *   解决了文件系统挂载点问题，LittleFS挂载到 `/spiffs`，FFat挂载到 `/ffat`。
    *   配置 `platformio.ini` 强制在上传时格式化文件系统分区，确保文件系统干净。
    *   验证了文件系统的初始化和挂载成功。

4.  **Web服务器模块 (`Sys_WebServer`)**
    *   实现了 `Sys_WebServer` 单例类，封装了 `ESPAsyncWebServer` 的初始化与事件处理。
    *   在 `setup()` 函数中集成了Wi-Fi连接逻辑，并成功连接到用户提供的Wi-Fi网络（IP地址：192.168.1.103）。
    *   Web服务器成功启动，并能够提供静态前端资源。

5.  **前端资源**
    *   创建了基础的 `data/index.html`、`data/style.css` 和 `data/script.js` 文件。
    *   通过浏览器访问ESP32的IP地址，验证了Web页面正常显示，并且前端的CSS样式和JavaScript交互（按钮点击）也正常工作。

6.  **实时通信链路 (WebSocket)**
    *   在 `Sys_WebServer` 中成功集成了 `AsyncWebSocket` 服务，并监听 `/ws` 路径。
    *   实现了基础的后端WebSocket事件处理器 (`onWsEvent`)，用于处理客户端连接、断开和数据收发。
    *   重构了 `data/script.js`，实现了完整的客户端WebSocket逻辑，包括动态连接、事件处理（`onopen`, `onmessage`, `onclose`）以及断线自动重连机制。
    *   优化了 `Sys_WebServer.cpp` 的路由逻辑，移除了冗余代码，使其更加健壮。

7.  **动态仪表盘 (按需请求)**
    *   根据您的反馈，将数据更新模式从“周期性推送”重构为更高效的“按需请求”。
    *   增强了 `Sys_MemoryManager` 模块，增加了接口以准确查询其内部管理的PSRAM内存池状态。
    *   重构了 `Sys_Tasks` 模块，使其在收到前端 `get_system_status` 命令时，能够调用 `Sys_MemoryManager` 获取准确的内存数据，并返回包含详细信息的JSON。
    *   更新了前端 `script.js`，使其能够正确解析新的JSON数据结构，并以更美观、清晰的格式展示运行时间、通用堆内存和PSRAM内存池的使用情况。
    *   通过增加 `Cache-Control` HTTP头，解决了浏览器缓存问题。
    *   通过完整的编译、烧录和硬件在环测试，最终验证了该功能的正确性。

### 后期开发计划与步骤

根据设计文档，下一阶段可聚焦于WIFI管理模块或设备控制模块的开发。

**阶段3：WIFI管理模块 (WIFI Management)**
*   **目标**：实现一个完整的WIFI设置界面，允许用户扫描网络、切换模式（AP/STA/AP-STA）并保存配置。
*   **主要任务**：
    *   创建 `Sys_WiFiManager` 模块，封装WIFI连接、扫描、模式切换逻辑。
    *   创建 `Sys_SettingsManager` 模块，用于将WIFI配置等信息持久化存储到NVS。
    *   设计并实现WIFI设置的前端UI，包括网络扫描的模态框、模式切换的单选按钮等。
    *   实现前后端通信逻辑，用于加载和保存WIFI配置。

### 使用说明

1.  **环境准备**：
    *   确保已安装VS Code和PlatformIO扩展。
    *   PlatformIO配置环境为 `espressif32@~6.11.0`，内置Arduino Core v3.2.0以上版本。
    *   确保开发板 `esp32s3_lckfb_N8R8` 已正确连接到COM端口（上传端口COM3，监视端口COM4）。
2.  **Wi-Fi配置**：
    *   打开 `src/main.cpp` 文件。
    *   在 `setupWiFiAndWebServer()` 函数中，将 `const char* ssid` 和 `const char* password` 替换为您的实际Wi-Fi网络名称和密码。
3.  **编译与上传**：
    *   在VS Code中，点击PlatformIO侧边栏的“Build”按钮（或运行 `platformio run` 命令）编译项目。
    *   点击“Upload”按钮（或运行 `platformio run -t upload` 命令）将固件上传到ESP32。
    *   点击“Monitor”按钮（或运行 `platformio run -t monitor` 命令）打开串口监视器，查看启动日志和IP地址。
4.  **访问Web界面**：
    *   从串口监视器获取ESP32的IP地址（例如：`192.168.1.103`）。
    *   在浏览器中输入该IP地址，即可访问管理系统的前端页面。
    *   点击“获取系统状态”按钮，可以验证仪表盘的实时数据更新功能。