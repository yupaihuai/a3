#include <Arduino.h>
#include "esp_spi_flash.h" // 用于获取 Flash 芯片信息
#include "esp32/spiram.h"  // 用于获取 PSRAM 信息
#include "esp_partition.h" // 用于遍历分区表
#include "esp_idf_version.h" // 用于获取 ESP-IDF 版本
#include "esp_heap_caps.h" // 用于获取不同内存类型堆信息
#include "Sys_MemoryManager.h" // 引入内存管理器
#include "Sys_Filesystem.h"    // 引入文件系统管理器
#include "Sys_WiFiManager.h"   // 引入WiFi管理器
#include "Sys_WebServer.h"     // 引入Web服务器管理器
#include "Sys_Tasks.h"         // 引入任务管理器
#include "Sys_SettingsManager.h" // 引入设置管理器

// 函数声明
void printDiagnostics();
void setupMemoryManager(); // 设置内存管理器
void setupFilesystems();   // 设置文件系统
void setupWiFiAndWebServer(); // 新增：设置Wi-Fi和Web服务器

void setup() {
  // 1. 初始化串口，直接使用数值
  Serial.begin(115200);

  // 2. 打印启动信息
  Serial.println(F("\n\n[FAST BOOT] System starting..."));
  Serial.println(F("[快速启动] 系统启动..."));

  // ----------------------------------------------------------------
  //  开发调试信息 (默认禁用以加速启动)
  //  在需要详细硬件信息时，请取消以下两行代码的注释
  // ----------------------------------------------------------------
  // delay(3000); // 确保串口监视器有时间连接
  // printDiagnostics(); // 打印详细的硬件和配置检查信息
  // ----------------------------------------------------------------

  // 3. 设置内存管理器
  setupMemoryManager();

  // 6. 设置文件系统
  setupFilesystems();

  // 7. 设置Wi-Fi和Web服务器
  setupWiFiAndWebServer();

  // 8. 打印结束信息
  Serial.println(F("\n\n============================================="));
  Serial.println(F("Check finished. The system will now idle."));
  Serial.println(F("检查完成。系统现在将进入空闲状态。"));
  Serial.println(F("============================================="));
}

void loop() {
  // 核心优化：让 loop() 空闲
  // 诊断任务已在 setup() 中完成。
  // loop() 函数将保持空闲状态，以降低CPU使用率。
  // vTaskDelay() 是比 delay() 更高效的等待方式，它会把CPU让给其他任务或空闲任务。
  vTaskDelay(pdMS_TO_TICKS(1000));
}

/**
 * @brief 打印所有硬件和配置的诊断信息
 */
