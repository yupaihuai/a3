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

    // 如果没有有效的SSID配置，则不执行任何操作
    if (_currentSettings.ssid.length() == 0) {
        ESP_LOGW(TAG, "No WiFi configuration found. Aborting WiFi.begin().");
        return;
    }

    // 根据加载的模式设置WiFi
    WiFi.mode((wifi_mode_t)_currentSettings.mode);

    if (_currentSettings.mode == WIFI_AP || _currentSettings.mode == WIFI_AP_STA) {
        // 为AP模式使用一个固定的、有意义的SSID，而不是用户配置的STA SSID
        const char* ap_ssid = "ESP32S3-Config";
        const char* ap_password = "12345678"; // 默认密码，可从NVS加载或在配网模式下设置
        ESP_LOGI(TAG, "Starting Access Point with fixed SSID: %s", ap_ssid);
        WiFi.softAP(ap_ssid, ap_password); // 启动一个带密码的配置网络
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

            // 阻塞等待连接，直到连接成功或超时
            int attempts = 0;
            const int MAX_ATTEMPTS = 30; // 最多等待30秒
            while (WiFi.status() != WL_CONNECTED && attempts < MAX_ATTEMPTS) {
                delay(1000); // 等待1秒
                ESP_LOGD(TAG, "Waiting for WiFi connection... (%d/%d)", ++attempts, MAX_ATTEMPTS);
            }

            if (WiFi.status() == WL_CONNECTED) {
                ESP_LOGI(TAG, "Connected to WiFi! IP: %s", WiFi.localIP().toString().c_str());
            } else {
                ESP_LOGW(TAG, "Failed to connect to WiFi after %d attempts.", MAX_ATTEMPTS);
            }
        } else {
            ESP_LOGW(TAG, "No SSID configured for STA mode, skipping STA connection.");
        }
    }
}

// 启动配网模式
void Sys_WiFiManager::startProvisioningMode() {
    const char* ap_ssid = "esp32s3"; // 与platformio.ini中的CONFIG_WIFI_PROV_SOFTAP_SSID一致
    const char* ap_password = "12345678"; // 与platformio.ini中的CONFIG_WIFI_PROV_SOFTAP_PASSWORD一致
    ESP_LOGI(TAG, "Starting Provisioning Mode AP with SSID: %s", ap_ssid);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_password); // 启动一个带密码的配置网络
    ESP_LOGI(TAG, "Provisioning AP IP address: %s", WiFi.softAPIP().toString().c_str());
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