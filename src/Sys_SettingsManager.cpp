#include "Sys_SettingsManager.h"
#include <esp_log.h>

static const char* TAG = "SettingsManager";

// 初始化静态实例指针
Sys_SettingsManager* Sys_SettingsManager::_instance = nullptr;

// 私有构造函数
Sys_SettingsManager::Sys_SettingsManager() {
    // 在构造函数中打开NVS命名空间
    // "wifi-config" 是存储设置的命名空间
    // false 表示以读/写模式打开
    if (!_preferences.begin("wifi-config", false)) {
        ESP_LOGE(TAG, "Failed to open 'wifi-config' namespace in NVS!");
    } else {
        ESP_LOGI(TAG, "NVS namespace 'wifi-config' opened successfully.");
    }
}

// 获取单例实例
Sys_SettingsManager* Sys_SettingsManager::getInstance() {
    if (_instance == nullptr) {
        _instance = new Sys_SettingsManager();
    }
    return _instance;
}

// 保存WiFi设置到NVS
bool Sys_SettingsManager::saveWiFiSettings(const WiFiSettings& settings) {
    ESP_LOGI(TAG, "Saving WiFi settings to NVS...");
    _preferences.putString("ssid", settings.ssid);
    _preferences.putString("password", settings.password);
    _preferences.putInt("mode", settings.mode);
    _preferences.putBool("static_ip", settings.staticIP);
    _preferences.putString("ip", settings.ip);
    _preferences.putString("subnet", settings.subnet);
    _preferences.putString("gateway", settings.gateway);
    ESP_LOGI(TAG, "WiFi settings saved for SSID: %s", settings.ssid.c_str());
    return true; // 在Preferences库中，put操作通常是即时的，这里简化处理
}

// 从NVS加载WiFi设置
WiFiSettings Sys_SettingsManager::loadWiFiSettings() {
    WiFiSettings settings;
    ESP_LOGI(TAG, "Loading WiFi settings from NVS...");
    
    // getString的第二个参数是当键不存在时的默认值
    // 当键不存在时，返回一个空字符串 ""
    settings.ssid = _preferences.getString("ssid", "");
    settings.password = _preferences.getString("password", "");
    settings.mode = _preferences.getInt("mode", 3); // 默认为 AP+STA 模式
    settings.staticIP = _preferences.getBool("static_ip", false);
    settings.ip = _preferences.getString("ip", "");
    settings.subnet = _preferences.getString("subnet", "");
    settings.gateway = _preferences.getString("gateway", "");

    if (settings.ssid.length() > 0) {
        ESP_LOGI(TAG, "Loaded WiFi settings. SSID: '%s'", settings.ssid.c_str());
    } else {
        ESP_LOGW(TAG, "No WiFi settings found in NVS.");
    }
    return settings;
}

// 检查WiFi是否已配置
bool Sys_SettingsManager::isWiFiConfigured() {
    // 逻辑很简单：如果NVS中存在一个非空的SSID，就认为已经配置过了
    return _preferences.getString("ssid", "").length() > 0;
}

// 擦除WiFi设置
bool Sys_SettingsManager::eraseWiFiSettings() {
    ESP_LOGW(TAG, "Erasing all keys in 'wifi-config' namespace...");
    return _preferences.clear();
}