/**
 * @file    wifi_mgr.h
 * @brief   WiFi 管理器 - STA 模式连接 + 事件处理
 *
 * 设计原则:
 *   - 仅支持 STA 模式 (连接路由器)，不做 AP
 *   - 自动重连机制
 *   - 连接状态回调通知上层
 */

#ifndef WIFI_MGR_H
#define WIFI_MGR_H

#include "esp_err.h"
#include "esp_wifi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* WiFi 连接状态 */
typedef enum {
    WIFI_STATE_DISCONNECTED = 0,  /* 未连接 */
    WIFI_STATE_CONNECTING,        /* 连接中 */
    WIFI_STATE_CONNECTED,         /* 已连接 (已获取 IP) */
    WIFI_STATE_FAILED,            /* 连接失败 */
} wifi_state_t;

/* WiFi 状态回调 */
typedef void (*wifi_state_cb_t)(wifi_state_t state);

/**
 * @brief 初始化 WiFi STA 模式
 * @param cb 状态回调 (可选)
 * @return ESP_OK 成功
 */
esp_err_t wifi_mgr_init(wifi_state_cb_t cb);

/**
 * @brief 连接到 WiFi 网络
 * @param ssid     WiFi SSID
 * @param password WiFi 密码
 * @return ESP_OK 成功
 */
esp_err_t wifi_mgr_connect(const char *ssid, const char *password);

/**
 * @brief 断开 WiFi 连接
 * @return ESP_OK 成功
 */
esp_err_t wifi_mgr_disconnect(void);

/**
 * @brief 获取当前 WiFi 状态
 * @return 当前状态
 */
wifi_state_t wifi_mgr_get_state(void);

/**
 * @brief 获取 RSSI 信号强度
 * @param[out] rssi 信号强度 (dBm)
 * @return ESP_OK 成功
 */
esp_err_t wifi_mgr_get_rssi(int8_t *rssi);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_MGR_H */