/**
 * @file    nvs_mgr.c
 * @brief   NVS 参数存储管理器实现
 */

#include "nvs_mgr.h"
#include "nvs_flash.h"
#include "esp_log.h"

static const char *TAG = "nvs_mgr";
static const char *NVS_NAMESPACE = "smart_car";

static nvs_handle_t s_nvs_handle = 0;
static bool s_initialized = false;

esp_err_t nvs_mgr_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS 分区损坏或版本更新，擦除后重新初始化 */
        ESP_LOGW(TAG, "NVS partition corrupted, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* 打开命名空间 */
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &s_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(ret));
        return ret;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "NVS manager initialized");
    return ESP_OK;
}

esp_err_t nvs_mgr_set_str(const char *key, const char *value)
{
    if (!s_initialized || key == NULL || value == NULL) return ESP_ERR_INVALID_STATE;
    return nvs_set_str(s_nvs_handle, key, value);
}

esp_err_t nvs_mgr_get_str(const char *key, char *buf, size_t len)
{
    if (!s_initialized || key == NULL || buf == NULL) return ESP_ERR_INVALID_STATE;

    size_t required_len = 0;
    esp_err_t ret = nvs_get_str(s_nvs_handle, key, NULL, &required_len);
    if (ret != ESP_OK) return ret;
    if (required_len > len) return ESP_ERR_INVALID_SIZE;

    return nvs_get_str(s_nvs_handle, key, buf, &required_len);
}

esp_err_t nvs_mgr_set_u32(const char *key, uint32_t value)
{
    if (!s_initialized || key == NULL) return ESP_ERR_INVALID_STATE;
    return nvs_set_u32(s_nvs_handle, key, value);
}

esp_err_t nvs_mgr_get_u32(const char *key, uint32_t *value)
{
    if (!s_initialized || key == NULL || value == NULL) return ESP_ERR_INVALID_STATE;
    return nvs_get_u32(s_nvs_handle, key, value);
}

esp_err_t nvs_mgr_set_i32(const char *key, int32_t value)
{
    if (!s_initialized || key == NULL) return ESP_ERR_INVALID_STATE;
    return nvs_set_i32(s_nvs_handle, key, value);
}

esp_err_t nvs_mgr_get_i32(const char *key, int32_t *value)
{
    if (!s_initialized || key == NULL || value == NULL) return ESP_ERR_INVALID_STATE;
    return nvs_get_i32(s_nvs_handle, key, value);
}

esp_err_t nvs_mgr_erase(const char *key)
{
    if (!s_initialized || key == NULL) return ESP_ERR_INVALID_STATE;
    return nvs_erase_key(s_nvs_handle, key);
}

esp_err_t nvs_mgr_erase_all(void)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    return nvs_erase_all(s_nvs_handle);
}