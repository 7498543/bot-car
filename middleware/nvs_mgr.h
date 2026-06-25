/**
 * @file    nvs_mgr.h
 * @brief   NVS 参数存储管理器 - 持久化配置读写
 *
 * 存储内容:
 *   - WiFi SSID / 密码
 *   - 车速校准参数
 *   - 屏幕亮度
 *   - OTA 固件 URL
 *   - 用户自定义配置
 *
 * 设计原则:
 *   - 统一键值对存取，避免散落各处的 NVS 调用
 *   - 支持默认值回退
 */

#ifndef NVS_MGR_H
#define NVS_MGR_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* NVS 存储键名宏定义 (统一管理，避免键名冲突) */
#define NVS_KEY_WIFI_SSID       "wifi_ssid"
#define NVS_KEY_WIFI_PASS       "wifi_pass"
#define NVS_KEY_CAR_SPEED       "car_speed"
#define NVS_KEY_BL_BRIGHTNESS   "bl_bright"
#define NVS_KEY_OTA_URL         "ota_url"
#define NVS_KEY_DEVICE_NAME     "dev_name"

/**
 * @brief 初始化 NVS 存储
 * @return ESP_OK 成功
 */
esp_err_t nvs_mgr_init(void);

/**
 * @brief 写入字符串
 * @param key   键名
 * @param value 字符串值
 * @return ESP_OK 成功
 */
esp_err_t nvs_mgr_set_str(const char *key, const char *value);

/**
 * @brief 读取字符串
 * @param key      键名
 * @param[out] buf 缓冲区
 * @param[in]  len 缓冲区大小
 * @return ESP_OK 成功
 */
esp_err_t nvs_mgr_get_str(const char *key, char *buf, size_t len);

/**
 * @brief 写入整数 (uint32_t)
 * @param key   键名
 * @param value 整数值
 * @return ESP_OK 成功
 */
esp_err_t nvs_mgr_set_u32(const char *key, uint32_t value);

/**
 * @brief 读取整数
 * @param key         键名
 * @param[out] value  输出值
 * @return ESP_OK 成功
 */
esp_err_t nvs_mgr_get_u32(const char *key, uint32_t *value);

/**
 * @brief 写入整数 (int32_t)
 * @param key   键名
 * @param value 整数值
 * @return ESP_OK 成功
 */
esp_err_t nvs_mgr_set_i32(const char *key, int32_t value);

/**
 * @brief 读取整数
 * @param key         键名
 * @param[out] value  输出值
 * @return ESP_OK 成功
 */
esp_err_t nvs_mgr_get_i32(const char *key, int32_t *value);

/**
 * @brief 删除指定键
 * @param key 键名
 * @return ESP_OK 成功
 */
esp_err_t nvs_mgr_erase(const char *key);

/**
 * @brief 清空所有 NVS 数据
 * @return ESP_OK 成功
 */
esp_err_t nvs_mgr_erase_all(void);

#ifdef __cplusplus
}
#endif

#endif /* NVS_MGR_H */