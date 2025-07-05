#ifndef SYS_WIFI_MANAGER_H
#define SYS_WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "Sys_SettingsManager.h" // 依赖设置管理器来加载和保存配置

class Sys_WiFiManager {
private:
    static Sys_WiFiManager* _instance;
    WiFiSettings _currentSettings;

    // 私有构造函数，实现单例模式
    Sys_WiFiManager();

public:
    // 禁止拷贝和赋值
    Sys_WiFiManager(const Sys_WiFiManager&) = delete;
    Sys_WiFiManager& operator=(const Sys_WiFiManager&) = delete;

    // 获取单例实例
    static Sys_WiFiManager* getInstance();

    // 初始化WiFi管理器，加载配置并连接
    void begin();

    // 启动配网模式 (强制开启一个AP)
    void startProvisioningMode();

    // 扫描可用WiFi网络
    // 返回扫描到的网络数量，扫描结果通过WiFi.SSID(i)等获取
    int scanNetworks();

    // 保存新设置并重新连接WiFi
    bool saveSettingsAndReconnect(const WiFiSettings& newSettings);

    // 获取当前WiFi设置
    WiFiSettings getCurrentSettings() const;

    // 获取当前连接状态
    bool isConnected() const;

    // 获取当前IP地址
    String getIPAddress() const;
};

#endif // SYS_WIFI_MANAGER_H