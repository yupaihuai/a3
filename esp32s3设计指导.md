# 管理系统
(esp32s3)

## 硬件资源
(ESP32S3R8N8)
https://wiki.lckfb.com/zh-hans/esp32s3r8n8/

### Xtensa双核240MHz
(双核建议使用xTaskCreatePinnedToCore调度)
为队列中的数据定义一个清晰的 struct

#### Core 0
(摄像头模块数据流、识别相关处理)

#### Core 1
(Web Server, WebSocket，WIFI，蓝牙、UI交互等)

### 8MB PSRAM
(大块内存，PSRAM优先)

#### 按需求分配
(上传大文件暂存区、视频缓冲池Frame Buffer、i18n加载的完整JSON文件等)

#### 内存管理避免碎片化
可考虑基于PSRAM的内存池管理分配几个大的、固定尺寸的内存块用于对应需求，用完后归还。这比零散的 ps_malloc 要安全得多。

##### 建议创建 Sys_MemoryManager 单例类，在系统启动时，根据最大需求从PSRAM中 ps_malloc 几块大的、固定尺寸的内存块；由Sys_MemoryManager 内部维护这些内存块的可用状态需要大内存的任务（如 Task_ImageProcessor）向 MemoryManager 请求一个内存块 get_frame_buffer()，用完后通过 release_frame_buffer() 归还。

##### 实现建议：
使用 进行底层分配。 heap_caps_malloc（尺寸、MALLOC_CAP_SPIRAM）
内部维护一个固定大小或可配置的内存块数组（例如，根据预期的最大帧尺寸预分配N个帧缓冲区）。
每个内存块可以有一个状态（, ）。 可用IN_USE 
get_frame_buffer（）：从可用列表中查找一个块并标记为  IN_USE 
release_frame_buffer（）：将块标记为  可用
可以考虑更高级的池化策略，例如基于位图的空闲列表管理

### 8MB Flash

#### 闪存滤波优化
使用环形缓冲区 (Circular Buffer) 实现的移动平均，例如文件系统或OTA固件存储的优化。

#### 自定义分区表：my_8MB.csv
nvs=20k + otadata=8k + app0=2.5M + app1=2.5M + spiffs=932k + ffat=1984k + coredump=64k

##### nvs=20k  //存储Wi-Fi凭据、设备配置等小数据

##### otadata=8k  //管理OTA启动哪个分区

##### app0=2.5M、app1=2.5M  //编译后的文件存储区

##### spiffs=932k    //SPIFFS、用于存放 HTML, JS, CSS等UI资源

##### ffat=2M  //FAT、用于存放WAV, JSON 等大文件

##### coredump= 64k  //预留系统处理区

### USB OTG

#### 4K-USB摄像头
(MJPG格式10FPS)

##### 摄像条码用于识别

#### 1080P-USB摄像头
(MJPG格式30FPS)

##### 拍摄人像保存SD
(后期增加TFSD，可能需要考虑文件系统的初始化、读写性能、以及如何在分区表中预留空间。)

### 512KB SRAM 

#### 任务堆栈、频繁读写的变量等
性能要求高的数据(例如识别算法的小块数据)或关键性能代码


### 384KB ROM

### 16KB RTC SRAM

### 板载可编程LED（GPIO48）

### 板载BOOT按钮（GPIO0）

#### BOOT按钮（GPIO0）实现恢复出厂设置（复用"恢复出厂"）按钮功能

### I2S功放喇叭-后期接入

