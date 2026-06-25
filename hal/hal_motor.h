/**
 * @file    hal_motor.h
 * @brief   电机 HAL 接口 - 统一电机控制 API
 *
 * 设计原则:
 *   - 上层业务仅调用此接口，不感知底层驱动芯片型号
 *   - 更换电机驱动板 (TB6612/L298N/DRV8833) 仅修改 hal_motor.c
 *   - 支持差速转向，通过左右轮速度差实现平滑转弯
 */

#ifndef HAL_MOTOR_H
#define HAL_MOTOR_H

#include "hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化电机驱动
 * @return HAL_OK 成功
 */
esp_err_t hal_motor_init(void);

/**
 * @brief 设置左右电机速度（差速驱动）
 * @param left_speed  左轮速度 0-100 (百分比)
 * @param right_speed 右轮速度 0-100 (百分比)
 * @return HAL_OK 成功
 */
esp_err_t hal_motor_set_speed(int8_t left_speed, int8_t right_speed);

/**
 * @brief 简单方向控制（封装的差速控制）
 * @param direction 运动方向
 * @param speed     速度 0-100
 * @return HAL_OK 成功
 */
esp_err_t hal_motor_move(motor_direction_t direction, uint8_t speed);

/**
 * @brief 紧急停止（立即刹车）
 * @return HAL_OK 成功
 */
esp_err_t hal_motor_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_MOTOR_H */