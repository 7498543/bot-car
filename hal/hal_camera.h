/**
 * @file    hal_camera.h
 * @brief   摄像头 HAL 接口 - 可选模块，编译期裁剪
 *
 * 当前版本: 摄像头非必需模块，保留接口定义供后续扩展。
 * 启用方式: sdkconfig 中开启 CONFIG_ESP_CAMERA_OV2640=y
 * 说明: 基础循迹避障不需要摄像头，此接口为 FPV/视觉识别预留。
 */

#ifndef HAL_CAMERA_H
#define HAL_CAMERA_H

#include "hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 编译期可选: 未启用摄像头时所有接口返回 ESP_ERR_NOT_SUPPORTED */
#if CONFIG_ESP_CAMERA_OV2640

/**
 * @brief 初始化摄像头
 * @return HAL_OK 成功
 */
esp_err_t hal_camera_init(void);

/**
 * @brief 捕获一帧图像
 * @param[out] buf  图像数据缓冲区指针
 * @param[out] len  图像数据长度
 * @return HAL_OK 成功
 */
esp_err_t hal_camera_capture(uint8_t **buf, size_t *len);

/**
 * @brief 释放图像缓冲区
 * @return HAL_OK 成功
 */
esp_err_t hal_camera_release(void);

#else  /* 摄像头未启用 - 空实现 stub */

static inline esp_err_t hal_camera_init(void) { return ESP_ERR_NOT_SUPPORTED; }
static inline esp_err_t hal_camera_capture(uint8_t **buf, size_t *len) { return ESP_ERR_NOT_SUPPORTED; }
static inline esp_err_t hal_camera_release(void) { return ESP_ERR_NOT_SUPPORTED; }

#endif /* CONFIG_ESP_CAMERA_OV2640 */

#ifdef __cplusplus
}
#endif

#endif /* HAL_CAMERA_H */