/**
 * @file    hal_sensor.h
 * @brief   传感器 HAL 接口 - 超声波测距 + 红外循迹
 *
 * 设计原则:
 *   - 统一传感器读取接口，上层不感知具体传感器型号
 *   - 更换传感器型号仅修改 hal_sensor.c
 */

#ifndef HAL_SENSOR_H
#define HAL_SENSOR_H

#include "hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化所有传感器
 * @return HAL_OK 成功
 */
esp_err_t hal_sensor_init(void);

/**
 * @brief 获取超声波测距值
 * @param[out] distance_cm 距离 (cm)，0 表示超时/无回波
 * @return HAL_OK 成功
 */
esp_err_t hal_sensor_get_distance(uint16_t *distance_cm);

/**
 * @brief 获取红外循迹传感器状态
 * @param[out] state 四路红外传感器状态
 * @return HAL_OK 成功
 */
esp_err_t hal_sensor_get_track(ir_track_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* HAL_SENSOR_H */