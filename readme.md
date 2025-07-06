# ESP32S3 管理系统开发蓝图

## 1. 项目状态评估 (截至当前)

**核心结论：项目基础稳固，核心架构已初步搭建，前端UI/UX已进行初步重构，但仍需进一步完善功能实现。**

经过全面的代码审查和硬件在环测试 (HIL)，项目已成功在 `esp32s3r8n8` 硬件上运行。测试确认了核心硬件配置（双核、8MB PSRAM、8MB Flash）、自定义分区表、文件系统以及基础Web服务器的正确性。

当前版本已完成了“阶段0/1”的骨架搭建，并根据设计指导对前端UI/UX进行了初步重构。FreeRTOS多任务架构已引入，为后续功能开发奠定了基础。

### 1.1. 已完成的工作

*   **硬件与环境验证**: 成功验证了 PlatformIO 环境、自定义板级配置和分区方案。
*   **核心模块占位**: 创建了 `Sys_MemoryManager`, `Sys_Filesystem`, `Sys_WebServer`, `Sys_SettingsManager`, `Sys_WiFiManager` 等核心模块的单例类骨架。
*   **FreeRTOS多任务架构**: 引入了 `Task_Worker`, `Task_WebSocketPusher`, `Task_SystemMonitor` 等FreeRTOS任务，并建立了命令队列 (`xCommandQueue`)、状态队列 (`xStateQueue`)。
*   **基础Web服务**: 启动了 `ESPAsyncWebServer`，能够提供静态 HTML/CSS/JS 文件，并建立了基础的 WebSocket 通信链路。
*   **按需数据获取与更新**: 实现了从前端请求、后端响应并更新系统状态（内存、运行时间、WiFi状态、IP地址等）的基础动态功能。
*   **前端UI/UX初步重构**:
    *   仪表盘页面已重构，包含了关键指标卡片、系统信息列表和全局操作按钮的占位符。
    *   设备控制页面中的WiFi设置表单已添加静态IP设置字段和密码显示/隐藏功能。
    *   引入了Bootstrap Icons，并为移动端添加了Offcanvas侧边栏结构。
    *   实现了基本的Toast提示和日志控制台功能。
*   **WiFi管理模块初步集成**: `Sys_WiFiManager` 已能加载NVS中的WiFi配置并尝试连接，并支持AP模式下的默认密码。

### 1.2. 待解决的关键差距 (Gap Analysis)

1.  **【最高优先级】WiFi配置持久化与配网模式完善**:
    *   **问题**: 虽然NVS读写已集成，但WiFi配置的保存和应用逻辑仍需在后端任务中完善。配网模式（Unified Provisioning）的完整实现尚未完成，目前仅是启动了一个开放AP。
    *   **解决方案**: 完善 `Task_Worker` 中 `CMD_SAVE_WIFI_SETTINGS` 的具体逻辑，确保配置能正确保存到NVS并触发WiFi重连。深入实现Unified Provisioning的流程，使其能够引导用户完成WiFi配置。

2.  **【架构完善】任务间通信与数据同步**:
    *   **问题**: `Task_WebSocketPusher` 和 `Task_SystemMonitor` 任务目前仍是占位符，未实现实际的数据采集和推送逻辑。
    *   **解决方案**: 实现 `Task_SystemMonitor` 周期性采集系统信息（如CPU负载、Flash使用、PSRAM使用详情等），并通过 `xStateQueue` 发送给 `Task_WebSocketPusher`。`Task_WebSocketPusher` 负责监听所有数据队列（包括未来的条码队列），并将数据格式化为JSON通过WebSocket推送给前端。

3.  **【UI/UX完善】前端交互与反馈**:
    *   **问题**: 虽然UI结构已初步搭建，但许多交互细节（如加载指示器、模态框二次确认的后端响应、错误提示的国际化）尚未完全实现。
    *   **解决方案**: 完善前端JavaScript，实现加载指示器的显示/隐藏、操作按钮的禁用/启用。为“重启设备”和“恢复出厂设置”按钮添加后端命令处理和前端模态框二次确认的完整逻辑。

4.  **【功能缺失】其他功能模块的实现**:
    *   **问题**: 蓝牙管理、USB摄像头管理、OTA管理、多语言系统等核心功能模块尚未开始实现。
    *   **解决方案**: 按照 `esp32s3设计指导.md` 中的规划，逐步实现这些功能模块的后端逻辑和前端UI。

