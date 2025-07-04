#include "Sys_Tasks.h"
#include <ArduinoJson.h>
#include <esp_log.h>
#include "Sys_MemoryManager.h" // 引入内存管理器以获取准确的内存信息

static const char* TAG = "Sys_Tasks";

/**
 * @brief 处理来自WebSocket的“获取系统状态”命令。
 */
void handleSystemStatusCommand(AsyncWebSocketClient *client, AsyncWebSocket* ws) {
    if (!ws) {
        ESP_LOGE(TAG, "WebSocket instance is null!");
        return;
    }

    ESP_LOGI(TAG, "Handling get_system_status command from client #%u", client->id());

    // 使用ArduinoJson创建JSON文档
    JsonDocument doc;
    // 获取内存管理器实例
    Sys_MemoryManager* memManager = Sys_MemoryManager::getInstance();

    doc["type"] = "system_status";
    JsonObject data = doc["data"].to<JsonObject>();
    data["uptime"] = millis() / 1000;
    
    // 添加通用堆内存信息
    data["heap_free"] = ESP.getFreeHeap();
    data["heap_total"] = ESP.getHeapSize();

    // 添加通过内存管理器获取的、更准确的PSRAM池信息
    data["psram_pool_free"] = memManager->getFreePoolSize();
    data["psram_pool_total"] = memManager->getTotalPoolSize();

    // 序列化JSON为字符串
    String output;
    serializeJson(doc, output);

    // 通过WebSocket广播JSON字符串
    // 使用 textAll() 将确保所有打开的页面都能同步状态
    ws->textAll(output); 
    
    ESP_LOGD(TAG, "Broadcasted system status: %s", output.c_str());
}