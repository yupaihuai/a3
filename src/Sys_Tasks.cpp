#include "Sys_Tasks.h"
#include "Sys_WiFiManager.h"
#include "Sys_MemoryManager.h"
#include <ArduinoJson.h>
#include <esp_log.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h> // 包含主头文件，确保所有依赖都已定义
#include "Sys_WebServer.h"     // 引入Web服务器管理器以调用cleanup

static const char* TAG = "Sys_Tasks";

// 定义全局队列句柄
QueueHandle_t xCommandQueue;
QueueHandle_t xStateQueue;
QueueHandle_t xBarcodeQueue; // 暂时未使用，但根据设计文档预留

// 静态WebSocket实例指针
static AsyncWebSocket* _ws;

// 工人任务实现 (Task_Worker)
void taskWorker(void *pvParameters) {
    ESP_LOGI(TAG, "Task_Worker started on core %d", xPortGetCoreID());
    CommandMessage receivedCmd;
    Sys_SettingsManager* settingsManager = Sys_SettingsManager::getInstance();
    Sys_WiFiManager* wifiManager = Sys_WiFiManager::getInstance();

    for (;;) {
        if (xQueueReceive(xCommandQueue, &receivedCmd, portMAX_DELAY) == pdPASS) {
            switch (receivedCmd.command) {
                case CMD_GET_SYSTEM_STATUS: {
                    ESP_LOGI(TAG, "Task_Worker: Received CMD_GET_SYSTEM_STATUS (On-demand)");
                    JsonDocument doc;
                    doc["type"] = "system_status";
                    JsonObject data = doc["data"].to<JsonObject>();
                    
                    // 采集详细系统信息
                    esp_chip_info_t chip_info;
                    esp_chip_info(&chip_info);
                    data["chip_model"] = "ESP32-S3"; // 简化，实际可根据chip_info.model
                    data["cpu_freq"] = ESP.getCpuFreqMHz();
                    data["flash_size_bytes"] = spi_flash_get_chip_size();
                    data["psram_size_bytes"] = esp_spiram_get_size();
                    data["idf_version"] = esp_get_idf_version();

                    data["uptime"] = millis() / 1000;
                    data["heap_free"] = ESP.getFreeHeap();
                    data["heap_total"] = ESP.getHeapSize();
                    data["psram_pool_free"] = Sys_MemoryManager::getInstance()->getFreePoolSize();
                    data["psram_pool_total"] = Sys_MemoryManager::getInstance()->getTotalPoolSize();
                    data["wifi_ip"] = wifiManager->getIPAddress();
                    data["wifi_connected"] = wifiManager->isConnected();
                    
                    String output;
                    serializeJson(doc, output);
                    _ws->textAll(output); // 广播给所有连接的客户端
                    break;
                }
                case CMD_SCAN_WIFI: {
                    ESP_LOGI(TAG, "Task_Worker: Received CMD_SCAN_WIFI");
                    int n = wifiManager->scanNetworks();
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
                    _ws->textAll(output); // 广播给所有连接的客户端
                    ESP_LOGI(TAG, "Task_Worker: Sent WiFi scan results.");
                    break;
                }
                case CMD_GET_WIFI_SETTINGS: {
                    ESP_LOGI(TAG, "Task_Worker: Received CMD_GET_WIFI_SETTINGS");
                    WiFiSettings currentSettings = settingsManager->loadWiFiSettings();
                    JsonDocument doc;
                    doc["type"] = "wifi_settings";
                    JsonObject data = doc["data"].to<JsonObject>();
                    data["ssid"] = currentSettings.ssid;
                    data["password"] = currentSettings.password;
                    data["mode"] = currentSettings.mode;
                    data["static_ip"] = currentSettings.staticIP;
                    data["ip"] = currentSettings.ip;
                    data["subnet"] = currentSettings.subnet;
                    data["gateway"] = currentSettings.gateway;
                    String output;
                    serializeJson(doc, output);
                    _ws->textAll(output); // 广播给所有连接的客户端
                    ESP_LOGI(TAG, "Task_Worker: Sent WiFi settings.");
                    break;
                }
                case CMD_SAVE_WIFI_SETTINGS: {
                    ESP_LOGI(TAG, "Task_Worker: Received CMD_SAVE_WIFI_SETTINGS");
                    JsonDocument doc;
                    DeserializationError error = deserializeJson(doc, receivedCmd.payload);
                    if (error) {
                        ESP_LOGE(TAG, "Task_Worker: Failed to parse WiFi settings JSON: %s", error.c_str());
                        _ws->textAll("{\"type\":\"toast\",\"data\":{\"message\":\"保存WiFi设置失败：JSON解析错误\",\"level\":\"danger\"}}");
                        break;
                    }
                    WiFiSettings newSettings;
                    newSettings.ssid = doc["ssid"].as<String>();
                    newSettings.password = doc["password"].as<String>();
                    newSettings.mode = doc["mode"].as<int>();
                    newSettings.staticIP = doc["static_ip"] | false; // 默认false
                    newSettings.ip = doc["ip"] | "";
                    newSettings.subnet = doc["subnet"] | "";
                    newSettings.gateway = doc["gateway"] | "";

                    if (wifiManager->saveSettingsAndReconnect(newSettings)) {
                        ESP_LOGI(TAG, "Task_Worker: WiFi settings saved and reconnected successfully.");
                        _ws->textAll("{\"type\":\"toast\",\"data\":{\"message\":\"WiFi设置已保存并应用！\",\"level\":\"success\"}}");
                        // 成功保存后，广播最新的WiFi设置给所有客户端
                        CommandMessage getSettingsCmd;
                        getSettingsCmd.command = CMD_GET_WIFI_SETTINGS;
                        xQueueSend(xCommandQueue, &getSettingsCmd, 0); // 立即请求更新UI
                    } else {
                        ESP_LOGE(TAG, "Task_Worker: Failed to save WiFi settings or reconnect.");
                        _ws->textAll("{\"type\":\"toast\",\"data\":{\"message\":\"保存WiFi设置失败！\",\"level\":\"danger\"}}");
                    }
                    break;
                }
                case CMD_REBOOT: {
                    ESP_LOGI(TAG, "Task_Worker: Received CMD_REBOOT");
                    // 先发消息，再清理网络，最后重启
                    _ws->textAll("{\"type\":\"toast\",\"data\":{\"message\":\"设备即将重启...\",\"level\":\"info\"}}");
                    vTaskDelay(pdMS_TO_TICKS(100)); // 短暂延时确保消息发出
                    Sys_WebServer::getInstance()->cleanup();
                    vTaskDelay(pdMS_TO_TICKS(100)); // 短暂延时等待网络资源释放
                    ESP.restart();
                    break;
                }
                case CMD_FACTORY_RESET: {
                    ESP_LOGW(TAG, "Task_Worker: Received CMD_FACTORY_RESET. Erasing NVS 'wifi-config' namespace...");
                    bool success = settingsManager->eraseWiFiSettings();
                    if (success) {
                         _ws->textAll("{\"type\":\"toast\",\"data\":{\"message\":\"恢复出厂设置成功！设备即将重启...\",\"level\":\"success\"}}");
                    } else {
                         _ws->textAll("{\"type\":\"toast\",\"data\":{\"message\":\"恢复出厂设置失败！设备即将重启...\",\"level\":\"danger\"}}");
                    }
                    vTaskDelay(pdMS_TO_TICKS(100)); // 短暂延时确保消息发出
                    Sys_WebServer::getInstance()->cleanup();
                    vTaskDelay(pdMS_TO_TICKS(100)); // 短暂延时等待网络资源释放
                    ESP.restart();
                    break;
                }
                default:
                    ESP_LOGW(TAG, "Task_Worker: Received unknown command: %d", receivedCmd.command);
                    _ws->textAll("{\"type\":\"toast\",\"data\":{\"message\":\"未知命令！\",\"level\":\"warning\"}}");
                    break;
            }
        }
    }
}