void printDiagnostics() {
  Serial.println(F("\n\n============================================="));
  Serial.println(F(" ESP32-S3 Hardware & Configuration Check"));
  Serial.println(F(" ESP32-S3 硬件 & 配置 检查"));
  Serial.println(F("============================================="));

  // --- 0. System Information / 系统信息 ---
  Serial.println(F("\n--- 0. System Information / 系统信息 ---"));
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  Serial.print(F("Chip / 芯片: ESP32-S3 rev "));
  Serial.println(chip_info.revision);
  Serial.print(F("CPU Cores / 内核数: "));
  Serial.println(chip_info.cores);
  Serial.print(F("CPU Frequency / 频率: "));
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(F(" MHz"));
  Serial.print(F("ESP-IDF Version / 版本: "));
  Serial.println(esp_get_idf_version());

  // --- 1. Flash and PSRAM Verification / 验证---
  Serial.println(F("\n--- 1. Flash and PSRAM Verification / 验证---"));

  // 获取 Flash 芯片大小 (来自芯片硬件)
  uint32_t flash_size_bytes = spi_flash_get_chip_size();
  Serial.print(F("Flash Size / 芯片大小 (由硬件检测): "));
  Serial.print(flash_size_bytes / (1024 * 1024));
  Serial.println(F(" MB"));

  // 获取 PSRAM 芯片大小 (来自芯片硬件)
  uint32_t psram_size_bytes = esp_spiram_get_size();
  if (psram_size_bytes > 0) {
    Serial.print(F("PSRAM Size / PSRAM大小 (由硬件检测): "));
    Serial.print(psram_size_bytes / (1024 * 1024));
    Serial.println(F(" MB"));
  } else {
    Serial.println(F("PSRAM: 未检测到或未启用!"));
  }

  // 检查编译时启用的 PSRAM 类型
  #if CONFIG_SPIRAM_MODE_OCT
    Serial.println(F("PSRAM Mode / 模式 (Compiled for): OPI (Octal 八线程)"));
  #elif CONFIG_SPIRAM_MODE_QUAD
    Serial.println(F("PSRAM Mode / 模式 (Compiled for): QPI (Quad 四线程)"));
  #else
    Serial.println(F("PSRAM Mode / 模式 (Compiled for): PSRAM not enabled in config! 配置未启用!"));
  #endif

  // 检查可用的堆内存 (Heap)
  uint32_t free_heap = ESP.getFreeHeap();
  uint32_t total_heap = ESP.getHeapSize();
  Serial.print(F("Heap Memory (Overall) / 堆内存 (总览): "));
  Serial.print(free_heap / 1024);
  Serial.print(F(" KB Free / "));
  Serial.print(total_heap / 1024);
  Serial.println(F(" KB Total"));

  // **新增：检查PSRAM专用堆内存**
  uint32_t free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  uint32_t total_spiram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  Serial.print(F("Heap Memory (PSRAM) / 堆内存 (PSRAM): "));
  Serial.print(free_spiram / 1024);
  Serial.print(F(" KB Free / "));
  Serial.print(total_spiram / 1024);
  Serial.println(F(" KB Total"));


  if (total_heap > 520 * 1024 || total_spiram > 0) { // ESP32-S3 内部SRAM约512KB
      Serial.println(F("-> 确认: PSRAM 已成功集成到堆内存中。"));
      Serial.println(F("-> PSRAM is successfully integrated into Heap memory!"));
  } else {
      Serial.println(F("-> Warning: PSRAM does not seem to be integrated into Heap. Check your config!"));
      Serial.println(F("-> 警告: PSRAM 似乎未集成到堆内存中，请检查您的配置!"));
  }

  // --- 2. 验证分区表 ---
  Serial.println(F("\n--- 2. Partition Table Verification / 分区表校验---"));
  Serial.println(F("Type      | Subtype   | Address    | Size (bytes) | Label"));
  Serial.println(F("----------------------------------------------------------------"));

  esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
  if (!it) {
      Serial.println(F("错误：无法找到任何分区。"));
      return;
  }
  
  while (it != NULL) {
    const esp_partition_t *part = esp_partition_get(it);
    
    // 准备子类型字符串
    const char* subtype_str;
    switch(part->subtype) {
        case ESP_PARTITION_SUBTYPE_APP_OTA_0: subtype_str = "ota_0"; break;
        case ESP_PARTITION_SUBTYPE_APP_OTA_1: subtype_str = "ota_1"; break;
        case ESP_PARTITION_SUBTYPE_DATA_NVS: subtype_str = "nvs"; break;
        case ESP_PARTITION_SUBTYPE_DATA_OTA: subtype_str = "otadata"; break;
        case ESP_PARTITION_SUBTYPE_DATA_SPIFFS: subtype_str = "spiffs"; break;
        case ESP_PARTITION_SUBTYPE_DATA_FAT: subtype_str = "fat"; break;
        case ESP_PARTITION_SUBTYPE_DATA_COREDUMP: subtype_str = "coredump"; break;
        default: subtype_str = "unknown"; break;
    }

    // 使用更安全的 snprintf 格式化输出，防止缓冲区溢出
    char line[128];
    snprintf(line, sizeof(line), "%-10s| %-9s | 0x%08X | %-12u | %s",
            (part->type == ESP_PARTITION_TYPE_APP) ? "app" : "data",
            subtype_str,
            part->address,
            part->size,
            part->label);
    Serial.println(line);
    
    it = esp_partition_next(it);
  }
  esp_partition_iterator_release(it);
  
  Serial.println(F("\n-> 请将此表与您的分区表CSV文件进行比较，它们应该完全匹配！"));
}