## 开发环境
(VS Code PlatformIO  - espressif32@~6.11.0 (Arduino Core v3.2.0以上版本)
https://github.com/platformio/platform-espressif32

### 前端框架
(响应式Web UI)

#### Bootstrap 5 / Vanilla JavaScript
(上传文件系统建议使用PlatformIO的extra_scripts，自动将 Web 资源压缩Gzip减少资源占用)

##### 建议使用PlatformIO 内建board_build.data_dir打包文件，在构建文件系统镜像(buildfs)前执行，完成Minify(移除空格和注释)和Gzip。在ESPAsyncWebServer中对.gz文件请求，例如添加响应头：response->addHeader("Content-Encoding", "gzip"); 用于浏览器自动解压提升加载速度

##### 建议使用fetch API 和 WebSocket获取数据，只更新页面上需要改变的部分而不是刷新整个页面

##### 使用JS的模板字符串（template literals）来动态生成HTML片段，并用 element.innerHTML 更新DOM（将事件监听器绑定在不会被重绘的父容器上，而不是绑定在会被替换掉的子元素上），这比操作单个DOM元素性能更好。
实现参考：在JS中维护一个全局state对象，当WebSocket收到数据，更新state对象，调用一个render()函数更新组件（例如一个卡片）。

###### // 代码参考示例：
const cardContainer = document.getElementById('card-container');
cardContainer.innerHTML = '<div class="card-content">...<button data-action="restart">重启</button>...</div>';
// 在初始化时，只绑定一次事件到父容器
cardContainer.addEventListener('click', (event) => {
  if (event.target.dataset.action === 'restart') {
    // 执行重启逻辑
  }
});

##### extra_scripts 实现细节：项目根目录创建并在ini内配置引用缩小Gzip 压缩，例如 minify_gzip.py脚本

##### 服务辅助（PWA）
未来实现离线访问或更快的加载速度，可以考虑将前端升级为渐进式Web应用（PWA），使用Service Worker进行资源缓存。

### 后端通信

#### ESPAsyncWebServer
(异步HTTP)

##### 依赖库：
AsyncTCP-esphome
ESPAsyncWebServer-esphome  //请注意这个库的调用类名是：AsyncWebServer
备用git，请忽略！
https://github.com/ESP32Async/ESPAsyncWebServer.git https://github.com/ESP32Async/AsyncTCP.git

#### WebSocket - 服务器主动通信
AJAX - 客户端主动请求
(实时数据)

##### 心跳机制：
WebSocket连接容易被路由器或代理断开，建议实现Ping/Pong心跳机制，确保连接活跃，并用于检测连接是否断开。

#### JSON
建议定义清晰简单、统一的通信协议格式的JSON RPC 2.0（远程过程调用）或事件驱动的协议，让前后端逻辑更清晰，通信更规范。

##### 依赖库：
ArduinoJson  //使用此库注意调用ArduinoJson @ ^7.4.1 版本。
备用请忽略！
避免使用过时的API,例如：StaticJsonDocument；使用时StaticJsonDocument doc(size);替换为 替换为 JsonDocument doc; JsonDocument 会自动处理内存，不再需要关心是Static还是Dynamic，也不用预设大小。

##### 错误码/消息统一：确保JSON响应中包含统一的成功/失败状态码和错误消息，方便前端进行国际化和错误提示。

### 服务层
(FreeRTOS任务设计)

#### FreeRTOS  队列 (Queues)
(数据流：源任务 -> 数据队列 -> Pusher任务 -> WebSocket)

##### 优先级划分：
Task_WebServer (Core 1, 优先级较高):
职责：运行ESPAsyncWebServer，快速处理HTTP和WebSocket的连接/命令事件，不被数据推送阻塞。
特性：轻量、快速响应。收到耗时命令就通过队列发给Task_Worker。

##### 优先级划分：
Task_ImageProcessor (Core 0, 优先级中等):
职责：循环获取UVC图像，运行解码和条码识别。
特性：CPU密集型。与Core 1完全隔离，通过xBarcodeQueue（事件驱动-条码识别队列）向外输出结果。

###### 摄像头到Web的条码数据流（跨核通信）
Task_ImageProcessor(运行在Core 0)捕获图像-运行识别库成功识别后通过 xBarcodeQueue 队列将结果发送给 Task_WebSocketPusher

##### 优先级划分：
Task_WebSocketPusher (Core 1, 优先级中等)：
职责：阻塞等待来自多个源头的数据（如条码识别队列 xBarcodeQueue、系统状态队列 xStateQueue）等通过WebSocket发送出
特性：专心处理数据推送，确保条码识别等实时数据能被尽快送达前端。

##### 优先级划分：
Task_Worker (Core 1, 优先级低)：
职责：等待并处理来自命令队列的各种任务（保存配置、OTA、控制外设等）。
特性：处理所有可能阻塞或耗时的操作。

##### 优先级划分：
Task_SystemMonitor (Core 1, 优先级低)：
职责：按需采集状态收集器，通过xStateQueue（事件驱动-系统状态队列）
特性：将周期性任务统一

#### FreeRTOS  信号量 (Semaphores)
保护共享资源，防止多任务同时访问导致的数据竞争和损坏

#### FreeRTOS  事件组 (Event Groups)
用于一个任务需要等待多个事件完成的场景

##### 多事件触发单任务，如事件组 Task_WebSocketPusher，可以等待一个事件组，其中包含“新条码数据就绪”、“系统状态更新”、“日志消息可用”等事件；它可以在任何一个事件发生时被唤醒，而不是轮询多个队列。

#### Watchdog Timer  监控 (WDT)
对于长时间运行且CPU密集型任务，要确保被FreeRTOS的WDT监控， 当任务卡死时，WDT可以触发系统重启。

#### 任务通知(Task Notifications)
如果一个任务只需要通知另一个任务某个事件发生了（而不是传递大量数据），FreeRTOS的任务通知比队列更轻量、更高效。

### 日志系统
(Frontend Console)

#### 将日志重定向到前端，这是一个很好的诊断工具。
考虑实现不同日志级别（DEBUG, INFO, WARN, ERROR, CRITICAL），并允许前端根据级别进行过滤显示。 可以增加时间戳和源任务/模块信息。

#### 日志持久化：
对于关键错误或崩溃日志，可以考虑将其写入Flash中的特定FAT分区，以便重启后仍然可以访问。

### 存储LittleFS、FATFS
(Arduino Core v3+版本已集成到框架中)

#### 在根项目结构为文件系统镜像建立独立的源文件夹：
data<-- 默认的data目录，用于默认的LittleFS/SPIFFS
media<-- 新增的media目录，用于 fatfs 的FAT分区
注意！esp-IDF固定先从ffat分区写入！分区顺序要先 fatfs再到SPIFFS。

##### 初始化：
建议使用 format on fail 的方式来初始化文件系统。首次启动时进行格式化，后续启动时不再强制格式化。
例如：if (!LittleFS.begin(false, "/spiffs", 10, "spiffs"))

##### 挂载：
建议使用 setupFilesystems() 函数挂载使用。


##### 验证：
测试该功能时建议使用testFilesystems() 函数测试

##### web路由映射：
SPIFFS 挂载：server.serveStatic("/", LittleFS, "/"); // 在 LittleFS 根目录查找文件
FFat 挂载：server.serveStatic("/media", ffat, "/");

#### 实现 Web上传功能，允许用户通过浏览器将WAV/JSON文件上传到FATFS分区

## 代码规范 
(代码开始前详细阅读本章节全部规范要求)

### 尽量使用较新版本的库/依赖等，调用时避免名称拼写错误，调用出错时优先查看对应库/依赖的示例或说明。

### 分阶段开发，每个阶段开发完成后，再次审视本设计大纲，确保符合要求；然后编写readme.md文档：(记录当前阶段设计说明和开发进度、后期开发计划及步骤、使用说明等)。之后测试已完成的项目，在经过确认正常后进入下一步，如未成功则按继续优化，优化成功后的内容更新至readme.md文档。完成要求后等待下一阶段的开发。

### 有详细友好的中文注释。

### 错误避免：
使用所有库、API、函数等前要符合开发环境版本、避免过时用法。各类命名要避免命名冲突！建议使用项目/命名空间前缀法，例如：Sys_WiFiManager.h (System)。

### 安全性：
开发前考虑整个系统安全性，为web增加一个密码登录机制，将密码的哈希值存在NVS中；ESPAsyncWebServer支持HTTP基础认证（Basic Auth）和基于表单的复杂认证。
HTTPS ：后期开发有必要时考虑；ESP-IDF支持mbedTLS，可以生成自签名证书用于局域网内访问或者对于互联网可访问的设备。
输入验证：建议对所有从前端接收的数据进行验证和清理，提升用户体验和减少无效请求。

### 强调错误处理与断言 (Error Handling & Asserts)
所有与系统资源交互的操作（如队列发送/接收、内存分配、文件读写、NVS操作）必须检查其返回值，并对可能的错误情况（如队列已满、内存不足）进行处理。
除了检查返回值，可考虑定义一个统一的错误类型或枚举。

### WebSocket重连机制
有完善的重连逻辑，前端JS监听WebSocket的onclose事件，触发后采用指数退避 (Exponential Backoff) 策略，直到最大间隔（如32秒）防止网络故障时持续请求。

### 状态同步
当用户用两个以上的页面在后台发起请求时，避免“脏数据”。建议Task_Worker成功保存任何配置后，让它通过xStateQueue通知Task_WebSocketPusher，Pusher任务不仅向发起请求的客户端发送成功Toast的消息，还向所有已连接的客户端广播一份最新的完整的配置JSON。

### 测试阶段：
对Sys_MemoryManagerSys_SettingsManager等模块进行单元测试，确保独立功能正确，之后再验证不同任务和模块之间的交互是否按预期工作。

### AI执行：拆分为更细小的许多个子任务依次进行，实现减小请求体积。

## UI/UX设计总则与布局

### 设计风格一致性：
统一色彩方案、字体、卡片样式和按钮风格，贯穿所有页面。

### 初始化原则：
FreeRTOS底层队列、网络服务、Web UI等、需要根据依赖关系在正确的时机启动；各种调用有足够的时间开始初始化。

### 实时反馈：
加载时间尽可能短，后续交互响应迅速，建议移动优先，然后为平板和桌面设备优化布局。利用WebSocke使用户操作、系统状态变更即时在界面上有所体现，无需刷新整个页面。

### 精确请求与响应：
点击操作按钮（例如“重启设备”）时，必须弹出一个Bootstrap的模态框（Modal）进行二次确认，防止误操作。用户确认后前端发送指令给Task_WebServer，Task_WebServer解析后执行对应的操作，耗时的操作则分发给Task_Worker。
非重启类的操作（例如“保存配置”），后端处理完成后，返回一个确认消息给前端，前端显示一个短暂的、非阻塞的浮动提示 (Toast)，告知用户操作结果，然后自动消失。

### 建议利用网格系统 (Grid System)和卡片系统(Cards)
适应移动端、平板端、桌面端

### 触控友好设计、线性内容流、图像img-fluid class自适应其容器宽度

### 加载指示器：
在进行网络请求、耗时操作时，有进度条提供视觉进度，避免用户认为应用卡死。

### 离线/在线状态：
在UI上明显地显示设备是否连接到互联网或局域网。

### 建议使用Sass编译器将文件编译成极小的CSS文件，然后minify、gzip，再上传到LittleFS。

### 桌面端布局(≥ Medium Screens)
采用Bootstrap Vertical Pills 主导，Horizontal Tabs 辅助，采用Bootstrap经典的三栏(或二栏)式布局

#### 
左侧：主导航栏，使用Bootstrap的 Vertical Pills
它是全局导航栏，用于切换各个功能模块。

#### 
中部：主内容区 (Main Content Area)
是动态变化区域，根据主导航的选择显示不同内容。
当内容需要再次分组时，在此区域顶部使用水平Tabs
(例如点击主导航的“设备控制”，这里就会出现WIFI、蓝牙、摄像头等Tabs)。

#### 
右侧/底部 : 全局状态/日志面板
固定或抽屉式弹出的面板，用于实时显示日志输出和一些关键状态（如连接状态、最近的事件）。这样用户在任何页面都能看到关键反馈。

### 移动端布局 (Small Screens)
利用桌面端的布局优雅地自适应到小屏幕

#### 
主导航栏 > Offcanvas 侧边栏
左侧的Vertical Pills导航栏会隐藏起来，页面顶部出现一个按钮 (Navbar Toggler)。点击后，主导航栏会以抽屉 (Offcanvas) 的形式从左侧滑出。

#### 
中部主内容区 > 垂直堆叠
如果内容区内部使用了水平Tabs，将Tabs转换为手风琴 (Accordion)。每个Tab变成一个可折叠的面板；建议通过响应式工具类（如d-md-none和d-none d-md-block）在HTML中实现

#### 
底部 : 全局状态/日志面板
从底部滑出的抽屉 (Offcanvas from bottom)

#### 
底部标签栏  (Bottom Navigation Bar) 
根据核心功能模块创建，可以用.fixed-bottom和Flexbox构建，如果超过4个，不常用或不重要的收纳为更多，点击“更多”可以拉起一个列表列出其余功能。例如全局状态/日志面板，在“更多”设置“日志”点击后触发一个Bootstrap的Offcanvas组件从屏幕底部向上滑出 (class="offcanvas-bottom")，内容是日志控制台。这个底部导航栏只在移动端可见。

#### 
底部上下文工具栏  (Contextual Actions) -
在所有包含表单和关键操作的页面，额外增加一个“底部工具栏”，用于放置“保存”、“更新”等按钮；仅在需要时出现。

## 功能模块
(模块化设计、单例模式封装class)
示例：
src/Sys_Filesh & .cpp //封装文件挂载读取。
src/Sys_SettingsManager.h & .cpp  //封装所有NVS读写操作。
src/Sys_WiFiManager.h & .cpp //封装WIFI连接、扫描、模式切换逻辑。
src/Sys_WebServer.h & .cpp  //封装ESPAsyncWebServer和WebSocket的初始化与事件处理。
src/Sys_Tasks.h & .cpp  //提供静态方法将每个任务作为一个类的成员函数

### Web服务模块
(配置/管理/响应式Web UI)

#### 总管理后台(Dashboard)
用户界面与实现

##### 布局建议：
无需次级Tabs。
顶部：一行关键指标卡片 (.col-md-6 .col-lg-3)，显示：WiFi状态、内存使用、运行时间、固件版本等。
中部：左侧大卡片 (.col-md-7)：系统信息列表 (.list-group)，显示IP、MAC等静态信息。右侧小卡片 (.col-md-5)：全局操作按钮（重启、恢复出厂），带模态框确认。
底部: 撑宽度的卡片 (.col-12)：日志控制台，内部可以有过滤按钮。

##### 功能实现建议：
数据源：（首次加载+手动刷新，点击刷新按钮才刷新）。
日志：日志系统可以被重定向，将格式化后的日志消息（带级别）发送到前端。
操作：例如点击“重启”按钮，通过WebSocket发送命令JSON到后端，后端WebSocket的回调函数解析command并执行ESP.restart()

###### 数据源：
首次加载：用户首次进入页面（或页面加载完成）时，前端JavaScript通过WebSocket发送一个初始数据请求，Task_WebServer在WebSocket回调中收到此命令后，由Task_Worker执行数据采集所有状态信息（WiFi RSSI, Uptime, Heap, Flash各分区大小, CPU Load等），打包成一个JSON对象发送。
手动刷新：在前端UI点击刷新按钮后，强制进行一次“首次加载”流程，立即获取最新数据。

##### 细分主题 3

#### 设备控制(Settings)
用户界面与实现

##### 布局建议：
主内容区顶部使用水平Tabs，内容对应WIFI设置、蓝牙设置、摄像头设置、OTA上传、语言选择等各模块功能的表单。

##### 功能实现建议：
当用户点击主导航进入“设备控制”模块时加载时前端JS立即通过WebSocket发送一个命令: {"command": "get_all_settings"}.；后端响应：Task_WebServer收到命令，通过队列分发给Task_Worker。
Task_Worker从NVS中读取WIFI、蓝牙、摄像头等所有配置，将所有配置整合成一个大的JSON对象，WebSocket将此JSON发回前端。前端WebSocket 的onmessage回调捕获这个JSON，例如调用一个populateSettingsForms(data)渲染函数，用收到的数据填充到所有Tabs下的对应表单控件中（输入框、开关、单选框等）。

### WIFI管理模块

#### WIFI配网： 
(使用 Unified Provisioning 的 SOFTAP配网)

##### 使用 wifi_prov_mgr_start_provisioning() 函数根据platformIo.ini内配网配置

#### WIFI设置 (WIFI Settings)
用户界面与实现

##### 布局建议：
总开关：一个（Toggle Switch）用于启用/禁用WIFI全部功能。
模式切换：使用一组 “内联单选按钮 (Inline Radios)来切换AP、STA、AP-STA模式，默认使用AP-STA模式（官方有WiFiMode： wifi_mode_t）。在STA、AP-STA模式下方有“扫描网络”按钮。
设置表单项：使用表单控件 (Form Controls) 为密码输入框添加一个可切换显示/隐藏密码的图标按钮，提升可用性；有占位符文本用于提示需要输入的内容，已设置则读取显示设置的值。
交互逻辑：使用JavaScript监听模式切换的单选按钮。当模式改变时，动态显示或隐藏相关的表单部分（例如，AP模式下隐藏“扫描网络”按钮和STA的密码输入框）。
高级选项：（使用Bootstrap的Collapse组件可折叠），用于配置静态IP相关。
保存按钮：桌面端在表单底部放置一个“保存”按钮；移动端采用“底部上下文工具栏”将“保存”按钮固定在屏幕底部，方便拇指操作。保存后弹出友好提示，之后刷新。

##### 功能实现建议：
“扫描网络”按钮，点击后使用Bootstrap的模态框(Modal)来展示扫描结果，扫描完成后，在模态框内显示网络列表。点击任何一个网络名称，其SSID会自动填入STA的输入框。有信息提示仅支持2.4G网络。

###### 数据加载：
进入设置页面时，前端通过WebSocket发送例如{"command": "get_all_settings"}，后端收到后，从NVS中读取所有配置，打包成一个大的JSON对象返回，前端用这些数据填充所有表单。

###### 扫描网络功能实现：
进入设置页面时，前端通过WebSocket发送例如{"command": "get_all_settings"}，后端收到后，从NVS中读取所有配置，打包成一个大的JSON对象返回，前端用这些数据填充所有表单。
备用请忽略：使用 Bootstrap 的 列表组 (List Group) 展示扫描结果。每个 list-group-item 都设置为可点击 (list-group-item-action)。

###### 保存配置：
点击任一“保存”按钮，前端将该表单的数据序列化成JSON，通过WebSocket发送。例如{"command": "save_wifi_settings", "data": {"ssid": "new_ssid", "pass": "new_pass"}}，后端收到save_*命令后将数据写入NVS完成持久化储存；完成后返回一个成功的Toast提示。

### 蓝牙管理模块

#### 蓝牙设置 (BLE Settings)
用户界面与实现

##### 布局建议：
总开关：一个（Toggle Switch）用于启用/禁用蓝牙全部功能。
设置表单项：使用输入框组 (Input Group) 组件来设置蓝牙广播名称。

##### 功能实现建议：
设置表单项：交互逻辑：用JavaScript监听开关的状态。当开关关闭时，将输入框设为disabled状态并可能灰显，向用户明确表示当前不可编辑。

#### （后期开发计划）
蓝牙播放功能实现建议：
(A2DP协议栈连接)

##### Wi-Fi和蓝牙共享同一个物理射频和天线，会相互干扰导致性能下降；建议在进行Wi-Fi密集操作时降低蓝牙的活动频率。可利用ESP-IDF提供的Wi-Fi/蓝牙共存（Coexistence）API，将其集成进来，根据当前活动的网络流量/蓝牙连接状态动态调整共存策略，以最小化干扰。 

##### 蓝牙音频用于播放简短的警告提示音（如ADPCM WAV文件）高优先级操作时降低蓝牙或暂停活动

##### 采用“连接-播放-断开” 的懒加载策略，动态连接管理不让ESP32与蓝牙设备保持长连接；触发播放提示音时，程序开始连接之前配对过的蓝牙设备；播放完成后，启动一个短暂的计时器（例如10秒），如果期间没有新的播放请求，则主动断开与音箱的连接，释放无线电资源。

### （中期开发计划）
USB摄像头管理模块

#### USB依赖库 ESP32_USB_Stream 
负责从摄像头获取 MJPEG/YUYV 帧，并可能进行 MJPEG 解码

##### https://github.com/esp-arduino-libs/ESP32_USB_Stream

#### （后期开发计划）
条码识别依赖库 zxing-cpp、ZBar等
备用请忽略：libjpeg-turbo等轻量JPEG

##### https://github.com/zxing-cpp/zxing-cpp
https://github.com/espressif/esp-idf-camera-scans-barcode-example

#### 摄像头界面UI (响应式Web )
仪表盘布局(Bootstrap的 Tabs 或 Vertical Pills)

##### 顶部：摄像头功能的开关、获取到的不同摄像头之间可切换

##### 布局：
大屏: 视频流占据主要区域，旁边或下方是控制面板（分辨率、帧率、开关等）。
小屏: 视频流占满屏幕宽度。控制面板可以设计成一个从底部滑出的 抽屉 (Offcanvas)，或者在视频流下方紧凑排列。这样能最大化视频可视区域。
控制: 使用图标按钮（如播放、暂停、拍照），并提供清晰的文字或 tooltip 提示。

##### 控制：获取摄像头内置的输出格式、分辨率，可调整的数值，比如对比度等。

#### USB摄像头功能实现建议：

##### 视频流：使用<img>指向视频流地址，
建议添加 .img-fluid class，使其自适应容器宽度。

#### （后期开发计划）
快递驿站出入库页面
（扫码识别画面、结果API推送等）

#### 快递出入库逻辑

##### 数据库对接

### （后期开发计划）
OTA管理模块

#### OTA界面UI (响应式Web )
仪表盘布局(Bootstrap的 Tabs 或 Vertical Pills)

##### OTA设置项：有输入框用于填写固件服务器URL，有占位符文本用于提示需要输入的内容，已设置则读取显示设置的值。
有一个“检查更新”按钮。

##### 文件选择：有文件上传框，有向导式提示需要输入的内容比如固件类型等；上传框后面有校验按钮，上传完成后校验按钮可用，校验成功则更新按钮可用，校验失败则前端浮动提示 (Toast)明确告知用户“固件无效或已损坏，请检查文件”。

##### 点击“更新”后，禁用页面所有其他按钮，显示一个带百分比的进度条，例如 (Progress Bar)，并附上提示文字：“正在上传... 请勿断电或关闭页面”。

##### 备用请忽略：结果反馈： 更新完成后，用一个醒目的警告框，例如(Alert) 显示成功或失败信息。成功后提示设备将3秒后重启。

#### OTA功能实现建议：

##### 文件输入：建议使用 Bootstrap 的 Form control 定制文件输入框。
上传文件：建议使用 XMLHttpRequest，可监听 xhr.upload.onprogress 事件来实时更新进度条的宽度和百分比文本。

##### 后端校验：在ESP32设备接收完整个固件文件后，在写入Flash之前，必须对其进行后端校验：数字签名验证或CRC/MD5校验。 数字签名可以防止恶意固件被上传和执行。

### （后期开发计划）
中、英文语言系统(i18n JSON)

#### 多语言界面UI (响应式Web )
仪表盘布局(Bootstrap的 Tabs 或 Vertical Pills)

##### zh.json

##### en.json

#### 多语言功能实现建议：

##### 在启动时根据用户配置或浏览器语言，只加载需要的json到PSRAM的一个全局JsonDocument中，后续文本从内存对象读取而不是访问文件系统，提高效率

##### 后端提供一个API GET /api/lang?locale=en_ZH。前端首次加载时，根据浏览器语言或用户设置请求对应的语言文件。获取后，将JSON对象存储在JavaScript的全局变量中，并保存在浏览器的 localStorage 中，以便下次访问时能立即加载，无需再次请求。

#### 前端语言选择： 
提供UI界面上的语言切换选项，并不仅依赖浏览器语言。

#### 热加载： 
如果可能，实现语言文件的热加载，无需重启设备即可切换语言。

## 阶段性实施方案建议
(分阶段测试)

### 阶段0：项目骨架与核心服务搭建 (The Foundation)
设计与验证： PSRAM正常集成，Flash分区正常挂载使用。项目能成功编译和上传，串口获取系统状态或硬件信息用于验证明硬件配置、文件上传、FreeRTOS调度器等工作正常。

### 阶段1：Web服务器与前端资源流水线 (The Web Stack)
设计与验证： 在浏览器中访问ESP32的IP地址，能看到 index.html 页面，且浏览器开发者工具显示资源是以gzip格式加载的。这验证了整个Web服务和前端资源流水线。

### 阶段2：实时通信与动态仪表盘 (Real-time Dashboard)
设计与验证： 仪表盘页面能实时、自动地更新系统数据，无需刷新页面。这验证了核心的后端数据流（源任务 -> 队列 -> Pusher任务 -> WebSocket）和前后端实时交互。

### 阶段3：WiFi管理模块 (Full WiFi Functionality)
设计与验证： 用户可以通过Web界面无缝地配置ESP32的Wi-Fi连接，配置能被持久化保存。

### 阶段4：USB摄像头集成与条码识别
设计与验证： Web界面能流畅显示USB摄像头的视频。当条码出现在画面中时，识别结果能实时显示在页面上。

### 阶段5：后期开发计划 - OTA、i18n 及其他

#### OTA: 在Web UI上创建文件上传表单。后端 ESPAsyncWebServer 提供文件上传处理接口，接收固件并调用 Update 库执行更新。

#### i18n: 创建 en.json 和 zh.json。前端JS在启动时根据浏览器语言或用户选择，fetch对应的JSON文件，然后用其内容动态渲染UI文本。

#### 蓝牙: 实现A2DP音频播放逻辑。

#### 安全性: 为Web服务器添加基于表单的登录认证。
