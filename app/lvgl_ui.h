/**
 * @file    lvgl_ui.h
 * @brief   LVGL 人机交互界面 - 状态显示
 * 设计原则:
 *   - 轻量化 UI，无动画特效
 *   - 数据通过回调函数更新，不直接操作硬件
 *   - 界面与业务逻辑完全解耦
 */

#ifndef LVGL_UI_H
#define LVGL_UI_H

#include "esp_err.h"
#include "comm_proto.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 LVGL 和 UI 界面
 * @return ESP_OK 成功
 */
esp_err_t lvgl_ui_init(void);

/**
 * @brief LVGL 定时器任务 (需在 FreeRTOS Task 中周期性调用)
 * @param interval_ms 调用间隔 (ms)
 */
void lvgl_ui_task_handler(uint32_t interval_ms);

/**
 * @brief 更新电池电压显示
 * @param voltage_mv 电压 (mV)
 */
void lvgl_ui_update_voltage(uint16_t voltage_mv);

/**
 * @brief 更新车速显示
 * @param speed 速度 0-100
 */
void lvgl_ui_update_speed(uint8_t speed);

/**
 * @brief 更新当前模式显示
 * @param mode 当前模式
 */
void lvgl_ui_update_mode(car_mode_t mode);

/**
 * @brief 更新超声波距离显示
 * @param distance_cm 距离 (cm)
 */
void lvgl_ui_update_distance(uint16_t distance_cm);

/**
 * @brief 更新 WiFi 信号强度显示
 * @param rssi 信号强度 (dBm)
 */
void lvgl_ui_update_wifi_rssi(int8_t rssi);

/**
 * @brief 更新 OTA 升级进度
 * @param progress 进度 0-100
 */
void lvgl_ui_update_ota_progress(uint8_t progress);

/**
 * @brief 获取 LVGL 任务需要的间隔时间 (ms)
 * @return 推荐调用间隔
 */
uint32_t lvgl_ui_get_handler_interval(void);

#ifdef __cplusplus
}
#endif

#endif /* LVGL_UI_H */