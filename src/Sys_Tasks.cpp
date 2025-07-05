#include "Sys_Tasks.h"
#include "Sys_WiFiManager.h"
#include "Sys_MemoryManager.h"
#include <ArduinoJson.h>
#include <esp_log.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h> // 包含主头文件，确保所有依赖都已定义

static const char* TAG = "Sys_Tasks";

// 定义全局命令队列句柄
QueueHandle_t xCommandQueue;

// 静态WebSocket实例指针
static AsyncWebSocket* _ws;

// 工人任务实现
void taskWorker(void *pvParameters) {
    ESP_LOGI(TAG, "Task Worker started on core %d", xPortGetCoreID());
    CommandMessage receivedCmd;

    for (;;) {
        // 阻塞等待命令队列中的新消息
        if (xQueueReceive(xCommandQueue, &receivedCmd, portMAX_DELAY) == pdPASS) {
            switch (receivedCmd.command) {
                case CMD_GET_SYSTEM_STATUS: {
                    ESP_LOGI(TAG, "Worker: Received CMD_GET_SYSTEM_STATUS");
                    JsonDocument doc;
                    doc["type"] = "system_status";
                    JsonObject data = doc["data"].to<JsonObject>();
                    data["uptime"] = millis() / 1000;
                    data["heap_free"] = ESP.getFreeHeap();
                    data["heap_total"] = ESP.getHeapSize();
                    data["psram_pool_free"] = Sys_MemoryManager::getInstance()->getFreePoolSize();
                    data["psram_pool_total"] = Sys_MemoryManager::getInstance()->getTotalPoolSize();
                    String output;
                    serializeJson(doc, output);
                    _ws->textAll(output);
                    break;
                }
                case CMD_SCAN_WIFI: {
                    ESP_LOGI(TAG, "Worker: Received CMD_SCAN_WIFI");
                    int n = Sys_WiFiManager::getInstance()->scanNetworks();
                    JsonDocument doc;
                    doc["type"] = "wifi_scan_result";
                    JsonArray networks = doc["data"].to<JsonArray>();
                    for (int i = 0; i < n; ++i) {
                        JsonObject net = networks.add<JsonObject>();
                        net["ssid"] = WiFi.SSID(i);
                        net["rssi"] = WiFi.RSSI(i);
                        net["encryption"] = WiFi.encryptionType(i);
                    }
                    String output;
                    serializeJson(doc, output);
                    // 注意：扫描结果只发给请求的客户端比较合理，但目前我们没有传递client id
                    // 为了简单起见，暂时广播给所有客户端
                    _ws->textAll(output);
                    ESP_LOGI(TAG, "Worker: Sent WiFi scan results.");
                    break;
                }
                default:
                    ESP_LOGW(TAG, "Worker: Received unknown command: %d", receivedCmd.command);
                    break;
            }
        }
    }
}

// 初始化任务和队列
void setupTasks(AsyncWebSocket* wsInstance) {
    _ws = wsInstance; // 保存WebSocket实例

    // 创建命令队列，可以容纳10个命令
    xCommandQueue = xQueueCreate(10, sizeof(CommandMessage));
    if (xCommandQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create command queue!");
        return;
    }

    // 创建工人任务，并固定到Core 1，低优先级
    xTaskCreatePinnedToCore(
        taskWorker,
        "WorkerTask",
        4096, // 堆栈大小
        NULL,
        1,    // 优先级
        NULL,
        1     // 固定到Core 1
    );

    ESP_LOGI(TAG, "Worker task created and started.");
}