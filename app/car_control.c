/**
 * @file    car_control.c
 * @brief   小车控制逻辑实现
 *
 * 三种模式:
 *   1. REMOTE: 接收 BLE/WiFi 指令，直接控制电机
 *   2. TRACK:  基于红外传感器循迹，自动纠偏
 *   3. AVOID:  基于超声波传感器，前进中遇障自动转向
 */

#include "car_control.h"
#include "hal_motor.h"
#include "hal_sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "car_ctrl";

static car_mode_t s_current_mode = CAR_MODE_REMOTE;
static uint8_t s_current_speed = 50;  /* 默认速度 50% */

esp_err_t car_control_init(void)
{
    s_current_mode = CAR_MODE_REMOTE;
    s_current_speed = 50;
    ESP_LOGI(TAG, "Car control initialized (mode=REMOTE, speed=%d%%)", s_current_speed);
    return ESP_OK;
}

esp_err_t car_control_set_mode(car_mode_t mode)
{
    s_current_mode = mode;
    ESP_LOGI(TAG, "Car mode changed to: %d", mode);

    /* 模式切换时先停止电机 */
    if (mode == CAR_MODE_REMOTE) {
        hal_motor_stop();
    }

    return ESP_OK;
}

car_mode_t car_control_get_mode(void)
{
    return s_current_mode;
}

esp_err_t car_control_handle_cmd(const bt_control_cmd_t *cmd)
{
    if (cmd == NULL) return ESP_ERR_INVALID_ARG;

    switch (cmd->command) {
    case BT_CMD_FORWARD:
        hal_motor_move(MOTOR_DIR_FORWARD, cmd->param);
        s_current_speed = cmd->param;
        break;
    case BT_CMD_BACKWARD:
        hal_motor_move(MOTOR_DIR_BACKWARD, cmd->param);
        s_current_speed = cmd->param;
        break;
    case BT_CMD_LEFT:
        hal_motor_move(MOTOR_DIR_LEFT, cmd->param);
        s_current_speed = cmd->param;
        break;
    case BT_CMD_RIGHT:
        hal_motor_move(MOTOR_DIR_RIGHT, cmd->param);
        s_current_speed = cmd->param;
        break;
    case BT_CMD_STOP:
        hal_motor_stop();
        break;
    case BT_CMD_MODE:
        car_control_set_mode((car_mode_t)cmd->param);
        break;
    default:
        ESP_LOGW(TAG, "Unknown command: 0x%02X", cmd->command);
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

void car_control_track_task(void)
{
    ESP_LOGI(TAG, "Track task started");
    s_current_mode = CAR_MODE_TRACK;

    while (s_current_mode == CAR_MODE_TRACK) {
        ir_track_state_t ir_state;
        if (hal_sensor_get_track(&ir_state) != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        /* 循迹纠偏逻辑:
         *   传感器排列: L2 L1 | R1 R2
         *   黑线=0, 白底=1
         *   理想状态: L1=0, R1=0 (黑线在中间)
         */
        if (ir_state.left_1 == 0 && ir_state.right_1 == 0) {
            /* 黑线居中，直行 */
            hal_motor_move(MOTOR_DIR_FORWARD, s_current_speed);
        } else if (ir_state.left_1 == 1 && ir_state.right_1 == 0) {
            /* 偏右，左转纠偏 */
            hal_motor_set_speed(s_current_speed / 2, s_current_speed);
        } else if (ir_state.left_1 == 0 && ir_state.right_1 == 1) {
            /* 偏左，右转纠偏 */
            hal_motor_set_speed(s_current_speed, s_current_speed / 2);
        } else if (ir_state.left_2 == 0) {
            /* 严重偏右，急左转 */
            hal_motor_set_speed(-s_current_speed / 2, s_current_speed);
        } else if (ir_state.right_2 == 0) {
            /* 严重偏左，急右转 */
            hal_motor_set_speed(s_current_speed, -s_current_speed / 2);
        } else {
            /* 全部偏离 (掉线)，停止 */
            hal_motor_stop();
        }

        vTaskDelay(pdMS_TO_TICKS(20));  /* 50Hz 控制频率 */
    }

    hal_motor_stop();
    ESP_LOGI(TAG, "Track task stopped");
}

void car_control_avoid_task(void)
{
    ESP_LOGI(TAG, "Avoid task started");
    s_current_mode = CAR_MODE_AVOID;

    while (s_current_mode == CAR_MODE_AVOID) {
        uint16_t distance = 0;
        if (hal_sensor_get_distance(&distance) != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        if (distance > 0 && distance < AVOID_DISTANCE_THRESHOLD_CM) {
            /* 检测到障碍物，后退 + 转向 */
            hal_motor_move(MOTOR_DIR_BACKWARD, s_current_speed / 2);
            vTaskDelay(pdMS_TO_TICKS(500));
            hal_motor_move(MOTOR_DIR_RIGHT, s_current_speed);
            vTaskDelay(pdMS_TO_TICKS(800));
        } else {
            /* 安全距离，继续前进 */
            hal_motor_move(MOTOR_DIR_FORWARD, s_current_speed);
        }

        vTaskDelay(pdMS_TO_TICKS(50));  /* 20Hz 控制频率 */
    }

    hal_motor_stop();
    ESP_LOGI(TAG, "Avoid task stopped");
}

uint8_t car_control_get_speed(void)
{
    return s_current_speed;
}