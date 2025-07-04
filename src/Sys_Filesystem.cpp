#include "Sys_Filesystem.h"
#include <esp_log.h> // For ESP_LOGx macros

static const char* TAG = "Sys_Filesystem";

Sys_Filesystem* Sys_Filesystem::_instance = nullptr;

// 私有构造函数
Sys_Filesystem::Sys_Filesystem() {
    ESP_LOGI(TAG, "Sys_Filesystem instance created.");
}

// 获取单例实例
Sys_Filesystem* Sys_Filesystem::getInstance() {
    if (_instance == nullptr) {
        _instance = new Sys_Filesystem();
    }
    return _instance;
}

// 初始化所有文件系统
bool Sys_Filesystem::setupFilesystems() {
    bool success = true;

    // 初始化 LittleFS
    ESP_LOGI(TAG, "Initializing LittleFS...");
    // 使用 format on fail 的方式来初始化文件系统
    if (!LittleFS.begin(false, "/littlefs", 10, "littlefs")) {
        ESP_LOGE(TAG, "LittleFS Mount Failed! Attempting to format...");
        if (LittleFS.format()) { // 直接使用 LittleFS 对象
            ESP_LOGI(TAG, "LittleFS Formatted Successfully! Remounting...");
            if (!LittleFS.begin(false, "/littlefs", 10, "littlefs")) {
                ESP_LOGE(TAG, "LittleFS Remount Failed after format!");
                success = false;
            } else {
                ESP_LOGI(TAG, "LittleFS Remounted Successfully after format.");
            }
        } else {
            ESP_LOGE(TAG, "LittleFS Format Failed!");
            success = false;
        }
    } else {
        ESP_LOGI(TAG, "LittleFS Mounted Successfully.");
    }

    // 初始化 FFat
    ESP_LOGI(TAG, "Initializing FFat...");
    // FFat 同样使用 format on fail
    if (!FFat.begin(false, "/ffat", 10, "ffat")) {
        ESP_LOGE(TAG, "FFat Mount Failed! Attempting to format...");
        if (FFat.format()) { // 直接使用 FFat 对象
            ESP_LOGI(TAG, "FFat Formatted Successfully! Remounting...");
            if (!FFat.begin(false, "/ffat", 10, "ffat")) {
                ESP_LOGE(TAG, "FFat Remount Failed after format!");
                success = false;
            } else {
                ESP_LOGI(TAG, "FFat Remounted Successfully after format.");
            }
        } else {
            ESP_LOGE(TAG, "FFat Format Failed!");
            success = false;
        }
    } else {
        ESP_LOGI(TAG, "FFat Mounted Successfully.");
    }

    return success;
}

// 测试文件系统
void Sys_Filesystem::testFilesystems() {
    ESP_LOGI(TAG, "--- Testing Filesystems ---");

    // LittleFS 测试
    ESP_LOGI(TAG, "Testing LittleFS...");
    if (LittleFS.begin(false, "/littlefs", 10, "littlefs")) {
        ESP_LOGI(TAG, "LittleFS Total Bytes: %u", LittleFS.totalBytes());
        ESP_LOGI(TAG, "LittleFS Used Bytes: %u", LittleFS.usedBytes());
        listDir(LittleFS, "/", 0); // 列出根目录文件
        // 写入一个测试文件
        File file = LittleFS.open("/test_littlefs.txt", "w");
        if (file) {
            file.println("Hello from LittleFS!");
            file.close();
            ESP_LOGI(TAG, "Wrote /test_littlefs.txt to LittleFS.");
        } else {
            ESP_LOGE(TAG, "Failed to open /test_littlefs.txt for writing.");
        }
        // 读取测试文件
        file = LittleFS.open("/test_littlefs.txt", "r");
        if (file) {
            String content = file.readString();
            ESP_LOGI(TAG, "Read from /test_littlefs.txt: %s", content.c_str());
            file.close();
        } else {
            ESP_LOGE(TAG, "Failed to open /test_littlefs.txt for reading.");
        }
    } else {
        ESP_LOGE(TAG, "LittleFS not mounted for testing.");
    }

    // FFat 测试
    ESP_LOGI(TAG, "Testing FFat...");
    if (FFat.begin(false, "/ffat", 10, "ffat")) {
        ESP_LOGI(TAG, "FFat Total Bytes: %u", FFat.totalBytes());
        ESP_LOGI(TAG, "FFat Used Bytes: %u", FFat.usedBytes());
        listDir(FFat, "/", 0); // 列出根目录文件
        // 写入一个测试文件
        File file = FFat.open("/test_ffat.txt", "w");
        if (file) {
            file.println("Hello from FFat!");
            file.close();
            ESP_LOGI(TAG, "Wrote /test_ffat.txt to FFat.");
        } else {
            ESP_LOGE(TAG, "Failed to open /test_ffat.txt for writing.");
        }
        // 读取测试文件
        file = FFat.open("/test_ffat.txt", "r");
        if (file) {
            String content = file.readString();
            ESP_LOGI(TAG, "Read from /test_ffat.txt: %s", content.c_str());
            file.close();
        } else {
            ESP_LOGE(TAG, "Failed to open /test_ffat.txt for reading.");
        }
    } else {
        ESP_LOGE(TAG, "FFat not mounted for testing.");
    }

    ESP_LOGI(TAG, "---------------------------");
}

// 格式化文件系统（谨慎使用）
// 注意：此函数不再接受 fs::FS &fs 参数，而是直接操作 LittleFS 和 FFat 对象
// 如果需要通用格式化，需要传入一个指示符来判断格式化哪个文件系统
bool Sys_Filesystem::formatFilesystem(fs::FS &fs, const char* partitionLabel) {
    ESP_LOGW(TAG, "Formatting filesystem on partition: %s. This will erase all data!", partitionLabel);
    // 根据 partitionLabel 判断要格式化哪个文件系统
    if (strcmp(partitionLabel, "littlefs") == 0) {
        if (LittleFS.format()) {
            ESP_LOGI(TAG, "LittleFS formatted successfully.");
            return true;
        } else {
            ESP_LOGE(TAG, "LittleFS format failed!");
            return false;
        }
    } else if (strcmp(partitionLabel, "ffat") == 0) {
        if (FFat.format()) {
            ESP_LOGI(TAG, "FFat formatted successfully.");
            return true;
        } else {
            ESP_LOGE(TAG, "FFat format failed!");
            return false;
        }
    } else {
        ESP_LOGE(TAG, "Unknown partition label for formatting: %s", partitionLabel);
        return false;
    }
}

// 列出指定文件系统中的文件
void Sys_Filesystem::listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
    ESP_LOGI(TAG, "Listing directory: %s", dirname);

    File root = fs.open(dirname);
    if(!root){
        ESP_LOGE(TAG, "Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        ESP_LOGE(TAG, "Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            ESP_LOGI(TAG, "  DIR : %s", file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            ESP_LOGI(TAG, "  FILE: %s  SIZE: %u", file.name(), file.size());
        }
        file = root.openNextFile();
    }
}