// WebSocket数据推送任务 (Task_WebSocketPusher)
void taskWebSocketPusher(void *pvParameters) {
    ESP_LOGI(TAG, "Task_WebSocketPusher started on core %d", xPortGetCoreID());
    // Task_WebSocketPusher现在主要负责从xStateQueue接收状态更新并推送
    // 也可以监听其他队列，例如xBarcodeQueue
    CommandMessage msg;
    for (;;) {
        if (xQueueReceive(xStateQueue, &msg, portMAX_DELAY) == pdPASS) {
            // 假设从xStateQueue接收到的payload已经是完整的JSON字符串
            if (_ws && _ws->count() > 0) {
                _ws->textAll(msg.payload);
                ESP_LOGD(TAG, "Pusher: Sent state update: %s", msg.payload);
            }
        }
    }
}

// 系统状态监控任务 (Task_SystemMonitor)
void taskSystemMonitor(void *pvParameters) {
    ESP_LOGI(TAG, "Task_SystemMonitor started on core %d", xPortGetCoreID());
    for (;;) {
        // 周期性调用WiFi管理器的update函数，处理连接和重连逻辑
        Sys_WiFiManager::getInstance()->update();

        // 其他周期性监控任务可以放在这里
        // ...

        // 任务延迟，控制执行频率
        vTaskDelay(pdMS_TO_TICKS(500)); // 每500ms检查一次
    }
}

