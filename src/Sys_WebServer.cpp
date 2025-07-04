#include "Sys_WebServer.h"
#include <esp_log.h> // For ESP_LOGx macros
#include <WiFi.h>    // For WiFi.localIP()

static const char* TAG = "Sys_WebServer";

Sys_WebServer* Sys_WebServer::_instance = nullptr;

// 私有构造函数
Sys_WebServer::Sys_WebServer() {
    _server = new AsyncWebServer(80); // 在端口80上创建Web服务器
    ESP_LOGI(TAG, "Sys_WebServer instance created.");
}

// 获取单例实例
Sys_WebServer* Sys_WebServer::getInstance() {
    if (_instance == nullptr) {
        _instance = new Sys_WebServer();
    }
    return _instance;
}

// 处理根路径请求
void Sys_WebServer::handleRoot(AsyncWebServerRequest *request) {
    ESP_LOGI(TAG, "Handling root request from %s", request->client().remoteIP().toString().c_str());
    // 尝试从LittleFS提供index.html
    if (LittleFS.exists("/index.html")) {
        request->send(LittleFS, "/index.html", "text/html");
    } else {
        request->send(404, "text/plain", "index.html not found in LittleFS!");
    }
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
        request->send(response);
        ESP_LOGD(TAG, "Serving gzipped file: %s", pathGz.c_str());
    } else {
        // 如果没有.gz文件，尝试提供原始文件
        if (fs.exists(path)) {
            request->send(fs, path, request->contentType());
            ESP_LOGD(TAG, "Serving raw file: %s", path.c_str());
        } else {
            request->send(404, "text/plain", "File not found");
            ESP_LOGW(TAG, "File not found (raw or gzipped): %s", path.c_str());
        }
    }
}

// 初始化Web服务器
void Sys_WebServer::begin() {
    // 配置路由
    _server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        this->handleRoot(request);
    });

    // 配置静态文件服务
    // LittleFS 用于存放 HTML, JS, CSS等UI资源
    _server->serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    // FFat 用于存放WAV, JSON 等大文件
    _server->serveStatic("/media", FFat, "/");

    // 处理Gzip压缩的静态文件
    // 这是一个更通用的处理方式，可以根据文件扩展名自动添加Content-Encoding头
    _server->onNotFound([this](AsyncWebServerRequest *request){
        String path = request->url();
        // 移除查询参数
        int queryIndex = path.indexOf('?');
        if (queryIndex != -1) {
            path = path.substring(0, queryIndex);
        }

        // 尝试从LittleFS提供文件（包括Gzip）
        if (LittleFS.exists(path + ".gz") || LittleFS.exists(path)) {
            this->handleGzip(request, path, LittleFS);
        } 
        // 尝试从FFat提供文件（包括Gzip）
        else if (FFat.exists(path + ".gz") || FFat.exists(path)) {
            this->handleGzip(request, path, FFat);
        }
        else {
            this->handleNotFound(request);
        }
    });

    // 启动服务器
    _server->begin();
    ESP_LOGI(TAG, "Web Server started on IP: %s", WiFi.localIP().toString().c_str());
}

// 停止Web服务器
void Sys_WebServer::end() {
    if (_server) {
        _server->end();
        ESP_LOGI(TAG, "Web Server stopped.");
    }
}