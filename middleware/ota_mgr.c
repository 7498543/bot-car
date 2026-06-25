/**
 * @file    ota_mgr.c
 * @brief   OTA 固件升级管理器实现
 *
 * 使用 ESP-IDF 原生 esp_https_ota 组件，支持 HTTPS 固件下载。
 * 简化版: 当前使用 HTTP 传输 (esp_http_client)，可切换为 HTTPS。
 */

#include "ota_mgr.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_app_desc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ota_mgr";

static ota_state_t s_ota_state = OTA_STATE_IDLE;
static ota_progress_cb_t s_progress_cb = NULL;

/* HTTP 事件回调 */
static esp_err_t ota_http_event_cb(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "OTA connected to server");
        break;
    case HTTP_EVENT_ON_DATA:
        /* 数据接收中，由 esp_https_ota 内部处理 */
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "OTA download finished");
        break;
    default:
        break;
    }
    return ESP_OK;
}

esp_err_t ota_mgr_init(ota_progress_cb_t cb)
{
    s_progress_cb = cb;
    s_ota_state = OTA_STATE_IDLE;

    /* 获取当前固件信息 */
    const esp_app_desc_t *desc = esp_app_get_description();
    ESP_LOGI(TAG, "Current firmware: %s (version: %s)", desc->project_name, desc->version);
    ESP_LOGI(TAG, "OTA partition: %s", desc->type == ESP_APP_DESC_OTA_0 ? "ota_0" : "ota_1");

    return ESP_OK;
}

esp_err_t ota_mgr_upgrade(const char *firmware_url)
{
    if (firmware_url == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    s_ota_state = OTA_STATE_CHECKING;
    if (s_progress_cb) s_progress_cb(s_ota_state, 0);

    ESP_LOGI(TAG, "Starting OTA from: %s", firmware_url);

    /* 配置 HTTP 客户端 */
    esp_http_client_config_t http_cfg = {
        .url = firmware_url,
        .event_handler = ota_http_event_cb,
        .timeout_ms = 10000,
        .keep_alive_enable = true,
        .skip_cert_common_name_check = true,  /* 简化: 跳过证书校验 */
    };

    /* 配置 OTA */
    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
        .http_client_init_cb = NULL,
        .partial_http_download = false,
        .max_http_request_size = 4096,
    };

    s_ota_state = OTA_STATE_DOWNLOADING;
    if (s_progress_cb) s_progress_cb(s_ota_state, 0);

    esp_err_t ret = esp_https_ota(&ota_cfg);

    if (ret == ESP_OK) {
        s_ota_state = OTA_STATE_SUCCESS;
        if (s_progress_cb) s_progress_cb(s_ota_state, 100);
        ESP_LOGI(TAG, "OTA upgrade successful, restarting...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    } else {
        s_ota_state = OTA_STATE_FAILED;
        if (s_progress_cb) s_progress_cb(s_ota_state, 0);
        ESP_LOGE(TAG, "OTA upgrade failed: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t ota_mgr_confirm(void)
{
    esp_err_t ret = esp_ota_mark_app_valid_cancel_rollback();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Firmware confirmed, rollback cancelled");
    } else {
        ESP_LOGE(TAG, "Failed to confirm firmware: %s", esp_err_to_name(ret));
    }
    return ret;
}

ota_state_t ota_mgr_get_state(void)
{
    return s_ota_state;
}

esp_err_t ota_mgr_get_version(char *version, size_t max_len)
{
    if (version == NULL || max_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    const esp_app_desc_t *desc = esp_app_get_description();
    snprintf(version, max_len, "%s", desc->version);
    return ESP_OK;
}