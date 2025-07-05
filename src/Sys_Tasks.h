#ifndef SYS_TASKS_H
#define SYS_TASKS_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
// 前向声明以避免循环依赖
class AsyncWebSocket;

// 定义可以发送给工人任务的命令类型
typedef enum {
    CMD_GET_SYSTEM_STATUS,
    CMD_SCAN_WIFI,
    // 未来可以添加更多命令
    // CMD_SAVE_WIFI_CONFIG,
} AppCommand;

// 定义在命令队列中传递的消息结构
struct CommandMessage {
    AppCommand command;
    // 可以添加一个void*指针来传递额外的数据，但为了简单起见，暂时省略
    // void* payload;
};

// 外部可以访问的命令队列句柄
extern QueueHandle_t xCommandQueue;

/**
 * @brief 初始化并创建所有系统级FreeRTOS任务。
 *
 * @param wsInstance 指向AsyncWebSocket实例的指针，用于任务回调。
 */
void setupTasks(AsyncWebSocket* wsInstance);

/**
 * @brief 通用的工人任务（Worker Task）。
 * 负责从命令队列中接收并处理所有耗时或阻塞的操作。
 * @param pvParameters 任务参数 (未使用)。
 */
void taskWorker(void *pvParameters);

#endif // SYS_TASKS_H