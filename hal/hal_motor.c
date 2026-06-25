/**
 * @file    hal_motor.c
 * @brief   电机 HAL 实现 - 基于 LEDC PWM + GPIO 方向控制
 *
 * 适配 TB6612 双路电机驱动板，更换 L298N 仅需修改方向引脚逻辑。
 * 当前实现: 单 PWM 调速 + 双 IO 方向 (IN1/IN2)
 */

#include "hal_motor.h"
#include "board_pins.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "hal_motor";

/* 速度百分比转换为 PWM 占空比 */
static inline uint32_t speed_to_duty(uint8_t speed_percent)
{
    if (speed_percent > 100) speed_percent = 100;
    return (uint32_t)(speed_percent * 1023 / 100);
}

esp_err_t hal_motor_init(void)
{
    /* 配置 PWM 定时器 */
    ledc_timer_config_t timer_cfg = {
        .speed_mode = MOTOR_PWM_MODE,
        .duty_resolution = MOTOR_PWM_RESOLUTION,
        .timer_num = MOTOR_PWM_TIMER,
        .freq_hz = MOTOR_PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    /* 配置 PWM 通道 - 左电机 */
    ledc_channel_config_t ch_a = {
        .gpio_num = MOTOR_A_PWM_GPIO,
        .speed_mode = MOTOR_PWM_MODE,
        .channel = MOTOR_PWM_CHANNEL_A,
        .timer_sel = MOTOR_PWM_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_a));

    /* 配置 PWM 通道 - 右电机 */
    ledc_channel_config_t ch_b = {
        .gpio_num = MOTOR_B_PWM_GPIO,
        .speed_mode = MOTOR_PWM_MODE,
        .channel = MOTOR_PWM_CHANNEL_B,
        .timer_sel = MOTOR_PWM_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_b));

    /* 配置方向控制 GPIO */
    gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << MOTOR_A_IN1_GPIO) | (1ULL << MOTOR_A_IN2_GPIO) |
                        (1ULL << MOTOR_B_IN1_GPIO) | (1ULL << MOTOR_B_IN2_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_cfg));

    /* 初始化为停止状态 */
    hal_motor_stop();

    ESP_LOGI(TAG, "Motor HAL initialized (TB6612 mode)");
    return ESP_OK;
}

esp_err_t hal_motor_set_speed(int8_t left_speed, int8_t right_speed)
{
    uint8_t abs_left = (left_speed < 0) ? -left_speed : left_speed;
    uint8_t abs_right = (right_speed < 0) ? -right_speed : right_speed;

    /* 左电机方向 */
    if (left_speed > 0) {
        gpio_set_level(MOTOR_A_IN1_GPIO, 1);
        gpio_set_level(MOTOR_A_IN2_GPIO, 0);
    } else if (left_speed < 0) {
        gpio_set_level(MOTOR_A_IN1_GPIO, 0);
        gpio_set_level(MOTOR_A_IN2_GPIO, 1);
    } else {
        gpio_set_level(MOTOR_A_IN1_GPIO, 0);
        gpio_set_level(MOTOR_A_IN2_GPIO, 0);
    }

    /* 右电机方向 */
    if (right_speed > 0) {
        gpio_set_level(MOTOR_B_IN1_GPIO, 1);
        gpio_set_level(MOTOR_B_IN2_GPIO, 0);
    } else if (right_speed < 0) {
        gpio_set_level(MOTOR_B_IN1_GPIO, 0);
        gpio_set_level(MOTOR_B_IN2_GPIO, 1);
    } else {
        gpio_set_level(MOTOR_B_IN1_GPIO, 0);
        gpio_set_level(MOTOR_B_IN2_GPIO, 0);
    }

    /* 设置 PWM 占空比 */
    ledc_set_duty(MOTOR_PWM_MODE, MOTOR_PWM_CHANNEL_A, speed_to_duty(abs_left));
    ledc_update_duty(MOTOR_PWM_MODE, MOTOR_PWM_CHANNEL_A);
    ledc_set_duty(MOTOR_PWM_MODE, MOTOR_PWM_CHANNEL_B, speed_to_duty(abs_right));
    ledc_update_duty(MOTOR_PWM_MODE, MOTOR_PWM_CHANNEL_B);

    return ESP_OK;
}

esp_err_t hal_motor_move(motor_direction_t direction, uint8_t speed)
{
    switch (direction) {
    case MOTOR_DIR_FORWARD:
        return hal_motor_set_speed(speed, speed);
    case MOTOR_DIR_BACKWARD:
        return hal_motor_set_speed(-speed, -speed);
    case MOTOR_DIR_LEFT:
        return hal_motor_set_speed(speed / 2, speed);     /* 差速左转 */
    case MOTOR_DIR_RIGHT:
        return hal_motor_set_speed(speed, speed / 2);     /* 差速右转 */
    case MOTOR_DIR_STOP:
    default:
        return hal_motor_stop();
    }
}

esp_err_t hal_motor_stop(void)
{
    ledc_set_duty(MOTOR_PWM_MODE, MOTOR_PWM_CHANNEL_A, 0);
    ledc_update_duty(MOTOR_PWM_MODE, MOTOR_PWM_CHANNEL_A);
    ledc_set_duty(MOTOR_PWM_MODE, MOTOR_PWM_CHANNEL_B, 0);
    ledc_update_duty(MOTOR_PWM_MODE, MOTOR_PWM_CHANNEL_B);

    gpio_set_level(MOTOR_A_IN1_GPIO, 0);
    gpio_set_level(MOTOR_A_IN2_GPIO, 0);
    gpio_set_level(MOTOR_B_IN1_GPIO, 0);
    gpio_set_level(MOTOR_B_IN2_GPIO, 0);

    return ESP_OK;
}