# ESP32-S3 模块化管理系统

本项目旨在为 `ESP32-S3 (N8R8)` 开发一个高效、模块化的嵌入式管理系统。系统充分利用了ESP32-S3的硬件资源，并提供一个功能丰富的响应式Web UI用于设备监控与管理。

本文档遵循 **[`esp32s3设计指导.md`](esp32s3设计指导.md)** 的规范编写，记录了项目的设计、进度、计划和使用说明。

---

## 1. 当前阶段设计说明 (Current Stage Design Description)

项目当前已完成 **阶段2** 的核心功能开发，整体设计如下：

### 2.1 软件架构
-   **模块化设计**: 核心功能均被封装为独立的C++单例类（如 `Sys_WiFiManager`, `Sys_WebServer` 等），实现了高内聚、低耦合。
-   **FreeRTOS驱动**: 系统运行于FreeRTOS之上，通过任务和队列实现异步非阻塞操作。`Task_Worker` 负责处理耗时命令，`Task_SystemMonitor` 负责处理周期性任务（如WiFi重连），确保了系统的稳定性和响应速度。
-   **硬件抽象**: `Sys_SettingsManager` 封装了NVS操作，`Sys_Filesystem` 统一管理多文件系统，`Sys_MemoryManager` 为PSRAM管理提供了基础。

### 2.2 Web服务设计
-   **后端**: 采用 `ESPAsyncWebServer` 提供HTTP服务，并通过 `AsyncWebSocket` 实现与前端的实时、双向通信。
-   **前端**: 基于 **Bootstrap 5** 和 **Vanilla JavaScript** 构建响应式UI，自适应桌面和移动设备。
-   **通信协议**: 前后端交互采用统一的JSON命令格式，所有耗时操作均由前端发送命令，后端 `Task_Worker` 接收并处理，处理结果通过WebSocket广播给所有客户端，实现了状态同步。
-   **性能优化**: Web资源（JS, CSS）在编译时通过Python脚本自动进行 **Minify** 和 **Gzip** 压缩，并通过服务器进行内容协商，显著提升了前端加载速度。

---

## 3. 开发进度 (Development Progress)

-   **[✔️] 阶段 0 & 1: 基础框架与Web服务流水线**
    -   已完成硬件验证、核心服务类封装、Web服务器启动、前端资源优化和WebSocket通信通道的建立。

-   **[✔️] 阶段 2: 核心功能模块开发**
    -   **Web服务模块**: 已完成仪表盘的动态数据刷新、全局操作（重启/恢复出厂设置）和实时日志显示功能。
    -   **WiFi管理模块**: 已完成完整的Web配网功能，包括模式切换、SSID扫描、静态IP配置等。并对该模块进行了关键重构，统一了配网方案，优化了模式切换的健壮性。

---

## 4. 后期开发计划及步骤 (Future Development Plan)

根据设计指导，下一步将继续完成阶段2的剩余模块，之后进入后续阶段。

### 4.1 蓝牙管理模块 (下一步工作)
-   **目标**: 实现蓝牙功能的Web UI配置。
-   **实施步骤**:
    1.  在“设备控制”页面增加“蓝牙设置”Tab。
    2.  UI上提供一个总开关（Toggle Switch）用于启用/禁用蓝牙功能。
    3.  UI上提供一个输入框用于设置蓝牙广播名称。
    4.  后端实现对应的WebSocket命令，用于保存蓝牙开关状态和设备名称到NVS。

### 4.2 后续阶段
-   **[ ] 阶段 4: USB摄像头集成与条码识别**
-   **[ ] 阶段 5: 后期功能开发 (OTA, i18n等)**

---

## 5. 待优化项与设计要点 (Areas for Improvement)
1.  **深化内存管理器 (`Sys_MemoryManager`) 实现:**
    *   **目标**: 从根本上避免内存碎片，为摄像头等高内存消耗任务提供稳定保障。
    *   **实施建议**: 将当前通用的内存分配模式重构为 **固定大小的内存块池**。在系统启动时，根据预设的块大小和数量，一次性从PSRAM中分配所有内存。并提供如 `get_frame_buffer()` / `release_frame_buffer()` 这样的专用接口供上层任务调用。

