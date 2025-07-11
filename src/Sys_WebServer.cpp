// Forcing recompile
#include "Sys_WebServer.h"
#include "Sys_Tasks.h"         // 包含任务处理模块
#include <ArduinoJson.h>       // 包含JSON库
#include <esp_log.h>           // For ESP_LOGx macros
#include <WiFi.h>              // For WiFi.localIP()

static const char* TAG = "Sys_WebServer";

Sys_WebServer* Sys_WebServer::_instance = nullptr;

// 私有构造函数
Sys_WebServer::Sys_WebServer() {
    _server = new AsyncWebServer(80); // 在端口80上创建Web服务器
    _ws = new AsyncWebSocket("/ws");    // 创建WebSocket服务器，并将其附加到/ws路径
    ESP_LOGI(TAG, "Sys_WebServer and WebSocket instances created.");
}

// 获取单例实例
Sys_WebServer* Sys_WebServer::getInstance() {
    if (_instance == nullptr) {
        _instance = new Sys_WebServer();
    }
    return _instance;
}

// 处理文件未找到
void Sys_WebServer::handleNotFound(AsyncWebServerRequest *request) {
    ESP_LOGW(TAG, "File not found: %s from %s", request->url().c_str(), request->client().remoteIP().toString().c_str());
    request->send(404, "text/plain", "Not Found");
}

// 处理Gzip压缩文件
void Sys_WebServer::handleGzip(AsyncWebServerRequest *request, const String& path, fs::FS& fs) {
    String pathGz = path + ".gz";
    if (fs.exists(pathGz)) {
        AsyncWebServerResponse *response = request->beginResponse(fs, pathGz, request->contentType());
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
        request->send(response);
        ESP_LOGD(TAG, "Serving gzipped file: %s", pathGz.c_str());
    } else {
        // 如果没有.gz文件，尝试提供原始文件
        if (fs.exists(path)) {
            AsyncWebServerResponse *response = request->beginResponse(fs, path, request->contentType());
            response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
            request->send(response);
            ESP_LOGD(TAG, "Serving raw file: %s", path.c_str());
        } else {
            request->send(404, "text/plain", "File not found");
            ESP_LOGW(TAG, "File not found (raw or gzipped): %s", path.c_str());
        }
    }
}

// WebSocket事件处理器
void Sys_WebServer::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            ESP_LOGI(TAG, "WebSocket client #%u connected from %s", client->id(), client->remoteIP().toString().c_str());
            // 移除主动发送的 "Welcome!" 消息，以确保所有通信都是JSON格式
            break;
        case WS_EVT_DISCONNECT:
            ESP_LOGI(TAG, "WebSocket client #%u disconnected", client->id());
            break;
        case WS_EVT_DATA: {
            AwsFrameInfo *info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                data[len] = 0; // Null-terminate
                ESP_LOGD(TAG, "WebSocket client #%u sent: %s", client->id(), (char*)data);

                // 解析收到的JSON命令
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, (char*)data);

                if (error) {
                    ESP_LOGE(TAG, "deserializeJson() failed: %s", error.c_str());
                    client->text("{\"error\":\"Invalid JSON\"}");
                    return;
                }

                const char* command = doc["command"];
                CommandMessage cmd;
                bool command_valid = true;

                if (strcmp(command, "get_system_status") == 0) {
                    cmd.command = CMD_GET_SYSTEM_STATUS;
                } else if (strcmp(command, "scan_wifi_networks") == 0) {
                    cmd.command = CMD_SCAN_WIFI;
                } else if (strcmp(command, "get_wifi_settings") == 0) {
                    cmd.command = CMD_GET_WIFI_SETTINGS;
                } else if (strcmp(command, "save_wifi_settings") == 0) {
                    cmd.command = CMD_SAVE_WIFI_SETTINGS;
                    // 将整个JSON payload复制到CommandMessage的payload字段
                    // 确保payload不会溢出
                    if (len < MAX_PAYLOAD_SIZE) {
                        memcpy(cmd.payload, data, len);
                        cmd.payload[len] = '\0'; // 确保字符串以null结尾
                    } else {
                        ESP_LOGE(TAG, "Received WiFi settings payload too large (%u bytes), max is %u.", len, MAX_PAYLOAD_SIZE);
                        client->text("{\"error\":\"Payload too large\"}");
                        command_valid = false;
                    }
                } else if (strcmp(command, "reboot") == 0) {
                    cmd.command = CMD_REBOOT;
                } else if (strcmp(command, "factory_reset") == 0) {
                    cmd.command = CMD_FACTORY_RESET;
                }
                else {
                    command_valid = false;
                    ESP_LOGW(TAG, "Unknown command received: %s", command);
                    client->text("{\"error\":\"Unknown command\"}");
                }

                if(command_valid) {
                    if (xQueueSend(xCommandQueue, &cmd, pdMS_TO_TICKS(100)) != pdPASS) {
                        ESP_LOGE(TAG, "Failed to post command to queue.");
                        client->text("{\"error\":\"Server busy, please try again.\"}");
                    }
                }
            }
            break;
        }
        case WS_EVT_PONG:
            ESP_LOGD(TAG, "WebSocket client #%u pong", client->id());
            break;
        case WS_EVT_ERROR:
            ESP_LOGE(TAG, "WebSocket client #%u error(%u): %s", client->id(), *((uint16_t*)arg), (char*)data);
            break;
    }
}

// 初始化Web服务器
void Sys_WebServer::begin() {
    // 绑定WebSocket事件
    _ws->onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
        this->onWsEvent(server, client, type, arg, data, len);
    });
    _server->addHandler(_ws); // 将WebSocket处理器添加到Web服务器

    // 配置静态文件服务
    // LittleFS 用于存放 HTML, JS, CSS等UI资源
    _server->serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    // FFat 用于存放WAV, JSON 等大文件
    _server->serveStatic("/media", FFat, "/");

    // 处理Gzip压缩的静态文件和404
    _server->onNotFound([this](AsyncWebServerRequest *request){
        String path = request->url();
        int queryIndex = path.indexOf('?');
        if (queryIndex != -1) {
            path = path.substring(0, queryIndex);
        }

        if (LittleFS.exists(path + ".gz") || LittleFS.exists(path)) {
            this->handleGzip(request, path, LittleFS);
        }
        else if (FFat.exists(path + ".gz") || FFat.exists(path)) {
            this->handleGzip(request, path, FFat);
        }
        else {
            this->handleNotFound(request);
        }
    });

    // 启动服务器
    _server->begin();
    ESP_LOGI(TAG, "Web Server with WebSocket started.");
    // IP地址的获取和显示将由Task_Worker在WiFi连接成功后推送
}

// 停止Web服务器
void Sys_WebServer::end() {
    if (_server) {
        _server->end();
        ESP_LOGI(TAG, "Web Server stopped.");
    }
}

AsyncWebSocket* Sys_WebServer::getWebSocket() {
    return _ws;
}

// 清理并关闭所有连接和服务器
void Sys_WebServer::cleanup() {
    ESP_LOGI(TAG, "Cleaning up WebServer and WebSocket...");
    if (_ws) {
        _ws->closeAll(); // 优雅地关闭所有WebSocket连接
    }
    if (_server) {
        _server->end(); // 停止Web服务器
    }
}