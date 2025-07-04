#include "Sys_MemoryManager.h"
#include <esp_log.h> // For ESP_LOGx macros

static const char* TAG = "Sys_MemoryManager";

Sys_MemoryManager* Sys_MemoryManager::_instance = nullptr;

// 私有构造函数
Sys_MemoryManager::Sys_MemoryManager() : _nextBlockIndex(0) {
    // 初始化内存池中的所有块为未使用状态
    for (int i = 0; i < MAX_MEMORY_BLOCKS; ++i) {
        _memoryPool[i].ptr = nullptr;
        _memoryPool[i].size = 0;
        _memoryPool[i].inUse = false;
    }
    ESP_LOGI(TAG, "Sys_MemoryManager instance created.");
}

// 获取单例实例
Sys_MemoryManager* Sys_MemoryManager::getInstance() {
    if (_instance == nullptr) {
        _instance = new Sys_MemoryManager();
    }
    return _instance;
}

// 初始化内存管理器，预分配PSRAM内存块
void Sys_MemoryManager::initialize(size_t blockSize, int numBlocks) {
    if (numBlocks > MAX_MEMORY_BLOCKS) {
        ESP_LOGW(TAG, "Requested %d blocks, but MAX_MEMORY_BLOCKS is %d. Limiting to MAX_MEMORY_BLOCKS.", numBlocks, MAX_MEMORY_BLOCKS);
        numBlocks = MAX_MEMORY_BLOCKS;
    }

    ESP_LOGI(TAG, "Initializing Sys_MemoryManager with %d blocks of %u bytes each from PSRAM...", numBlocks, blockSize);
    for (int i = 0; i < numBlocks; ++i) {
        // 从PSRAM中分配内存
        _memoryPool[i].ptr = heap_caps_malloc(blockSize, MALLOC_CAP_SPIRAM);
        if (_memoryPool[i].ptr != nullptr) {
            _memoryPool[i].size = blockSize;
            _memoryPool[i].inUse = false;
            ESP_LOGI(TAG, "Allocated PSRAM block %d at 0x%08X, size %u bytes.", i, (unsigned int)_memoryPool[i].ptr, blockSize);
        } else {
            ESP_LOGE(TAG, "Failed to allocate PSRAM block %d of %u bytes!", i, blockSize);
            // 如果分配失败，后续的块也可能失败，可以提前退出
            break; 
        }
    }
    printMemoryInfo();
}

// 从内存池中获取一个可用内存块
void* Sys_MemoryManager::getMemoryBlock(size_t requestedSize) {
    // 简单的循环查找可用块
    for (int i = 0; i < MAX_MEMORY_BLOCKS; ++i) {
        int currentIndex = (_nextBlockIndex + i) % MAX_MEMORY_BLOCKS;
        if (!_memoryPool[currentIndex].inUse && _memoryPool[currentIndex].ptr != nullptr && _memoryPool[currentIndex].size >= requestedSize) {
            _memoryPool[currentIndex].inUse = true;
            _nextBlockIndex = (currentIndex + 1) % MAX_MEMORY_BLOCKS; // 更新下一个查找的起始点
            ESP_LOGD(TAG, "Returning PSRAM block at 0x%08X, size %u bytes (requested %u).", (unsigned int)_memoryPool[currentIndex].ptr, _memoryPool[currentIndex].size, requestedSize);
            return _memoryPool[currentIndex].ptr;
        }
    }
    ESP_LOGW(TAG, "No suitable PSRAM block found for requested size %u bytes.", requestedSize);
    return nullptr; // 没有找到合适的内存块
}

// 归还内存块到内存池
void Sys_MemoryManager::releaseMemoryBlock(void* ptr) {
    if (ptr == nullptr) {
        ESP_LOGW(TAG, "Attempted to release a nullptr.");
        return;
    }

    for (int i = 0; i < MAX_MEMORY_BLOCKS; ++i) {
        if (_memoryPool[i].ptr == ptr) {
            if (_memoryPool[i].inUse) {
                _memoryPool[i].inUse = false;
                ESP_LOGD(TAG, "Released PSRAM block at 0x%08X.", (unsigned int)ptr);
            } else {
                ESP_LOGW(TAG, "Attempted to release an already free PSRAM block at 0x%08X.", (unsigned int)ptr);
            }
            return;
        }
    }
    ESP_LOGW(TAG, "Attempted to release an unknown PSRAM block at 0x%08X.", (unsigned int)ptr);
}

// 打印内存使用情况
void Sys_MemoryManager::printMemoryInfo() {
    ESP_LOGI(TAG, "--- Sys_MemoryManager PSRAM Pool Info ---");
    uint32_t totalAllocated = 0;
    uint32_t totalInUse = 0;
    for (int i = 0; i < MAX_MEMORY_BLOCKS; ++i) {
        if (_memoryPool[i].ptr != nullptr) {
            totalAllocated += _memoryPool[i].size;
            if (_memoryPool[i].inUse) {
                totalInUse += _memoryPool[i].size;
                ESP_LOGI(TAG, "Block %d: 0x%08X, Size: %u bytes, Status: IN USE", i, (unsigned int)_memoryPool[i].ptr, _memoryPool[i].size);
            } else {
                ESP_LOGI(TAG, "Block %d: 0x%08X, Size: %u bytes, Status: FREE", i, (unsigned int)_memoryPool[i].ptr, _memoryPool[i].size);
            }
        }
    }
    ESP_LOGI(TAG, "Total PSRAM allocated by manager: %u bytes", totalAllocated);
    ESP_LOGI(TAG, "Total PSRAM in use by manager: %u bytes", totalInUse);
    ESP_LOGI(TAG, "Free PSRAM (overall system): %u bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    ESP_LOGI(TAG, "Total PSRAM (overall system): %u bytes", heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
    ESP_LOGI(TAG, "-----------------------------------------");
}