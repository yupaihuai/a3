#include <Arduino.h>
#include "esp_spi_flash.h" // 用于获取 Flash 芯片信息
#include "esp32/spiram.h"  // 用于获取 PSRAM 信息
#include "esp_partition.h" // 用于遍历分区表

// 函数声明，方便在loop中调用
void printDiagnostics();

void setup() {
  Serial.begin(115200);

  delay(1000); // 短暂的延迟确保初始化完成，等待串口监视器连接
  Serial.println("\n\nBooting up... System is starting.");
}

void loop() {
  // 每隔5秒打印一次完整的诊断信息
  printDiagnostics();
  delay(5000); 
}
void printDiagnostics() {
  Serial.println("\n\n=============================================");
  Serial.println(" ESP32-S3 Hardware & Configuration Check");
  Serial.println(" ESP32-S3 硬件 & 配置 检查");
  Serial.println("=============================================");

  // -----------------------------------------------------------------
  // 1. 验证 Flash 和 PSRAM (对应 board_build.arduino.memory_type)
  // -----------------------------------------------------------------
  Serial.println("\n--- 1. Flash and PSRAM Verification / 验证---");

  // 获取 Flash 芯片大小 (来自芯片硬件)
  uint32_t flash_size_bytes = spi_flash_get_chip_size();
  Serial.print("Flash 芯片大小 (由硬件检测): ");
  Serial.print(flash_size_bytes / (1024 * 1024));
  Serial.println(" MB");

  // 获取 PSRAM 芯片大小 (来自芯片硬件)
  // <<<<<<<<<<<<  FIXED: 使用新的函数名
  uint32_t psram_size_bytes = esp_spiram_get_size(); 
  if (psram_size_bytes > 0) {
    Serial.print("PSRAM 大小 (由硬件检测): ");
    Serial.print(psram_size_bytes / (1024 * 1024));
    Serial.println(" MB");
  } else {
    Serial.println("PSRAM: 未检测到或未启用!");
  }

  // 检查编译时启用的 PSRAM 类型 (验证配置是否生效)
  #if CONFIG_SPIRAM_MODE_OCT
    Serial.println("PSRAM Mode (Compiled for): OPI (Octal)");
    Serial.println("PSRAM 模式 (已编译): OPI (Octal 八线程)");
  #elif CONFIG_SPIRAM_MODE_QUAD
    Serial.println("PSRAM Mode (Compiled for): QPI (Quad)");
    Serial.println("PSRAM 模式 (已编译): QPI (Quad 四线程)");
  #else
    Serial.println("PSRAM Mode (Compiled for): PSRAM not enabled in config!");
    Serial.println("PSRAM 模式 (已编译): PSRAM 配置未启用!");
  #endif

  // 检查可用的堆内存 (Heap)，启用PSRAM后，这里会包含PSRAM空间
  uint32_t free_heap = ESP.getFreeHeap();
  uint32_t total_heap = ESP.getHeapSize();
  Serial.print("Heap Memory: ");
  Serial.print(free_heap / 1024);
  Serial.print(" KB Free / ");
  Serial.print(total_heap / 1024);
  Serial.println(" KB Total");
  if (total_heap > 512 * 1024) { // ESP32-S3 内部SRAM约512KB，超过则说明PSRAM已加入Heap
      Serial.println("-> PSRAM is successfully integrated into Heap memory. Great!");
      Serial.println("-> PSRAM 已集成到堆内存!");
  } else {
      Serial.println("-> Warning: PSRAM does not seem to be integrated into Heap. Check your config!");
      Serial.println("-> 警告: PSRAM 没有集成到堆内存中. 检查你的配置!");
  }


  // -----------------------------------------------------------------
  // 2. 验证分区表 (对应 board_build.partitions = my_8MB.csv)
  // -----------------------------------------------------------------
  Serial.println("\n--- 2. Partition Table Verification / 分区表校验---");
  Serial.println("Type      | Subtype   | Address    | Size (bytes) | Label");
  Serial.println("----------------------------------------------------------------");

  esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
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
        default: subtype_str = "unknown"; break;
    }

    // 格式化输出，使其对齐
    char line[128];
    sprintf(line, "%-10s| %-9s | 0x%08X | %-12u | %s",
            (part->type == ESP_PARTITION_TYPE_APP) ? "app" : "data",
            subtype_str,
            part->address,
            part->size,
            part->label);
    Serial.println(line);
    
    it = esp_partition_next(it);
  }
  esp_partition_iterator_release(it);
  
  Serial.println("\n-> Compare this table with your 'my_8MB.csv' file. They should match!");
  Serial.println("\n-> 将此表与“my_8MB.csv”文件进行比较。它们应该匹配！");

  Serial.println("\n=============================================");
  Serial.println("      Verification script finished.");
  Serial.println("      验证脚本完成.");
  Serial.println("=============================================");
}