// 初始化任务和队列
void setupTasks(AsyncWebSocket* wsInstance) {
    _ws = wsInstance; // 保存WebSocket实例

    // 创建命令队列
    xCommandQueue = xQueueCreate(5, sizeof(CommandMessage)); // 减小队列深度，避免过多积压
    if (xCommandQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create command queue!");
        return;
    }

    // 创建状态队列 (用于Task_SystemMonitor向Task_WebSocketPusher发送数据，如果Task_SystemMonitor有周期性任务)
    // 考虑到按需采集，xStateQueue可能暂时不直接用于系统状态推送，但保留其定义以备未来扩展
    xStateQueue = xQueueCreate(5, sizeof(CommandMessage));
    if (xStateQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create state queue!");
        return;
    }

    // 创建条码队列 (暂时不创建，如果需要再创建)
    // xBarcodeQueue = xQueueCreate(5, sizeof(CommandMessage));

    // 创建工人任务 (Task_Worker)，固定到Core 1，优先级低
    xTaskCreatePinnedToCore(
        taskWorker,
        "Task_Worker",
        8192, // 增加堆栈大小，因为处理JSON和WiFi操作可能需要更多内存
        NULL,
        1,    // 优先级 (低)
        NULL,
        1     // 固定到Core 1
    );

    // 创建WebSocket推送任务 (Task_WebSocketPusher)，固定到Core 1，优先级中等
    xTaskCreatePinnedToCore(
        taskWebSocketPusher,
        "Task_WebSocketPusher",
        4096, // 堆栈大小
        NULL,
        2,    // 优先级 (中等，高于Worker)
        NULL,
        1     // 固定到Core 1
    );

    // 创建系统状态监控任务 (Task_SystemMonitor)，固定到Core 1，优先级低
    // 此任务现在主要用于保持活跃，不进行周期性数据采集
    xTaskCreatePinnedToCore(
        taskSystemMonitor,
        "Task_SystemMonitor",
        3072, // 堆栈大小
        NULL,
        1,    // 优先级 (低，与Worker相同或略低)
        NULL,
        1     // 固定到Core 1
    );

    ESP_LOGI(TAG, "All FreeRTOS tasks and queues created.");
}