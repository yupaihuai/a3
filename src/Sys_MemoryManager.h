#ifndef SYS_MEMORY_MANAGER_H
#define SYS_MEMORY_MANAGER_H

#include <Arduino.h>
#include <esp_heap_caps.h> // For MALLOC_CAP_SPIRAM

// 定义内存块结构体
struct MemoryBlock {
    void* ptr;
    size_t size;
    bool inUse;
};

class Sys_MemoryManager {
private:
    static Sys_MemoryManager* _instance;
    // 内存块数组，可以根据实际需求调整大小和数量
    // 这里只是一个示例，实际应用中可能需要更灵活的内存池管理
    static const int MAX_MEMORY_BLOCKS = 5; 
    MemoryBlock _memoryPool[MAX_MEMORY_BLOCKS];
    int _nextBlockIndex; // 用于简单的循环分配

    Sys_MemoryManager(); // 私有构造函数，实现单例模式

public:
    // 获取单例实例
    static Sys_MemoryManager* getInstance();

    // 初始化内存管理器，预分配PSRAM内存块
    void initialize(size_t blockSize, int numBlocks);

    // 从内存池中获取一个可用内存块
    void* getMemoryBlock(size_t requestedSize);

    // 归还内存块到内存池
    void releaseMemoryBlock(void* ptr);

    // 打印内存使用情况
    void printMemoryInfo();

    // 获取内存池总大小
    size_t getTotalPoolSize() const;

    // 获取内存池可用大小
    size_t getFreePoolSize() const;
};

#endif // SYS_MEMORY_MANAGER_H