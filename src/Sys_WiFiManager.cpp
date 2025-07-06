#include "Sys_WiFiManager.h"
#include <esp_log.h>

static const char* TAG = "WiFiManager";

// 初始化静态实例指针
Sys_WiFiManager* Sys_WiFiManager::_instance = nullptr;

// 私有构造函数
Sys_WiFiManager::Sys_WiFiManager() : _reconnectTimer(0), _lastStatus(WL_IDLE_STATUS), _reconnecting(false) {
    // 初始化计时器、最后状态和重连标志
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
    ESP_LOGI(TAG, "Initializing WiFi Manager (non-blocking)...");
    _currentSettings = Sys_SettingsManager::getInstance()->loadWiFiSettings();
    _applyAndConnect();
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
    ESP_LOGI(TAG, "Saving new WiFi settings and preparing to reconnect...");
    Sys_SettingsManager::getInstance()->saveWiFiSettings(newSettings);
    _currentSettings = newSettings; // 更新内存中的当前设置
    
    ESP_LOGI(TAG, "Disconnecting WiFi to apply new settings...");
    _reconnecting = true; // 设置重连标志
    WiFi.disconnect(true, true); // Disconnect, turn off radio, and erase SDK config
    
    // 实际的重连操作将由 update() 函数在检测到断开连接后处理
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

// 周期性更新函数，由后台任务调用
void Sys_WiFiManager::update() {
    wl_status_t currentStatus = WiFi.status();

    // 1. 处理保存设置后的重连逻辑
    if (_reconnecting && currentStatus == WL_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi is fully disconnected. Applying new settings and reconnecting...");
        _applyAndConnect();
        _reconnecting = false; // 清除重连标志
        _lastStatus = WiFi.status(); // 更新状态
        return; // 本次更新周期完成
    }

    // 如果正在重连中，则暂时不执行后续的自动重连逻辑
    if (_reconnecting) {
        return;
    }

    // 2. 只在STA或AP+STA模式下处理自动重连逻辑
    if (WiFi.getMode() != WIFI_STA && WiFi.getMode() != WIFI_AP_STA) {
        return;
    }

    // 3. 如果状态发生变化，则打印日志
    if (currentStatus != _lastStatus) {
        switch (currentStatus) {
            case WL_CONNECTED:
                ESP_LOGI(TAG, "WiFi Connected! IP: %s", WiFi.localIP().toString().c_str());
                break;
            case WL_NO_SSID_AVAIL:
                ESP_LOGW(TAG, "WiFi connection failed: SSID not found.");
                break;
            case WL_CONNECT_FAILED:
                ESP_LOGW(TAG, "WiFi connection failed: Authentication error.");
                break;
            case WL_DISCONNECTED:
                ESP_LOGW(TAG, "WiFi disconnected. Will attempt to reconnect...");
                _reconnectTimer = millis(); // 立即开始重连计时
                break;
            default:
                ESP_LOGD(TAG, "WiFi status changed to: %d", currentStatus);
                break;
        }
        _lastStatus = currentStatus;
    }

    // 4. 如果未连接，并且距离上次尝试已超过10秒，则尝试重连
    if (currentStatus != WL_CONNECTED && millis() - _reconnectTimer > 10000) {
        ESP_LOGI(TAG, "Attempting to reconnect to WiFi...");
        WiFi.reconnect();
        _reconnectTimer = millis(); // 重置计时器
    }
}

// 私有方法：应用当前设置并发起连接
void Sys_WiFiManager::_applyAndConnect() {
    // 如果没有有效的SSID配置，则不执行任何操作
    if (_currentSettings.ssid.length() == 0) {
        ESP_LOGW(TAG, "No WiFi configuration found. Aborting connection attempt.");
        return;
    }

    // 根据加载的模式设置WiFi
    WiFi.mode((wifi_mode_t)_currentSettings.mode);

    if (_currentSettings.mode == WIFI_AP || _currentSettings.mode == WIFI_AP_STA) {
        const char* ap_ssid = "ESP32S3-Config";
        const char* ap_password = "12345678";
        ESP_LOGI(TAG, "Starting Access Point with fixed SSID: %s", ap_ssid);
        WiFi.softAP(ap_ssid, ap_password);
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
            } else {
                // 确保在DHCP模式下清除静态IP配置
                esp_netif_dhcpc_start(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"));
            }
            
            WiFi.begin(_currentSettings.ssid.c_str(), _currentSettings.password.c_str());
            _reconnectTimer = millis(); // 初始化重连计时器
        } else {
            ESP_LOGW(TAG, "No SSID configured for STA mode, skipping STA connection.");
        }
    }
}