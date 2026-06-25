/**
 * @file    car_control.h
 * @brief   小车控制逻辑 - 遥控/循迹/避障 三种模式
 *
 * 设计原则:
 *   - 模式切换通过统一接口，互斥运行
 *   - 控制逻辑与硬件驱动完全解耦，仅通过 HAL 接口操作
 *   - 支持外部命令 (BLE/WiFi) 和内部自主决策
 */

#ifndef CAR_CONTROL_H
#define CAR_CONTROL_H

#include "hal_types.h"
#include "bt_mgr.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 避障距离阈值 */
#define AVOID_DISTANCE_THRESHOLD_CM  20   /* 小于此距离触发避障 */
#define AVOID_DISTANCE_SAFE_CM       40   /* 安全距离，恢复前进 */

/**
 * @brief 初始化小车控制系统
 * @return ESP_OK 成功
 */
esp_err_t car_control_init(void);

/**
 * @brief 设置当前运行模式
 * @param mode 目标模式
 * @return ESP_OK 成功
 */
esp_err_t car_control_set_mode(car_mode_t mode);

/**
 * @brief 获取当前运行模式
 * @return 当前模式
 */
car_mode_t car_control_get_mode(void);

/**
 * @brief 处理外部遥控指令 (来自 BLE/WiFi)
 * @param cmd 控制指令
 * @return ESP_OK 成功
 */
esp_err_t car_control_handle_cmd(const bt_control_cmd_t *cmd);

/**
 * @brief 循迹模式任务 (需在独立 FreeRTOS Task 中调用)
 * @note  阻塞式循环，模式切换时退出
 */
void car_control_track_task(void);

/**
 * @brief 避障模式任务 (需在独立 FreeRTOS Task 中调用)
 * @note  阻塞式循环，模式切换时退出
 */
void car_control_avoid_task(void);

/**
 * @brief 获取当前车速
 * @return 速度百分比 0-100
 */
uint8_t car_control_get_speed(void);

#ifdef __cplusplus
}
#endif

#endif /* CAR_CONTROL_H */