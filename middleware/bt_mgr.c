/**
 * @file    bt_mgr.c
 * @brief   BLE 蓝牙管理器实现 - 简化版 GATT 透传遥控
 *
 * 设计原则:
 *   - 仅 BLE (低功耗蓝牙)，Classic BT 已通过 mem_release 释放
 *   - Service UUID: 0xFFE0, TX Char: 0xFFE1 (Write), RX Char: 0xFFE2 (Notify)
 *   - 上位机通过 Write 发送控制指令，设备通过 Notify 上报状态
 *   - 断线自动重连广播
 */

#include "bt_mgr.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"

static const char *TAG = "bt_mgr";

/* BLE UUID 定义 (16-bit 自定义 UUID) */
#define GATTS_SERVICE_UUID      0xFFE0  /* 主服务 */
#define GATTS_CHAR_UUID_TX      0xFFE1  /* 上位机 -> 小车 (Write) */
#define GATTS_CHAR_UUID_RX      0xFFE2  /* 小车 -> 上位机 (Notify) */

#define GATTS_NUM_HANDLE        8       /* 属性表句柄数 */
#define DEVICE_APPEARANCE       0x0080  /* Generic Motorized Vehicle */

/* BLE 广播参数 (复用，避免重复初始化) */
static const esp_ble_adv_params_t s_adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

/* 全局状态 */
static bt_data_cb_t s_data_cb          = NULL;
static uint16_t     s_conn_id          = 0;
static uint16_t     s_tx_char_handle   = 0;
static bool         s_connected        = false;
static uint8_t      s_service_uuid[2]  = {0};

/* GATT 属性表索引 */
enum {
    IDX_SVC,            /* 0: 服务声明 */
    IDX_CHAR_TX_DECL,   /* 1: TX 特征声明 */
    IDX_CHAR_TX_VAL,    /* 2: TX 特征值 (Write) */
    IDX_CHAR_RX_DECL,   /* 3: RX 特征声明 */
    IDX_CHAR_RX_VAL,    /* 4: RX 特征值 (Notify) */
    IDX_CHAR_RX_CFG,    /* 5: RX CCCD (Client Characteristic Configuration Descriptor) */
    HRS_IDX_NB,         /* 6: 总数 */
};

/* ======================== GATT 属性表 ======================== */
static const esp_gatts_attr_db_t s_gatt_db[HRS_IDX_NB] = {
    /* 服务声明 (Primary Service: 0xFFE0) */
    [IDX_SVC] = {
        .attr_control = { .auto_rsp = ESP_GATT_AUTO_RSP },
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p = (uint8_t *)&(uint16_t){ESP_GATT_UUID_PRI_SERVICE},
            .perm = ESP_GATT_PERM_READ,
            .max_length = sizeof(s_service_uuid),
            .length = sizeof(s_service_uuid),
            .value = s_service_uuid,
        },
    },
    /* TX 特征声明 (上位机 -> 小车) */
    [IDX_CHAR_TX_DECL] = {
        .attr_control = { .auto_rsp = ESP_GATT_AUTO_RSP },
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p = (uint8_t *)&(uint16_t){ESP_GATT_UUID_CHAR_DECLARE},
            .perm = ESP_GATT_PERM_READ,
            .max_length = 1,
            .length = 1,
            .value = (uint8_t *)&(uint8_t){ESP_GATT_CHAR_PROP_BIT_WRITE},
        },
    },
    /* TX 特征值 (Write, 无响应) */
    [IDX_CHAR_TX_VAL] = {
        .attr_control = { .auto_rsp = ESP_GATT_AUTO_RSP },
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p = (uint8_t *)&(uint16_t){GATTS_CHAR_UUID_TX},
            .perm = ESP_GATT_PERM_WRITE,
            .max_length = 20,
            .length = 0,
            .value = NULL,
        },
    },
    /* RX 特征声明 (小车 -> 上位机) */
    [IDX_CHAR_RX_DECL] = {
        .attr_control = { .auto_rsp = ESP_GATT_AUTO_RSP },
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p = (uint8_t *)&(uint16_t){ESP_GATT_UUID_CHAR_DECLARE},
            .perm = ESP_GATT_PERM_READ,
            .max_length = 1,
            .length = 1,
            .value = (uint8_t *)&(uint8_t){ESP_GATT_CHAR_PROP_BIT_NOTIFY},
        },
    },
    /* RX 特征值 (Notify) */
    [IDX_CHAR_RX_VAL] = {
        .attr_control = { .auto_rsp = ESP_GATT_AUTO_RSP },
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p = (uint8_t *)&(uint16_t){GATTS_CHAR_UUID_RX},
            .perm = ESP_GATT_PERM_READ,
            .max_length = 20,
            .length = 0,
            .value = NULL,
        },
    },
    /* RX CCCD (使能 Notify) */
    [IDX_CHAR_RX_CFG] = {
        .attr_control = { .auto_rsp = ESP_GATT_AUTO_RSP },
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p = (uint8_t *)&(uint16_t){ESP_GATT_UUID_CHAR_CLIENT_CONFIG},
            .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
            .max_length = 2,
            .length = 0,
            .value = NULL,
        },
    },
};