2.  **精细打磨Web UI响应式体验:**
    *   **目标**: 提升移动端的用户体验，使其更符合现代Web App的设计标准。
    *   **实施建议**: 在“设备控制”页面，当屏幕宽度小于 `md` (768px) 时，将当前的 **水平Tabs** 自动转换为 **手风琴 (Accordion)** 布局。这可以通过Bootstrap的响应式显示工具类 (`d-md-block`, `d-none d-md-block` 等) 轻松实现。

3.  **增强系统安全性 - 实现Web登录认证:**
    *   **目标**: 防止未经授权的访问，保护设备配置。
    *   **实施建议**:
        *   创建一个简单的 `login.html` 登录页面。
        *   在 `Sys_WebServer` 中增加会话验证逻辑，未认证的访问将重定向到登录页。
        *   在 `Sys_SettingsManager` 中增加 `savePasswordHash()` 和 `verifyPassword()` 函数，用于密码的存储和验证。
        *   登录成功后，服务器生成会话令牌，客户端在后续的请求中（HTTP或WebSocket）需携带此令牌进行验证。
---

## 5. 使用说明 (Usage Instructions)

### 5.1 所需环境
-   **硬件**: ESP32-S3 开发板 (N8R8 - 8MB Flash, 8MB PSRAM)。
-   **软件**: Visual Studio Code + PlatformIO IDE 插件。

### 5.2 项目配置
1.  克隆本项目仓库。
2.  打开 `platformio.ini` 文件。
3.  根据您的硬件连接，修改 `upload_port` 和 `monitor_port` 的COM端口号。

### 5.3 编译与上传
-   **首次使用或更新Web UI时，必须同时上传固件和文件系统：**
    ```shell
    platformio run -t upload -t uploadfs
    ```
-   **如果只修改了C++代码，仅需上传固件：**
    ```shell
    platformio run -t upload
    ```

### 5.4 系统访问
1.  通过PlatformIO的串口监视器查看设备启动后输出的IP地址。
2.  在浏览器中打开 `http://<设备IP地址>` 即可访问Web管理界面。
3.  **初次使用**: 如果设备未连接过WiFi，它会启动一个名为 `ESP32S3-Config` 的热点（密码: `12345678`）。请连接此热点，然后通过浏览器访问 `http://192.168.4.1` 进入Web界面进行配网。


---

## 5. 待优化项与设计要点 (Areas for Improvement)

在当前功能基础上，为使项目更加健壮、专业和安全，以下是根据 **[`设计蓝图_v2.md`](设计蓝图_v2.md)** 提出的下一步优化建议：

1.  **深化内存管理器 (`Sys_MemoryManager`) 实现:**
    *   **目标**: 从根本上避免内存碎片，为摄像头等高内存消耗任务提供稳定保障。
    *   **实施建议**: 将当前通用的内存分配模式重构为 **固定大小的内存块池**。在系统启动时，根据预设的块大小和数量，一次性从PSRAM中分配所有内存。并提供如 `get_frame_buffer()` / `release_frame_buffer()` 这样的专用接口供上层任务调用。

2.  **精细打磨Web UI响应式体验:**
    *   **目标**: 提升移动端的用户体验，使其更符合现代Web App的设计标准。
    *   **实施建议**: 在“设备控制”页面，当屏幕宽度小于 `md` (768px) 时，将当前的 **水平Tabs** 自动转换为 **手风琴 (Accordion)** 布局。这可以通过Bootstrap的响应式显示工具类 (`d-md-block`, `d-none d-md-block` 等) 轻松实现。

3.  **增强系统安全性 - 实现Web登录认证:**
    *   **目标**: 防止未经授权的访问，保护设备配置。
    *   **实施建议**:
        *   创建一个简单的 `login.html` 登录页面。
        *   在 `Sys_WebServer` 中增加会话验证逻辑，未认证的访问将重定向到登录页。
        *   在 `Sys_SettingsManager` 中增加 `savePasswordHash()` 和 `verifyPassword()` 函数，用于密码的存储和验证。
        *   登录成功后，服务器生成会话令牌，客户端在后续的请求中（HTTP或WebSocket）需携带此令牌进行验证。
