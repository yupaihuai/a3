#ifndef SYS_WEB_SERVER_H
#define SYS_WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h> // 注意这个库的调用类名是：AsyncWebServer
#include <AsyncTCP.h>
#include <LittleFS.h> // 用于服务前端文件
#include <FFat.h>     // 用于服务大文件

class Sys_WebServer {
private:
    static Sys_WebServer* _instance;
    AsyncWebServer* _server; // 使用指针，在构造函数中初始化
    AsyncWebSocket* _ws;     // WebSocket服务器实例

    Sys_WebServer(); // 私有构造函数，实现单例模式

    // WebSocket事件处理器
    void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    // 处理文件未找到
    void handleNotFound(AsyncWebServerRequest *request);
    // 处理Gzip压缩文件
    void handleGzip(AsyncWebServerRequest *request, const String& path, fs::FS& fs);

public:
    // 获取单例实例
    static Sys_WebServer* getInstance();

    // 初始化Web服务器
    void begin();

    // 停止Web服务器
    void end();

    // 清理并关闭所有连接和服务器
    void cleanup();

    // 获取WebSocket实例
    AsyncWebSocket* getWebSocket();
};

#endif // SYS_WEB_SERVER_H