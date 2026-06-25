/**
 * @file    app_main.c
 * @brief   应用层入口实现 - 系统初始化与任务调度
 */

#include "app_main.h"
#include "power_mgr.h"
#include "hal_motor.h"
#include "hal_display.h"
#include "hal_sensor.h"
#include "nvs_mgr.h"
#include "wifi_mgr.h"
#include "bt_mgr.h"
#include "ota_mgr.h"
#include "car_control.h"
#include "lvgl_ui.h"
#include "comm_proto.h"
#include "board_pins.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "app_main";

/* 任务句柄 */
static TaskHandle_t s_ui_task_handle = NULL;
static TaskHandle_t s_car_task_handle = NULL;

/* ======================== BLE 数据回调 ======================== */
static void bt_data_handler(const bt_control_cmd_t *cmd)
{
    if (cmd == NULL) return;

    /* 遥控模式下处理运动指令 */
    if (car_control_get_mode() == CAR_MODE_REMOTE) {
        car_control_handle_cmd(cmd);
    }

    /* 模式切换指令任何模式下都处理 */
    if (cmd->command == BT_CMD_MODE) {
        car_control_set_mode((car_mode_t)cmd->param);
    }
}

/* ======================== OTA 进度回调 ======================== */
static void ota_progress_handler(ota_state_t state, uint8_t progress)
{
    lvgl_ui_update_ota_progress(progress);
    ESP_LOGI(TAG, "OTA state=%d progress=%d%%", state, progress);
}

/* ======================== WiFi 状态回调 ======================== */
static void wifi_state_handler(wifi_state_t state)
{
    if (state == WIFI_STATE_CONNECTED) {
        int8_t rssi = 0;
        wifi_mgr_get_rssi(&rssi);
        lvgl_ui_update_wifi_rssi(rssi);
    }
}

/* ======================== LVGL UI 刷新任务 ======================== */
static void ui_update_task(void *arg)
{
    uint32_t interval = lvgl_ui_get_handler_interval();

    while (1) {
        /* 更新传感器数据到 UI */
        uint16_t voltage = 0;
        if (power_mgr_get_voltage(&voltage) == ESP_OK) {
            lvgl_ui_update_voltage(voltage);
        }

        lvgl_ui_update_speed(car_control_get_speed());
        lvgl_ui_update_mode(car_control_get_mode());

        uint16_t distance = 0;
        if (hal_sensor_get_distance(&distance) == ESP_OK) {
            lvgl_ui_update_distance(distance);
        }

        /* 刷新 LVGL */
        lvgl_ui_task_handler(interval);
    }
}

/* ======================== 小车控制任务 ======================== */
static void car_control_task(void *arg)
{
    car_mode_t last_mode = CAR_MODE_REMOTE;

    while (1) {
        car_mode_t current_mode = car_control_get_mode();

        /* 模式切换时重建任务 */
        if (current_mode != last_mode) {
            ESP_LOGI(TAG, "Switching mode: %d -> %d", last_mode, current_mode);

            if (current_mode == CAR_MODE_TRACK) {
                car_control_track_task();
            } else if (current_mode == CAR_MODE_AVOID) {
                car_control_avoid_task();
            }

            last_mode = current_mode;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* ======================== 初始化入口 ======================== */
esp_err_t app_main_init(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ESP32 Smart Car Starting...");
    ESP_LOGI(TAG, "========================================");

    /* ---- 第1层: 内核层初始化 ---- */
    ESP_LOGI(TAG, "[1/4] Kernel layer init...");
    power_mgr_init();

    /* ---- 第2层: HAL 层初始化 ---- */
    ESP_LOGI(TAG, "[2/4] HAL layer init...");
    hal_motor_init();
    hal_sensor_init();

    /* ---- 第3层: 中间件层初始化 ---- */
    ESP_LOGI(TAG, "[3/4] Middleware layer init...");
    nvs_mgr_init();
    ota_mgr_init(ota_progress_handler);

    /* WiFi 初始化 (非阻塞，后台连接) */
    wifi_mgr_init(wifi_state_handler);
    char ssid[32] = {0}, pass[64] = {0};
    if (nvs_mgr_get_str(NVS_KEY_WIFI_SSID, ssid, sizeof(ssid)) == ESP_OK &&
        nvs_mgr_get_str(NVS_KEY_WIFI_PASS, pass, sizeof(pass)) == ESP_OK) {
        ESP_LOGI(TAG, "Reconnecting WiFi: %s", ssid);
        wifi_mgr_connect(ssid, pass);
    }

    /* BLE 初始化 */
    bt_mgr_init(bt_data_handler);
    bt_mgr_start_advertising("ESP32-SmartCar");

    /* ---- 第4层: 应用层初始化 ---- */
    ESP_LOGI(TAG, "[4/4] Application layer init...");
    lvgl_ui_init();
    car_control_init();

    /* ---- 启动任务 ---- */
    xTaskCreate(ui_update_task, "ui_task", 4096, NULL, 3, &s_ui_task_handle);
    xTaskCreate(car_control_task, "car_task", 4096, NULL, 2, &s_car_task_handle);

    ESP_LOGI(TAG, "All systems initialized, tasks running!");
    ESP_LOGI(TAG, "========================================");

    return ESP_OK;
}