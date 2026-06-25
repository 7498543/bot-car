/**
 * @file    hal_display.c
 * @brief   显示 HAL 实现 - ST7789 240x240 SPI
 *
 * 使用 ESP-IDF 官方 esp_lcd 组件驱动 ST7789。
 * 更换 ILI9341 等屏幕仅需替换 esp_lcd_new_panel_xxx() 调用。
 */

#include "hal_display.h"
#include "board_pins.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"

static const char *TAG = "hal_display";

static esp_lcd_panel_handle_t s_panel = NULL;

/* LVGL 显示缓冲区 (1/10 屏幕行，节省内存) */
#define LVGL_BUF_SIZE (DISPLAY_WIDTH * 20)
static lv_color_t s_disp_buf1[LVGL_BUF_SIZE];
static lv_color_t s_disp_buf2[LVGL_BUF_SIZE];

esp_err_t hal_display_init(void)
{
    /* 初始化背光 PWM */
    ledc_timer_config_t bl_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = DISPLAY_BL_PWM_TIMER,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&bl_timer);

    ledc_channel_config_t bl_ch = {
        .gpio_num = DISPLAY_BL_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = DISPLAY_BL_PWM_CHANNEL,
        .timer_sel = DISPLAY_BL_PWM_TIMER,
        .duty = 512,  /* 50% 亮度 */
        .hpoint = 0,
    };
    ledc_channel_config(&bl_ch);

    /* 配置 SPI 总线 */
    spi_bus_config_t spi_cfg = {
        .mosi_io_num = DISPLAY_MOSI_GPIO,
        .miso_io_num = -1,          /* 屏幕不需要 MISO */
        .sclk_io_num = DISPLAY_CLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(DISPLAY_SPI_HOST, &spi_cfg, SPI_DMA_CH_AUTO));

    /* 配置 LCD Panel IO */
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = DISPLAY_CS_GPIO,
        .dc_gpio_num = DISPLAY_DC_GPIO,
        .spi_mode = 0,
        .pclk_hz = DISPLAY_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)DISPLAY_SPI_HOST,
                                              &io_cfg, &io_handle));

    /* 创建 ST7789 Panel */
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = DISPLAY_RST_GPIO,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,   /* ST7789 默认 BGR */
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_cfg, &s_panel));

    /* 初始化面板 */
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    ESP_LOGI(TAG, "ST7789 display initialized (%dx%d)", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    return ESP_OK;
}

esp_err_t hal_display_set_backlight(uint8_t brightness)
{
    if (brightness > 100) brightness = 100;
    uint32_t duty = (uint32_t)(brightness * 1023 / 100);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, DISPLAY_BL_PWM_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, DISPLAY_BL_PWM_CHANNEL);
    return ESP_OK;
}

void hal_display_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    if (s_panel == NULL) return;

    int x1 = area->x1, y1 = area->y1;
    int x2 = area->x2, y2 = area->y2;

    esp_lcd_panel_draw_bitmap(s_panel, x1, y1, x2 + 1, y2 + 1, (const void *)color_map);
    lv_disp_flush_ready(drv);
}