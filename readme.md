# ESP32S3 管理系统开发日志

## 阶段0：项目骨架与核心服务搭建 (The Foundation) & 阶段1：Web服务器与前端资源流水线 (The Web Stack)

### 设计说明与开发进度

本阶段主要目标是搭建ESP32S3管理系统的基础骨架，并实现Web服务器和前端资源流水线，确保硬件环境、内存管理、文件系统以及Web服务核心功能的正常运行。

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

### 后期开发计划与步骤

根据设计文档，下一阶段将聚焦于实时通信与动态仪表盘的实现。

**阶段2：实时通信与动态仪表盘 (Real-time Dashboard)**
*   **目标**：实现WebSocket通信，使仪表盘页面能实时、自动地更新系统数据，无需刷新页面。
*   **主要任务**：
    *   在 `Sys_WebServer` 中集成 WebSocket 功能。
    *   设计清晰简单、统一的通信协议格式（JSON RPC 2.0 或事件驱动协议）。
    *   实现心跳机制，确保WebSocket连接活跃。
    *   引入 `ArduinoJson` 库处理JSON数据。
    *   设计FreeRTOS任务：`Task_WebSocketPusher` (Core 1, 优先级中等)，负责阻塞等待来自多个源头的数据（如条码识别队列 `xBarcodeQueue`、系统状态队列 `xStateQueue`）并通过WebSocket发送。
    *   设计FreeRTOS任务：`Task_SystemMonitor` (Core 1, 优先级低)，负责周期性收集系统状态并通过 `xStateQueue` 输出。
    *   前端JS使用 `fetch API` 和 `WebSocket` 获取数据，只更新页面上需要改变的部分。
    *   前端JS使用模板字符串动态生成HTML片段，并用 `element.innerHTML` 更新DOM。

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