/**
 * @file    hal_sensor.c
 * @brief   传感器 HAL 实现 - HC-SR04 超声波 + TCRT5000 红外循迹
 */

#include "hal_sensor.h"
#include "board_pins.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

static const char *TAG = "hal_sensor";

esp_err_t hal_sensor_init(void)
{
    /* 超声波引脚配置 */
    gpio_config_t us_cfg = {
        .pin_bit_mask = (1ULL << ULTRASONIC_TRIG_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&us_cfg);

    gpio_config_t us_echo_cfg = {
        .pin_bit_mask = (1ULL << ULTRASONIC_ECHO_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&us_echo_cfg);
    gpio_set_level(ULTRASONIC_TRIG_GPIO, 0);

    /* 红外循迹引脚配置 (全部为输入) */
    gpio_config_t ir_cfg = {
        .pin_bit_mask = (1ULL << IR_TRACK_L1_GPIO) | (1ULL << IR_TRACK_L2_GPIO) |
                        (1ULL << IR_TRACK_R1_GPIO) | (1ULL << IR_TRACK_R2_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&ir_cfg);

    ESP_LOGI(TAG, "Sensors initialized (HC-SR04 + TCRT5000 x4)");
    return ESP_OK;
}

esp_err_t hal_sensor_get_distance(uint16_t *distance_cm)
{
    if (distance_cm == NULL) return ESP_ERR_INVALID_ARG;

    /* 发送 10us 触发脉冲 */
    gpio_set_level(ULTRASONIC_TRIG_GPIO, 1);
    ets_delay_us(10);
    gpio_set_level(ULTRASONIC_TRIG_GPIO, 0);

    /* 等待回响信号上升沿 (超时 30ms ≈ 5m) */
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(ULTRASONIC_ECHO_GPIO) == 0) {
        if ((esp_timer_get_time() - start) > 30000) {
            *distance_cm = 0;
            return ESP_ERR_TIMEOUT;
        }
    }

    /* 测量高电平持续时间 */
    int64_t pulse_start = esp_timer_get_time();
    while (gpio_get_level(ULTRASONIC_ECHO_GPIO) == 1) {
        if ((esp_timer_get_time() - pulse_start) > 30000) {
            *distance_cm = 0;
            return ESP_ERR_TIMEOUT;
        }
    }
    int64_t pulse_end = esp_timer_get_time();

    /* 距离 = 高电平时间(us) * 0.034 / 2 (往返) */
    *distance_cm = (uint16_t)((pulse_end - pulse_start) * 0.034f / 2.0f);

    return ESP_OK;
}

esp_err_t hal_sensor_get_track(ir_track_state_t *state)
{
    if (state == NULL) return ESP_ERR_INVALID_ARG;

    /* 读取四路红外传感器 (0=黑线/低电平, 1=白底/高电平) */
    state->left_1  = gpio_get_level(IR_TRACK_L1_GPIO);
    state->left_2  = gpio_get_level(IR_TRACK_L2_GPIO);
    state->right_1 = gpio_get_level(IR_TRACK_R1_GPIO);
    state->right_2 = gpio_get_level(IR_TRACK_R2_GPIO);
    state->reserved = 0;

    return ESP_OK;
}