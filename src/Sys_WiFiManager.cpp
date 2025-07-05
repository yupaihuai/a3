#include "Sys_WiFiManager.h"
#include <esp_log.h>

static const char* TAG = "WiFiManager";

// 初始化静态实例指针
Sys_WiFiManager* Sys_WiFiManager::_instance = nullptr;

// 私有构造函数
Sys_WiFiManager::Sys_WiFiManager() {
    // 构造函数中可以不执行任何操作，所有初始化在begin()中进行
}

// 获取单例实例
Sys_WiFiManager* Sys_WiFiManager::getInstance() {
    if (_instance == nullptr) {
        _instance = new Sys_WiFiManager();
    }
    return _instance;
}

// 初始化WiFi管理器
void Sys_WiFiManager::begin() {
    ESP_LOGI(TAG, "Initializing WiFi Manager...");
    _currentSettings = Sys_SettingsManager::getInstance()->loadWiFiSettings();

    // 根据加载的模式设置WiFi
    WiFi.mode((wifi_mode_t)_currentSettings.mode);

    if (_currentSettings.mode == WIFI_AP || _currentSettings.mode == WIFI_AP_STA) {
        // 为AP模式使用一个固定的、有意义的SSID，而不是用户配置的STA SSID
        const char* ap_ssid = "ESP32S3-Config";
        ESP_LOGI(TAG, "Starting Access Point with fixed SSID: %s", ap_ssid);
        WiFi.softAP(ap_ssid, NULL); // 启动一个开放的配置网络
        ESP_LOGI(TAG, "AP IP address: %s", WiFi.softAPIP().toString().c_str());
    }

    if (_currentSettings.mode == WIFI_STA || _currentSettings.mode == WIFI_AP_STA) {
        if (_currentSettings.ssid.length() > 0) {
            ESP_LOGI(TAG, "Attempting to connect to Station with SSID: %s", _currentSettings.ssid.c_str());
            
            if (_currentSettings.staticIP) {
                IPAddress ip, gateway, subnet;
                ip.fromString(_currentSettings.ip);
                gateway.fromString(_currentSettings.gateway);
                subnet.fromString(_currentSettings.subnet);
                WiFi.config(ip, gateway, subnet);
                ESP_LOGI(TAG, "Using static IP configuration for STA.");
            }

            WiFi.begin(_currentSettings.ssid.c_str(), _currentSettings.password.c_str());

            // 等待连接，这里可以添加超时逻辑
            int retries = 0;
            while (WiFi.status() != WL_CONNECTED && retries < 20) {
                delay(500);
                Serial.print(".");
                retries++;
            }

            if(WiFi.status() == WL_CONNECTED) {
                ESP_LOGI(TAG, "STA Mode Connected. IP Address: %s", WiFi.localIP().toString().c_str());
            } else {
                ESP_LOGW(TAG, "STA Mode connection failed. Continuing with AP mode if enabled.");
                // 如果STA连接失败，在AP+STA模式下，AP仍然是可用的。
                // 我们不需要做任何事，只需确保 getIPAddress() 能返回正确的IP。
            }

        } else {
            ESP_LOGW(TAG, "No SSID configured for STA mode, skipping STA connection.");
        }
    }
}

// 扫描可用WiFi网络
int Sys_WiFiManager::scanNetworks() {
    ESP_LOGI(TAG, "Starting WiFi network scan...");
    int n = WiFi.scanNetworks();
    ESP_LOGI(TAG, "Scan finished. Found %d networks.", n);
    return n;
}

// 保存新设置并重新连接WiFi
bool Sys_WiFiManager::saveSettingsAndReconnect(const WiFiSettings& newSettings) {
    ESP_LOGI(TAG, "Saving new WiFi settings and reconnecting...");
    Sys_SettingsManager::getInstance()->saveWiFiSettings(newSettings);
    
    ESP_LOGI(TAG, "Disconnecting WiFi...");
    WiFi.disconnect(true); // true to turn off radio
    delay(1000);

    // 使用新配置重新初始化
    begin();
    return true;
}

// 获取当前WiFi设置
WiFiSettings Sys_WiFiManager::getCurrentSettings() const {
    return _currentSettings;
}

// 获取当前连接状态
bool Sys_WiFiManager::isConnected() const {
    return WiFi.isConnected();
}

// 获取当前IP地址
String Sys_WiFiManager::getIPAddress() const {
    // 在AP+STA模式下，如果STA已连接，优先返回STA的IP
    if (WiFi.getMode() == WIFI_AP_STA && WiFi.isConnected()) {
        return WiFi.localIP().toString();
    }
    // 如果是纯AP模式，或者AP+STA但STA未连接，返回AP的IP
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        return WiFi.softAPIP().toString();
    }
    // 如果是纯STA模式
    if (WiFi.getMode() == WIFI_STA && WiFi.isConnected()) {
        return WiFi.localIP().toString();
    }
    // 其他情况（如未连接的STA）
    return "0.0.0.0";
}