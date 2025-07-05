# ESP32S3 管理系统开发蓝图

## 1. 项目状态评估 (截至当前)

**核心结论：项目基础稳固，但功能实现与设计蓝图存在显著差距。**

经过全面的代码审查和硬件在环测试 (HIL)，项目已成功在 `esp32s3r8n8` 硬件上运行。测试确认了核心硬件配置（双核、8MB PSRAM、8MB Flash）、自定义分区表、文件系统以及基础Web服务器的正确性。

然而，当前版本仅完成了“阶段0/1”的骨架搭建，与 `esp32s3设计指导.md` 中定义的宏伟目标相比，仍处于早期原型阶段。

### 1.1. 已完成的工作

*   **硬件与环境验证**: 成功验证了 PlatformIO 环境、自定义板级配置和分区方案。
*   **核心模块占位**: 创建了 `Sys_MemoryManager`, `Sys_Filesystem`, `Sys_WebServer` 等核心模块的单例类骨架。
*   **基础Web服务**: 启动了 `ESPAsyncWebServer`，能够提供静态 HTML/CSS/JS 文件，并建立了基础的 WebSocket 通信链路。
*   **按需数据获取**: 实现了从前端请求、后端响应并更新系统状态（内存、运行时间）的基础动态功能。

### 1.2. 待解决的关键差距 (Gap Analysis)

1.  **【最高优先级】网络配置硬编码**:
    *   **问题**: WiFi 的 SSID 和密码目前硬编码在 `src/main.cpp` 中。这是当前项目最大的短板，完全不符合设计文档中关于“SOFTAP配网”的要求，导致设备无法脱离开发环境进行部署。
    *   **解决方案**: 立即着手开发 **WIFI管理模块** 和 **设置持久化模块**。

2.  **【架构差距】缺乏多任务架构**:
    *   **问题**: 系统目前运行在 Arduino 的 `setup()` 和 `loop()` 单任务模式下。设计文档中规划的、用于确保系统响应性和稳定性的 FreeRTOS 多任务架构（如 `Task_WebServer`, `Task_Worker`）尚未实现。
    *   **解决方案**: 在开发后续功能（如WIFI设置保存）时，必须同步引入 FreeRTOS 队列和任务，将耗时操作从主Web任务中分离。

3.  **【UI/UX差距】前端界面简陋**:
    *   **问题**: 当前的 Web 页面仅为功能验证的占位符，与设计文档中规划的、基于 Bootstrap 的响应式、多模块仪表盘相去甚远。
    *   **解决方案**: 在下一阶段的开发中，严格按照 UI/UX 设计总则重构整个前端。

## 2. 下一阶段开发计划：完善Web服务模块

根据您的指示，我们正式进入 **阶段2：Web服务模块** 的开发。此阶段的核心目标是解决上述差距，首先从最关键的网络配置入手。

### 2.1. 任务一：实现WIFI管理与持久化

*   **目标**: 让用户可以通过 Web 界面配置设备的 Wi-Fi 连接，并永久保存设置。
*   **后端任务**:
    1.  **完善 `Sys_WiFiManager`**: 封装 Wi-Fi 扫描、模式切换 (AP/STA/AP-STA)、连接等核心逻辑。
    2.  **完善 `Sys_SettingsManager`**: 封装使用 `Preferences` 库对 NVS（非易失性存储）的读写操作，用于持久化保存 Wi-Fi 配置。
    3.  **剥离 `main.cpp`**: 将 Wi-Fi 连接逻辑从 `main.cpp` 中移除，改为在启动时从 NVS 加载配置。如果无配置，则启动配网模式。
    4.  **实现 WebSocket API**: 创建处理前端请求的 WebSocket 命令，如 `get_wifi_settings`, `save_wifi_settings`, `scan_wifi_networks`。
*   **前端任务**:
    1.  **创建“设备控制”页面**: 根据设计文档，建立包含“WIFI设置”Tab的页面。
    2.  **实现WIFI设置表单**: 设计 UI，包含模式切换、SSID/密码输入框、保存按钮等。
    3.  **实现网络扫描模态框**: 点击“扫描”按钮后，弹出模态框，以列表形式显示扫描到的网络，点击后可自动填充SSID。
    4.  **实现前后端交互**: 编写 JavaScript，通过 WebSocket 发送命令、接收数据并动态更新UI。

## 3. 使用说明

1.  **环境准备**:
    *   确保已安装 VS Code 和 PlatformIO 扩展。
    *   PlatformIO 环境为 `espressif32@~6.11.0`。
    *   **检查端口配置**: 打开 `platformio.ini` 文件，确认 `upload_port` 和 `monitor_port` 与您的硬件连接匹配。
2.  **Wi-Fi配置 (当前版本)**:
    *   **注意**: 当前版本使用硬编码的 Wi-Fi 凭据。
    *   打开 `src/main.cpp` 文件，在 `setupWiFiAndWebServer()` 函数中修改 `ssid` 和 `password`。
3.  **编译与上传**:
    *   使用 PlatformIO 的 "Build", "Upload", "Upload Filesystem" 和 "Monitor" 功能。
    *   **完整命令**: `platformio run -t clean -t uploadfs -t upload -t monitor`
4.  **访问Web界面**:
    *   从串口监视器获取 IP 地址，然后在浏览器中访问。