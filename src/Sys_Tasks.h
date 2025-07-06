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
    CMD_GET_WIFI_SETTINGS,
    CMD_SAVE_WIFI_SETTINGS,
    CMD_REBOOT,
    CMD_FACTORY_RESET
} AppCommand;

// 定义在命令队列中传递的消息结构
#define MAX_PAYLOAD_SIZE 256 // 假设JSON payload的最大长度
struct CommandMessage {
    AppCommand command;
    char payload[MAX_PAYLOAD_SIZE]; // 用于传递JSON字符串数据
};

// 外部可以访问的命令队列句柄
extern QueueHandle_t xCommandQueue;
extern QueueHandle_t xStateQueue; // 系统状态队列
extern QueueHandle_t xBarcodeQueue; // 条码识别队列

/**
 * @brief 初始化并创建所有系统级FreeRTOS任务。
 *
 * @param wsInstance 指向AsyncWebSocket实例的指针，用于任务回调。
 */
void setupTasks(AsyncWebSocket* wsInstance);

/**
 * @brief WebSocket数据推送任务。
 * 负责从多个数据队列接收数据，并通过WebSocket发送到前端。
 * @param pvParameters 任务参数 (未使用)。
 */
void taskWebSocketPusher(void *pvParameters);

/**
 * @brief 系统状态监控任务。
 * 负责周期性采集系统状态，并通过队列发送给WebSocket推送任务。
 * @param pvParameters 任务参数 (未使用)。
 */
void taskSystemMonitor(void *pvParameters);

/**
 * @brief 通用的工人任务（Worker Task）。
 * 负责从命令队列中接收并处理所有耗时或阻塞的操作。
 * @param pvParameters 任务参数 (未使用)。
 */
void taskWorker(void *pvParameters);

#endif // SYS_TASKS_H