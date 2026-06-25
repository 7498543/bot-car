/**
 * @file    main.c
 * @brief   程序入口
 *
 * 架构分层:
 *   kernel/   -> 内核层: 引脚定义、电源管理
 *   hal/      -> 硬件抽象层: 电机/屏幕/传感器统一接口
 *   middleware/-> 中间件层: NVS/OTA/WiFi/BLE/通信协议
 *   app/      -> 应用层: 控制逻辑/LVGL界面/任务调度
 *
 * 启动流程: app_main_init() 逐层初始化 -> FreeRTOS 任务调度
 */

#include "app_main.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 Smart Car Bootloader");

    /* 统一初始化入口，由 app_main_init() 逐层启动 */
    esp_err_t ret = app_main_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    /* 主循环由 FreeRTOS 任务接管，此函数不再返回 */
    ESP_LOGI(TAG, "System running, FreeRTOS scheduler active");
}