/* ======================== GAP 事件回调 ======================== */
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        /* 广播数据配置完成后启动广播 */
        esp_ble_gap_start_advertising(&s_adv_params);
        break;

    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "BLE advertising started");
        } else {
            ESP_LOGE(TAG, "BLE advertising start failed");
        }
        break;

    default:
        break;
    }
}

/* ======================== GATT 事件回调 ======================== */
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "GATT server registered, app_id=%d", param->reg.app_id);
        /* 创建 GATT 服务 */
        esp_ble_gatts_create_attr_tab(s_gatt_db, gatts_if, HRS_IDX_NB, IDX_SVC);
        break;

    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(TAG, "GATT service created, status=%d", param->create.status);
        /* 保存 TX 特征句柄，供后续写入使用 */
        s_tx_char_handle = param->create.attr_handle + IDX_CHAR_TX_VAL;
        /* 启动服务 */
        esp_ble_gatts_start_service(param->create.service_handle);
        break;

    case ESP_GATTS_START_EVT:
        ESP_LOGI(TAG, "GATT service started");
        break;

    case ESP_GATTS_CONNECT_EVT:
        s_conn_id   = param->connect.conn_id;
        s_connected = true;
        ESP_LOGI(TAG, "BLE client connected (conn_id=%d)", s_conn_id);
        /* 连接后更新 MTU */
        esp_ble_gattc_send_mtu_req(gatts_if, s_conn_id);
        break;

    case ESP_GATTS_DISCONNECT_EVT:
        s_connected = false;
        ESP_LOGI(TAG, "BLE client disconnected, restarting advertising...");
        /* 断线后自动重新广播 */
        esp_ble_gap_start_advertising(&s_adv_params);
        break;

    case ESP_GATTS_WRITE_EVT: {
        /* 收到上位机控制指令: [command][param] */
        if (param->write.len >= 2 && s_data_cb) {
            bt_control_cmd_t cmd = {
                .command = param->write.value[0],
                .param   = param->write.value[1],
            };
            s_data_cb(&cmd);
            ESP_LOGI(TAG, "BLE cmd: %c param=%d", cmd.command, cmd.param);
        }
        /* 确认写入 */
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                    param->write.trans_id, ESP_GATT_OK, NULL);
        break;
    }

    default:
        break;
    }
}

/* ======================== 公开接口 ======================== */

esp_err_t bt_mgr_init(bt_data_cb_t cb)
{
    s_data_cb = cb;

    /* NVS 已在 nvs_mgr_init() 中初始化，此处无需重复初始化 */

    /* 释放 Classic BT 内存，仅保留 BLE */
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    /* 初始化 BLE 控制器 */
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    /* 初始化 Bluedroid 协议栈 */
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    /* 注册 BLE 回调 */
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));

    /* 注册 GATT 应用 (app_id = 0) */
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(0));

    /* 设置 Service UUID */
    s_service_uuid[0] = (uint8_t)(GATTS_SERVICE_UUID & 0xFF);
    s_service_uuid[1] = (uint8_t)((GATTS_SERVICE_UUID >> 8) & 0xFF);

    ESP_LOGI(TAG, "BLE manager initialized");
    return ESP_OK;
}

esp_err_t bt_mgr_start_advertising(const char *device_name)
{
    if (device_name == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* 设置设备名称 */
    esp_ble_gap_set_device_name(device_name);

    /* 配置广播数据: 包含设备名 + 外观类型 */
    esp_ble_adv_data_t adv_data = {
        .set_scan_rsp    = false,
        .include_name    = true,
        .include_txpower = true,
        .appearance      = DEVICE_APPEARANCE,
        .flag            = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
    };
    esp_ble_gap_config_adv_data(&adv_data);

    ESP_LOGI(TAG, "BLE advertising configured: %s", device_name);
    return ESP_OK;
}

esp_err_t bt_mgr_stop_advertising(void)
{
    return esp_ble_gap_stop_advertising();
}