## 2. 下一阶段开发计划：完善Web服务模块核心功能

根据您的指示，我们继续深入 **阶段2：Web服务模块** 的开发。此阶段的核心目标是解决上述关键差距中的优先级任务，特别是WiFi配置的完整实现和任务间通信的完善。

### 2.1. 任务一：完善WIFI管理与持久化 (后端)

*   **目标**: 确保用户通过 Web 界面配置的 Wi-Fi 连接能够永久保存，并能正确应用。
*   **具体步骤**:
    1.  **`Sys_SettingsManager` 验证**: 确认 `saveWiFiSettings` 和 `loadWiFiSettings` 函数的健壮性，处理NVS读写错误。
    2.  **`Sys_WiFiManager` 优化**: 确保 `saveSettingsAndReconnect` 能够平滑地断开旧连接并使用新配置进行连接。
    3.  **`Task_Worker` 完善**: 确保 `CMD_SAVE_WIFI_SETTINGS` 命令能够正确解析前端发送的完整WiFi配置JSON，并调用 `Sys_WiFiManager::saveSettingsAndReconnect`。

### 2.2. 任务二：实现系统状态周期性推送 (后端)

*   **目标**: 实现 `Task_SystemMonitor` 周期性采集系统状态，并通过 `Task_WebSocketPusher` 推送给前端。
*   **具体步骤**:
    1.  **`Task_SystemMonitor` 实现**: 在该任务中，周期性地（例如每5秒）收集ESP32的运行时间、堆内存、PSRAM使用、WiFi连接状态、IP地址、芯片信息（型号、CPU频率、Flash大小、PSRAM大小、IDF版本）等。
    2.  **数据封装**: 将收集到的数据封装为JSON对象。
    3.  **队列发送**: 将JSON数据通过 `xStateQueue` 发送给 `Task_WebSocketPusher`。

### 2.3. 任务三：完善WebSocket数据推送 (后端)

*   **目标**: `Task_WebSocketPusher` 能够接收来自不同队列的数据，并统一通过WebSocket推送给前端。
*   **具体步骤**:
    1.  **`Task_WebSocketPusher` 实现**: 监听 `xStateQueue` 和 `xCommandQueue` (用于Toast消息等)。
    2.  **数据分发**: 根据消息类型，将数据通过 `_ws->textAll()` 或 `_ws->text(client->id(), ...)` 推送给所有或特定客户端。

### 2.4. 任务四：前端UI/UX交互细节完善

*   **目标**: 提升用户体验，实现加载指示器、模态框二次确认等。
*   **具体步骤**:
    1.  **加载指示器**: 在前端发送耗时命令（如扫描WiFi、保存设置）时，显示加载动画或进度条，并在收到响应后隐藏。
    2.  **操作确认模态框**: 为“重启设备”和“恢复出厂设置”按钮实现Bootstrap模态框二次确认。
    3.  **日志显示**: 确保后端推送的日志能正确显示在前端的日志控制台中，并支持滚动。

## 3. 使用说明

1.  **环境准备**:
    *   确保已安装 VS Code 和 PlatformIO 扩展。
    *   PlatformIO 环境为 `espressif32@~6.11.0`。
    *   **检查端口配置**: 打开 `platformio.ini` 文件，确认 `upload_port` 和 `monitor_port` 与您的硬件连接匹配。
2.  **Wi-Fi配置 (当前版本)**:
    *   当前版本已支持通过Web界面配置Wi-Fi。首次启动时，设备将尝试加载NVS中的配置。如果未配置，将启动一个名为 "ESP32S3-Config" (密码: 12345678) 的AP，您可以通过此AP连接设备并访问Web界面进行配置。
3.  **编译与上传**:
    *   使用 PlatformIO 的 "Build", "Upload", "Upload Filesystem" 和 "Monitor" 功能。
    *   **完整命令**: `platformio run -t clean -t uploadfs -t upload -t monitor`
4.  **访问Web界面**:
    *   从串口监视器获取 IP 地址，然后在浏览器中访问。
    *   如果设备处于配网模式，连接到 "ESP32S3-Config" AP后，访问 `http://192.168.4.1`。