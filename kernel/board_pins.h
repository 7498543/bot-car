/**
 * @file    board_pins.h
 * @brief   硬件引脚定义 - 标准化统一管理
 *
 * 适配 ESP32 / ESP32-S3，更换主控仅需修改此文件。
 * 所有外设引脚集中定义，业务代码通过 HAL 接口访问，不直接操作引脚。
 *
 * 引脚分配原则:
 *   - ADC2 与 WiFi 共用，优先使用 ADC1
 *   - 避开 JTAG/Flash 默认引脚 (GPIO 6-11)
 *   - 输入引脚避开 GPIO 34-39 (仅输入，无内部上拉/下拉)
 */

#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "driver/gpio.h"
#include "hal/gpio_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================== 电机驱动 (TB6612/L298N) ========================== */
/* 左电机 (Motor A) */
#define MOTOR_A_PWM_GPIO         GPIO_NUM_16    /* PWM 调速引脚 */
#define MOTOR_A_IN1_GPIO         GPIO_NUM_17    /* 方向控制 IN1 */
#define MOTOR_A_IN2_GPIO         GPIO_NUM_18    /* 方向控制 IN2 */

/* 右电机 (Motor B) */
#define MOTOR_B_PWM_GPIO         GPIO_NUM_21    /* PWM 调速引脚 */
#define MOTOR_B_IN1_GPIO         GPIO_NUM_22    /* 方向控制 IN1 */
#define MOTOR_B_IN2_GPIO         GPIO_NUM_23    /* 方向控制 IN2 */

/* PWM 参数 */
#define MOTOR_PWM_FREQ           5000           /* PWM 频率: 5kHz */
#define MOTOR_PWM_RESOLUTION     10             /* PWM 分辨率: 10bit (0-1023) */
#define MOTOR_PWM_CHANNEL_A      LEDC_CHANNEL_0
#define MOTOR_PWM_CHANNEL_B      LEDC_CHANNEL_1
#define MOTOR_PWM_TIMER          LEDC_TIMER_0
#define MOTOR_PWM_MODE           LEDC_LOW_SPEED_MODE

/* ========================== 超声波测距 (HC-SR04) ========================== */
#define ULTRASONIC_TRIG_GPIO     GPIO_NUM_26    /* 触发引脚 */
#define ULTRASONIC_ECHO_GPIO     GPIO_NUM_27    /* 回响引脚 (仅输入) */

/* ========================== 红外循迹 (TCRT5000 x4) ========================== */
#define IR_TRACK_L1_GPIO         GPIO_NUM_32    /* 左1 (仅输入) */
#define IR_TRACK_L2_GPIO         GPIO_NUM_33    /* 左2 (仅输入) */
#define IR_TRACK_R1_GPIO         GPIO_NUM_34    /* 右1 (仅输入) */
#define IR_TRACK_R2_GPIO         GPIO_NUM_35    /* 右2 (仅输入) */

/* ========================== 屏幕显示 (ST7789 SPI) ========================== */
#define DISPLAY_SPI_HOST         SPI2_HOST      /* SPI 主机 */
#define DISPLAY_MOSI_GPIO        GPIO_NUM_13    /* MOSI */
#define DISPLAY_CLK_GPIO         GPIO_NUM_14    /* SCLK */
#define DISPLAY_CS_GPIO          GPIO_NUM_15    /* 片选 */
#define DISPLAY_DC_GPIO          GPIO_NUM_2     /* 数据/命令 */
#define DISPLAY_RST_GPIO         GPIO_NUM_4     /* 复位 */
#define DISPLAY_BL_GPIO          GPIO_NUM_5     /* 背光 (PWM) */

/* 屏幕参数 */
#define DISPLAY_WIDTH            240            /* 分辨率 240x240 */
#define DISPLAY_HEIGHT           240
#define DISPLAY_PIXEL_CLOCK_HZ   (40 * 1000 * 1000)  /* 40MHz */
#define DISPLAY_BL_PWM_CHANNEL   LEDC_CHANNEL_2
#define DISPLAY_BL_PWM_TIMER     LEDC_TIMER_1

/* ========================== 电源监测 (ADC) ========================== */
#define POWER_ADC_CHANNEL        ADC_CHANNEL_0  /* ADC1_CH0 -> GPIO36 */
#define POWER_ADC_UNIT           ADC_UNIT_1
#define POWER_ADC_ATTEN          ADC_ATTEN_DB_11
#define POWER_VOLTAGE_DIVIDER    2.0f           /* 分压比 R1/(R1+R2) */

/* ========================== 状态 LED ========================== */
#define LED_STATUS_GPIO          GPIO_NUM_12    /* 运行状态指示灯 */
#define LED_WIFI_GPIO            GPIO_NUM_25    /* WiFi 连接指示灯 */

/* ========================== 按键 ========================== */
#define BUTTON_BOOT_GPIO         GPIO_NUM_0     /* BOOT 按键 (多功能复用) */

#ifdef __cplusplus
}
#endif

#endif /* BOARD_PINS_H */