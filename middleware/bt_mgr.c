/**
 * @file    bt_mgr.c
 * @brief   BLE 蓝牙管理器实现
 *
 * 简化版 BLE GATT 服务:
 *   - Service UUID: 0xFFE0 (通用自定义服务)
 *   - Characteristic UUID: 0xFFE1 (读写 + Notify)
 *   - 上位机写入控制指令，设备回复状态
 */

#include "bt_mgr.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"
#include "nvs_flash.h"

static const char *TAG = "bt_mgr";

/* BLE UUID 定义 */
#define GATTS_SERVICE_UUID 0xFFE0
#define GATTS_CHAR_UUID_TX 0xFFE1 /* 上位机 -> 小车 (Write) */
#define GATTS_CHAR_UUID_RX 0xFFE2 /* 小车 -> 上位机 (Notify) */
#define GATTS_NUM_HANDLE 8

#define DEVICE_APPEARANCE 0x0080 /* Generic Motorized Vehicle */

static bt_data_cb_t s_data_cb = NULL;
static uint16_t s_conn_id = 0;
static uint16_t s_tx_char_handle = 0;
static bool s_connected = false;

/* GATT 属性句柄 */
enum
{
    IDX_SVC,
    IDX_CHAR_TX_DECL,
    IDX_CHAR_TX_VAL,
    IDX_CHAR_RX_DECL,
    IDX_CHAR_RX_VAL,
    IDX_CHAR_RX_CFG,
    HRS_IDX_NB,
};

/* GAP 事件回调 */
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&(esp_ble_adv_params_t){
            .adv_int_min = 0x20,
            .adv_int_max = 0x40,
            .adv_type = ADV_TYPE_IND,
            .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
            .channel_map = ADV_CHNL_ALL,
            .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        });
        break;

    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGI(TAG, "BLE advertising started");
        }
        break;

    default:
        break;
    }
}

/* GATT 事件回调 */
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "GATT server registered");
        esp_ble_gatts_create_service(gatts_if, &(esp_gatts_attr_db_t){
                                                   // TODO: 完整 GATT 属性表定义
                                               },
                                     GATTS_NUM_HANDLE, IDX_SVC);
        break;

    case ESP_GATTS_CONNECT_EVT:
        s_conn_id = param->connect.conn_id;
        s_connected = true;
        ESP_LOGI(TAG, "BLE client connected (conn_id=%d)", s_conn_id);
        break;

    case ESP_GATTS_DISCONNECT_EVT:
        s_connected = false;
        ESP_LOGI(TAG, "BLE client disconnected");
        esp_ble_gap_start_advertising(&(esp_ble_adv_params_t){
            .adv_int_min = 0x20,
            .adv_int_max = 0x40,
            .adv_type = ADV_TYPE_IND,
            .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
            .channel_map = ADV_CHNL_ALL,
            .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        });
        break;

    case ESP_GATTS_WRITE_EVT:
    {
        /* 收到上位机控制指令 */
        if (param->write.len >= 2 && s_data_cb)
        {
            bt_control_cmd_t cmd = {
                .command = param->write.value[0],
                .param = param->write.value[1],
            };
            s_data_cb(&cmd);
            ESP_LOGI(TAG, "BLE cmd: %c param=%d", cmd.command, cmd.param);
        }
        break;
    }

    default:
        break;
    }
}

esp_err_t bt_mgr_init(bt_data_cb_t cb)
{
    s_data_cb = cb;

    /* 释放 Classic BT 内存，仅保留 BLE */
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    /* 初始化 BLE 控制器 */
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    /* 初始化 Bluedroid 协议栈 */
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    /* 注册回调 */
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(0)); /* app_id = 0 */

    ESP_LOGI(TAG, "BLE manager initialized");
    return ESP_OK;
}

esp_err_t bt_mgr_start_advertising(const char *device_name)
{
    if (device_name == NULL)
        return ESP_ERR_INVALID_ARG;

    /* 设置设备名称 */
    esp_ble_gap_set_device_name(device_name);

    /* 配置广播数据 */
    esp_ble_adv_data_t adv_data = {
        .set_scan_rsp = false,
        .include_name = true,
        .include_txpower = true,
        .appearance = DEVICE_APPEARANCE,
        .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
    };
    esp_ble_gap_config_adv_data(&adv_data);

    ESP_LOGI(TAG, "BLE advertising configured: %s", device_name);
    return ESP_OK;
}

esp_err_t bt_mgr_stop_advertising(void)
{
    return esp_ble_gap_stop_advertising();
}