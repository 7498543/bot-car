/**
 * @file    lvgl_ui.c
 * @brief   LVGL 人机交互界面实现
 */

#include "lvgl_ui.h"
#include "hal_display.h"
#include "board_pins.h"
#include "lvgl.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "lvgl_ui";

/* LVGL 对象 */
static lv_obj_t *s_label_voltage = NULL;
static lv_obj_t *s_label_mode = NULL;
static lv_obj_t *s_bar_speed = NULL;
static lv_obj_t *s_label_distance = NULL;
static lv_obj_t *s_label_wifi = NULL;
static lv_obj_t *s_label_ota = NULL;

/* 屏幕尺寸 */
#define UI_WIDTH   240
#define UI_HEIGHT  240

esp_err_t lvgl_ui_init(void)
{
    /* 初始化 LVGL */
    lv_init();

    /* 初始化显示硬件 */
    hal_display_init();

    /* 创建 LVGL 显示驱动 */
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[DISPLAY_WIDTH * 20];
    static lv_color_t buf2[DISPLAY_WIDTH * 20];
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, DISPLAY_WIDTH * 20);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = DISPLAY_WIDTH;
    disp_drv.ver_res = DISPLAY_HEIGHT;
    disp_drv.flush_cb = hal_display_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /* ==================== 创建 UI 界面 ==================== */

    /* 顶部状态栏背景 */
    lv_obj_t *status_bar = lv_obj_create(lv_screen_active());
    lv_obj_set_size(status_bar, UI_WIDTH, 30);
    lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_border_width(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(status_bar, 0, LV_PART_MAIN);

    /* 电池电压标签 */
    s_label_voltage = lv_label_create(status_bar);
    lv_label_set_text(s_label_voltage, "3.85V");
    lv_obj_set_style_text_color(s_label_voltage, lv_color_hex(0x00FF00), LV_PART_MAIN);
    lv_obj_align(s_label_voltage, LV_ALIGN_LEFT_MID, 5, 0);

    /* WiFi 信号标签 */
    s_label_wifi = lv_label_create(status_bar);
    lv_label_set_text(s_label_wifi, "---");
    lv_obj_set_style_text_color(s_label_wifi, lv_color_hex(0x00BFFF), LV_PART_MAIN);
    lv_obj_align(s_label_wifi, LV_ALIGN_RIGHT_MID, -5, 0);

    /* 模式显示 */
    s_label_mode = lv_label_create(lv_screen_active());
    lv_label_set_text(s_label_mode, "REMOTE");
    lv_obj_set_style_text_color(s_label_mode, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(s_label_mode, LV_ALIGN_TOP_MID, 0, 40);

    /* 速度条 */
    s_bar_speed = lv_bar_create(lv_screen_active());
    lv_obj_set_size(s_bar_speed, 200, 20);
    lv_obj_align(s_bar_speed, LV_ALIGN_TOP_MID, 0, 70);
    lv_bar_set_range(s_bar_speed, 0, 100);
    lv_bar_set_value(s_bar_speed, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar_speed, lv_color_hex(0x00AA00), LV_PART_INDICATOR);

    /* 距离显示 */
    s_label_distance = lv_label_create(lv_screen_active());
    lv_label_set_text(s_label_distance, "Dist: ---cm");
    lv_obj_set_style_text_color(s_label_distance, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
    lv_obj_align(s_label_distance, LV_ALIGN_TOP_MID, 0, 110);

    /* OTA 进度 */
    s_label_ota = lv_label_create(lv_screen_active());
    lv_label_set_text(s_label_ota, "OTA: idle");
    lv_obj_set_style_text_color(s_label_ota, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_align(s_label_ota, LV_ALIGN_BOTTOM_MID, 0, -10);

    ESP_LOGI(TAG, "LVGL UI initialized");
    return ESP_OK;
}

void lvgl_ui_task_handler(uint32_t interval_ms)
{
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(interval_ms));
}

void lvgl_ui_update_voltage(uint16_t voltage_mv)
{
    if (s_label_voltage == NULL) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2fV", voltage_mv / 1000.0f);

    /* 根据电压设置颜色 */
    if (voltage_mv < 3300) {
        lv_obj_set_style_text_color(s_label_voltage, lv_color_hex(0xFF0000), LV_PART_MAIN);
    } else if (voltage_mv < 3500) {
        lv_obj_set_style_text_color(s_label_voltage, lv_color_hex(0xFFAA00), LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(s_label_voltage, lv_color_hex(0x00FF00), LV_PART_MAIN);
    }

    lv_label_set_text(s_label_voltage, buf);
}

void lvgl_ui_update_speed(uint8_t speed)
{
    if (s_bar_speed == NULL) return;
    if (speed > 100) speed = 100;
    lv_bar_set_value(s_bar_speed, speed, LV_ANIM_ON);
}

void lvgl_ui_update_mode(car_mode_t mode)
{
    if (s_label_mode == NULL) return;

    switch (mode) {
    case CAR_MODE_REMOTE:
        lv_label_set_text(s_label_mode, "REMOTE");
        lv_obj_set_style_text_color(s_label_mode, lv_color_hex(0x00BFFF), LV_PART_MAIN);
        break;
    case CAR_MODE_TRACK:
        lv_label_set_text(s_label_mode, "TRACK");
        lv_obj_set_style_text_color(s_label_mode, lv_color_hex(0x00FF00), LV_PART_MAIN);
        break;
    case CAR_MODE_AVOID:
        lv_label_set_text(s_label_mode, "AVOID");
        lv_obj_set_style_text_color(s_label_mode, lv_color_hex(0xFFAA00), LV_PART_MAIN);
        break;
    }
}

void lvgl_ui_update_distance(uint16_t distance_cm)
{
    if (s_label_distance == NULL) return;
    char buf[24];
    snprintf(buf, sizeof(buf), "%hucm", distance_cm);
    lv_label_set_text(s_label_distance, buf);
}

void lvgl_ui_update_wifi_rssi(int8_t rssi)
{
    if (s_label_wifi == NULL) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d dBm", rssi);
    lv_label_set_text(s_label_wifi, buf);
}

void lvgl_ui_update_ota_progress(uint8_t progress)
{
    if (s_label_ota == NULL) return;
    if (progress == 0) {
        lv_label_set_text(s_label_ota, "OTA: idle");
    } else if (progress >= 100) {
        lv_label_set_text(s_label_ota, "OTA: done!");
    } else {
        char buf[24];
        snprintf(buf, sizeof(buf), "OTA: %u%%", progress);
        lv_label_set_text(s_label_ota, buf);
    }
}

uint32_t lvgl_ui_get_handler_interval(void)
{
    return 5;  /* 5ms 刷新周期 */
}