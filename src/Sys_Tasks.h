#ifndef SYS_TASKS_H
#define SYS_TASKS_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "Sys_WebServer.h" // 包含WebServer头文件以获取WebSocket实例

/**
 * @brief 处理来自WebSocket的“获取系统状态”命令。
 *
 * 该函数会采集当前的系统状态，将其格式化为JSON，
 * 并通过WebSocket将结果广播给所有连接的客户端。
 *
 * @param client 发起请求的客户端实例 (当前未使用，但为未来扩展保留)。
 * @param ws     指向AsyncWebSocket服务器实例的指针。
 */
void handleSystemStatusCommand(AsyncWebSocketClient *client, AsyncWebSocket* ws);

#endif // SYS_TASKS_H