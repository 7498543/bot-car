/**
 * @file    wifi_mgr.c
 * @brief   WiFi 管理器实现
 */

#include "wifi_mgr.h"
#include "nvs_mgr.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"

static const char *TAG = "wifi_mgr";

/* FreeRTOS 事件组标志位 */
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

static EventGroupHandle_t s_wifi_event_group = NULL;
static wifi_state_cb_t s_state_cb = NULL;
static wifi_state_t s_current_state = WIFI_STATE_DISCONNECTED;
static int s_retry_count = 0;

#define WIFI_MAX_RETRY  5

/* WiFi 事件处理 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            s_current_state = WIFI_STATE_CONNECTING;
            if (s_state_cb) s_state_cb(s_current_state);
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            s_current_state = WIFI_STATE_DISCONNECTED;
            if (s_state_cb) s_state_cb(s_current_state);

            if (s_retry_count < WIFI_MAX_RETRY) {
                esp_wifi_connect();
                s_retry_count++;
                ESP_LOGI(TAG, "WiFi reconnecting (attempt %d/%d)", s_retry_count, WIFI_MAX_RETRY);
            } else {
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                s_current_state = WIFI_STATE_FAILED;
                if (s_state_cb) s_state_cb(s_current_state);
                ESP_LOGE(TAG, "WiFi connection failed after %d retries", WIFI_MAX_RETRY);
            }
            break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        s_retry_count = 0;
        s_current_state = WIFI_STATE_CONNECTED;
        if (s_state_cb) s_state_cb(s_current_state);
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "WiFi connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

esp_err_t wifi_mgr_init(wifi_state_cb_t cb)
{
    s_state_cb = cb;

    /* 创建事件组 */
    s_wifi_event_group = xEventGroupCreate();

    /* 初始化 TCP/IP 协议栈 */
    ESP_ERROR_CHECK(esp_netif_init());

    /* 创建默认事件循环 */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* 创建 WiFi STA 网络接口 */
    esp_netif_create_default_wifi_sta();

    /* 初始化 WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* 注册事件处理 */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                        &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                        &wifi_event_handler, NULL, NULL));

    /* 设置为 STA 模式 */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_LOGI(TAG, "WiFi manager initialized");
    return ESP_OK;
}

esp_err_t wifi_mgr_connect(const char *ssid, const char *password)
{
    if (ssid == NULL || password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* 配置 WiFi 连接参数 */
    wifi_config_t wifi_cfg = {0};
    strncpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *)wifi_cfg.sta.password, password, sizeof(wifi_cfg.sta.password) - 1);
    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_cfg.sta.pmf_cfg.capable = true;
    wifi_cfg.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

    /* 启动 WiFi */
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to WiFi: %s", ssid);

    /* 保存到 NVS */
    nvs_mgr_set_str(NVS_KEY_WIFI_SSID, ssid);
    nvs_mgr_set_str(NVS_KEY_WIFI_PASS, password);

    /* 等待连接结果 (超时 15 秒) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                        pdFALSE, pdFALSE, pdMS_TO_TICKS(15000));

    if (bits & WIFI_CONNECTED_BIT) {
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
}

esp_err_t wifi_mgr_disconnect(void)
{
    return esp_wifi_disconnect();
}

wifi_state_t wifi_mgr_get_state(void)
{
    return s_current_state;
}

esp_err_t wifi_mgr_get_rssi(int8_t *rssi)
{
    if (rssi == NULL) return ESP_ERR_INVALID_ARG;
    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    if (ret == ESP_OK) {
        *rssi = ap_info.rssi;
    }
    return ret;
}