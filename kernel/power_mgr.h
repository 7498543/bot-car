/**
 * @file    power_mgr.h
 * @brief   电源管理 - 电池电压采集、低电量告警
 *
 * 轻量化设计: 仅支持 ADC 电压采集，不包含深度休眠。
 * 小车场景实时运行，不需要休眠功耗管理。
 */

#ifndef POWER_MGR_H
#define POWER_MGR_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 电源状态 */
typedef enum {
    POWER_STATUS_NORMAL = 0,  /* 正常 (>3.5V) */
    POWER_STATUS_LOW,         /* 低电量 (3.3V - 3.5V) */
    POWER_STATUS_CRITICAL,    /* 临界 (<3.3V) */
} power_status_t;

/**
 * @brief 初始化 ADC 电源监测
 * @return ESP_OK 成功
 */
esp_err_t power_mgr_init(void);

/**
 * @brief 获取电池电压 (mV)
 * @param[out] voltage_mv 电压值 (mV)
 * @return ESP_OK 成功
 */
esp_err_t power_mgr_get_voltage(uint16_t *voltage_mv);

/**
 * @brief 获取电源状态等级
 * @return 电源状态枚举
 */
power_status_t power_mgr_get_status(void);

#ifdef __cplusplus
}
#endif

#endif /* POWER_MGR_H */