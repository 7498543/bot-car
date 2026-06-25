/**
 * @file    hal_types.h
 * @brief   HAL 层通用类型定义
 *
 * 所有 HAL 模块共享的错误码、状态枚举。
 */

#ifndef HAL_TYPES_H
#define HAL_TYPES_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* HAL 操作结果 */
#define HAL_OK              ESP_OK
#define HAL_ERR_INIT        ESP_ERR_INVALID_STATE
#define HAL_ERR_PARAM       ESP_ERR_INVALID_ARG
#define HAL_ERR_TIMEOUT     ESP_ERR_TIMEOUT

/* 电机运动方向 */
typedef enum {
    MOTOR_DIR_FORWARD = 0,  /* 前进 */
    MOTOR_DIR_BACKWARD,     /* 后退 */
    MOTOR_DIR_LEFT,         /* 左转 */
    MOTOR_DIR_RIGHT,        /* 右转 */
    MOTOR_DIR_STOP,         /* 停止 */
} motor_direction_t;

/* 传感器类型 */
typedef enum {
    SENSOR_ULTRASONIC = 0,  /* 超声波测距 */
    SENSOR_IR_TRACK,        /* 红外循迹 */
} sensor_type_t;

/* 红外循迹传感器状态 */
typedef struct {
    uint8_t left_1  : 1;    /* 左1: 0=黑线 1=白底 */
    uint8_t left_2  : 1;    /* 左2 */
    uint8_t right_1 : 1;    /* 右1 */
    uint8_t right_2 : 1;    /* 右2 */
    uint8_t reserved : 4;
} ir_track_state_t;

#ifdef __cplusplus
}
#endif

#endif /* HAL_TYPES_H */