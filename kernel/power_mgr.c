/**
 * @file    power_mgr.c
 * @brief   电源管理实现 - ADC 电压采集
 */

#include "power_mgr.h"
#include "board_pins.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "power_mgr";

static adc_oneshot_unit_handle_t s_adc_handle = NULL;

esp_err_t power_mgr_init(void)
{
    /* 配置 ADC Oneshot 单元 */
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = POWER_ADC_UNIT,
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc_handle));

    /* 配置 ADC 通道 */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = POWER_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, POWER_ADC_CHANNEL, &chan_cfg));

    ESP_LOGI(TAG, "ADC power monitor initialized");
    return ESP_OK;
}

esp_err_t power_mgr_get_voltage(uint16_t *voltage_mv)
{
    if (s_adc_handle == NULL || voltage_mv == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* 多次采样取平均，提高精度 */
    int raw = 0;
    for (int i = 0; i < 8; i++) {
        int sample;
        adc_oneshot_read(s_adc_handle, POWER_ADC_CHANNEL, &sample);
        raw += sample;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    raw /= 8;

    /* 12bit ADC: 0-4095 -> 0-3.3V (实际量程受衰减影响) */
    float voltage = (float)raw / 4095.0f * 3.3f * POWER_VOLTAGE_DIVIDER;
    *voltage_mv = (uint16_t)(voltage * 1000.0f);

    return ESP_OK;
}

power_status_t power_mgr_get_status(void)
{
    uint16_t voltage_mv = 0;
    if (power_mgr_get_voltage(&voltage_mv) != ESP_OK) {
        return POWER_STATUS_NORMAL;  /* 读取失败默认正常 */
    }

    if (voltage_mv < 3300) {
        return POWER_STATUS_CRITICAL;
    } else if (voltage_mv < 3500) {
        return POWER_STATUS_LOW;
    }
    return POWER_STATUS_NORMAL;
}