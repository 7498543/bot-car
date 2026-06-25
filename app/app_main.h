/**
 * @file    app_main.h
 * @brief   应用层入口 - 统一初始化 + 任务调度
 *
 * 启动流程:
 *   1. 内核层初始化 (电源管理)
 *   2. HAL 层初始化 (电机/屏幕/传感器)
 *   3. 中间件初始化 (NVS -> WiFi -> BLE -> OTA)
 *   4. 应用层初始化 (LVGL UI -> 控制逻辑)
 *   5. 启动 FreeRTOS 任务调度
 */

#ifndef APP_MAIN_H
#define APP_MAIN_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 应用层初始化 (由 main.c 调用)
 * @return ESP_OK 成功
 */
esp_err_t app_main_init(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_MAIN_H */