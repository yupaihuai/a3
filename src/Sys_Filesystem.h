#ifndef SYS_FILESYSTEM_H
#define SYS_FILESYSTEM_H

#include <Arduino.h>
#include <LittleFS.h> // For LittleFS
#include <FFat.h>     // For FATFS

class Sys_Filesystem {
private:
    static Sys_Filesystem* _instance;
    Sys_Filesystem(); // 私有构造函数，实现单例模式

public:
    // 获取单例实例
    static Sys_Filesystem* getInstance();

    // 初始化所有文件系统
    bool setupFilesystems();

    // 测试文件系统
    void testFilesystems();

    // 格式化文件系统（谨慎使用）
    bool formatFilesystem(fs::FS &fs, const char* partitionLabel);

    // 列出指定文件系统中的文件
    void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
};

#endif // SYS_FILESYSTEM_H