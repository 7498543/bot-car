/**
 * @file    hal_display.h
 * @brief   显示 HAL 接口 - 统一屏幕控制 API
 *
 * 设计原则:
 *   - 上层 LVGL 通过此接口注册显示驱动
 *   - 更换屏幕型号 (ST7789/ILI9341/GC9A01) 仅修改 hal_display.c
 *   - 支持背光 PWM 调节
 */

#ifndef HAL_DISPLAY_H
#define HAL_DISPLAY_H

#include "hal_types.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化屏幕并注册 LVGL 显示驱动
 * @return HAL_OK 成功
 */
esp_err_t hal_display_init(void);

/**
 * @brief 设置屏幕背光亮度
 * @param brightness 亮度 0-100 (百分比)
 * @return HAL_OK 成功
 */
esp_err_t hal_display_set_backlight(uint8_t brightness);

/**
 * @brief 获取 LVGL 显示刷新回调（供 LVGL 注册使用）
 * @return LVGL flush callback 函数指针
 */
void hal_display_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);

#ifdef __cplusplus
}
#endif

#endif /* HAL_DISPLAY_H */