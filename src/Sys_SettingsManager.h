#ifndef SYS_SETTINGS_MANAGER_H
#define SYS_SETTINGS_MANAGER_H

#include <Arduino.h>
#include <Preferences.h> // 使用Preferences库简化NVS操作

// 定义一个结构体来统一管理所有WiFi相关的设置
struct WiFiSettings {
    String ssid;
    String password;
    int mode; // 1=STA, 2=AP, 3=AP_STA
    bool staticIP;
    String ip;
    String subnet;
    String gateway;
};

class Sys_SettingsManager {
private:
    static Sys_SettingsManager* _instance;
    Preferences _preferences;

    // 私有构造函数，实现单例模式
    Sys_SettingsManager();

public:
    // 禁止拷贝和赋值
    Sys_SettingsManager(const Sys_SettingsManager&) = delete;
    Sys_SettingsManager& operator=(const Sys_SettingsManager&) = delete;

    // 获取单例实例
    static Sys_SettingsManager* getInstance();

    // 保存WiFi设置到NVS
    bool saveWiFiSettings(const WiFiSettings& settings);

    // 从NVS加载WiFi设置
    WiFiSettings loadWiFiSettings();

    // 检查WiFi是否已配置 (即，SSID是否已保存)
    bool isWiFiConfigured();

    // （未来扩展）保存其他设置...
    // bool saveMqttSettings(const MqttSettings& settings);
};

#endif // SYS_SETTINGS_MANAGER_H