/**
 * @brief 设置内存管理器
 */
void setupMemoryManager() {
    Serial.println(F("\n\n============================================="));
    Serial.println(F(" Initializing Sys_MemoryManager"));
    Serial.println(F(" 初始化 Sys_MemoryManager"));
    Serial.println(F("============================================="));

    Sys_MemoryManager* memManager = Sys_MemoryManager::getInstance();
    // 根据设计文档，为摄像头模块数据流、识别相关处理预留内存
    // 假设摄像头帧大小为 1MB，预分配 4 个块
    memManager->initialize(1024 * 1024, 4); // 1MB per block, 4 blocks

    // 简单测试内存分配和释放
    Serial.println(F("\n--- Testing Sys_MemoryManager ---"));
    void* block1 = memManager->getMemoryBlock(500 * 1024); // 请求 500KB
    if (block1) {
        Serial.println(F("Successfully got a 500KB block."));
    }
    void* block2 = memManager->getMemoryBlock(1024 * 1024); // 请求 1MB
    if (block2) {
        Serial.println(F("Successfully got a 1MB block."));
    }
    memManager->releaseMemoryBlock(block1);
    Serial.println(F("Released 500KB block."));
    memManager->printMemoryInfo(); // 打印当前内存池状态
    Serial.println(F("---------------------------------"));
}

/**
 * @brief 设置文件系统
 */
void setupFilesystems() {
    Serial.println(F("\n\n============================================="));
    Serial.println(F(" Initializing Sys_Filesystem"));
    Serial.println(F(" 初始化 Sys_Filesystem"));
    Serial.println(F("============================================="));

    Sys_Filesystem* fsManager = Sys_Filesystem::getInstance();
    if (fsManager->setupFilesystems()) {
        Serial.println(F("Filesystems initialized successfully."));
        fsManager->testFilesystems(); // 测试文件系统
    } else {
        Serial.println(F("Failed to initialize filesystems."));
    }
}

/**
 * @brief 设置Wi-Fi和Web服务器
 */
void setupWiFiAndWebServer() {
    Serial.println(F("\n\n============================================="));
    Serial.println(F(" Initializing WiFi and Web Server"));
    Serial.println(F(" 初始化 Wi-Fi 和 Web 服务器"));
    Serial.println(F("============================================="));

    Sys_SettingsManager* settingsManager = Sys_SettingsManager::getInstance();
    Sys_WiFiManager* wifiManager = Sys_WiFiManager::getInstance();

    // 总是尝试加载并应用WiFi设置
    WiFiSettings storedSettings = settingsManager->loadWiFiSettings();

    if (storedSettings.ssid.length() > 0) {
        // 如果NVS中有保存的SSID，则尝试连接
        Serial.println(F("WiFi is configured. Attempting non-blocking connect..."));
        wifiManager->begin(); // 使用加载的设置进行连接
    } else {
        // 如果NVS中没有SSID，则启动配网模式
        Serial.println(F("WiFi is not configured. Starting provisioning mode..."));
        wifiManager->startProvisioningMode();
    }

    // 无论在哪种模式下，都启动Web服务器以提供UI
    // 后续可以通过isConnected()等方法在UI上显示具体状态
    Serial.println(F("\nStarting Web Server..."));
    Sys_WebServer* webServer = Sys_WebServer::getInstance();
    webServer->begin();

    // 启动后台任务
    setupTasks(webServer->getWebSocket());
    
    // IP地址的获取和显示应该在一个后台任务中进行，因为连接是非阻塞的
    Serial.println(F("Web server started. IP address will be available once connected."));
}
