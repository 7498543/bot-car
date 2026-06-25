# 技术文档

> 基于 ESP-IDF v5.x + FreeRTOS + LVGL 的四层分层软件架构。
> 模块化硬件设计，支持主板/屏幕/摄像头/电机/电源无缝升级兼容。

---

## 目录

1. [架构设计](#1-架构设计)
2. [硬件引脚定义](#2-硬件引脚定义)
3. [工程目录结构](#3-工程目录结构)
4. [HAL 硬件抽象接口](#4-hal-硬件抽象接口)
5. [LVGL 人机交互界面](#5-lvgl-人机交互界面)
6. [通信协议](#6-通信协议)
7. [OTA 固件升级](#7-ota-固件升级)
8. [编译与烧录](#8-编译与烧录)
9. [硬件升级兼容性](#9-硬件升级兼容性)
10. [精简裁剪记录](#10-精简裁剪记录)

---

## 1. 架构设计

### 1.1 分层架构

```
┌─────────────────────────────────────────────────────┐
│  app/          应用层 (运行层)                        │
│  car_control  小车控制逻辑 (遥控/循迹/避障)            │
│  lvgl_ui      LVGL 人机界面 (电压/速度/模式/OTA)       │
│  app_main     统一初始化入口 + FreeRTOS 任务调度        │
├─────────────────────────────────────────────────────┤
│  middleware/   中间件层 (原生态层合并)                  │
│  ota_mgr      固件远程升级 + 校验 + 自动回滚            │
│  nvs_mgr      NVS 参数持久化存储                      │
│  wifi_mgr      WiFi STA 连接 + 自动重连               │
│  bt_mgr        BLE 透传遥控 (精简 Classic BT)          │
│  comm_proto    统一通信协议 (帧格式 + 校验)             │
├─────────────────────────────────────────────────────┤
│  hal/          HAL 硬件抽象层                         │
│  hal_motor     电机驱动 (PWM 差速转向)                 │
│  hal_display   ST7789 SPI 屏幕 + LVGL 刷新回调        │
│  hal_sensor    超声波测距 + 红外循迹                   │
│  hal_camera    摄像头接口 stub (编译期可选)             │
├─────────────────────────────────────────────────────┤
│  kernel/       内核层                                 │
│  board_pins    统一引脚定义 (30+ 宏)                   │
│  power_mgr     ADC 电压采集 + 低电量告警               │
├─────────────────────────────────────────────────────┤
│  main/         程序入口 (app_main)                     │
│  ESP-IDF + FreeRTOS + LVGL 底层运行时                 │
└─────────────────────────────────────────────────────┘
```

### 1.2 依赖关系

```
main ──> app ──> middleware ──> hal ──> kernel ──> ESP-IDF
                   (单向依赖，上层不感知底层实现)
```

### 1.3 设计原则

| 原则 | 说明 |
|------|------|
| 单向依赖 | 上层仅依赖直接下层，禁止反向依赖 |
| 接口隔离 | 业务代码通过 HAL 接口访问硬件，不直接操作 GPIO/SPI/I2C |
| 编译期裁剪 | 摄像头通过 `#if CONFIG_ESP_CAMERA_OV2640` 可选编译 |
| 硬件解耦 | 更换外设仅修改 HAL 层单文件，业务代码零改动 |
| 最小复杂度 | 34 个源文件，无冗余模块，无过度抽象 |

---

## 2. 硬件引脚定义

> 所有引脚集中定义在 `kernel/board_pins.h`，更换主控仅修改此文件。

### 2.1 引脚分配总表

| 外设 | 信号 | GPIO | 类型 | 备注 |
|------|------|------|------|------|
| **左电机** | PWM_A | 16 | Output | LEDC PWM 调速 |
| | IN1 | 17 | Output | 方向控制 |
| | IN2 | 18 | Output | 方向控制 |
| **右电机** | PWM_B | 21 | Output | LEDC PWM 调速 |
| | IN1 | 22 | Output | 方向控制 |
| | IN2 | 23 | Output | 方向控制 |
| **超声波** | TRIG | 26 | Output | 10us 触发脉冲 |
| | ECHO | 27 | Input | 回响信号 |
| **红外循迹** | L1 | 32 | Input | 左1 (仅输入) |
| | L2 | 33 | Input | 左2 (仅输入) |
| | R1 | 34 | Input | 右1 (仅输入) |
| | R2 | 35 | Input | 右2 (仅输入) |
| **ST7789 屏幕** | MOSI | 13 | Output | SPI 数据 |
| | CLK | 14 | Output | SPI 时钟 |
| | CS | 15 | Output | 片选 |
| | DC | 2 | Output | 数据/命令 |
| | RST | 4 | Output | 硬件复位 |
| | BL | 5 | Output | 背光 PWM |
| **电源监测** | ADC | 36 | Input | ADC1_CH0 (SVP) |
| **状态 LED** | STATUS | 12 | Output | 运行指示灯 |
| | WIFI | 25 | Output | WiFi 指示灯 |
| **按键** | BOOT | 0 | Input | 多功能复用 |

### 2.2 PWM 资源配置

| 资源 | 用途 | 参数 |
|------|------|------|
| LEDC_TIMER_0 | 电机 PWM | 5kHz, 10bit (0-1023) |
| LEDC_CHANNEL_0 | 左电机 | 绑定 TIMER_0 |
| LEDC_CHANNEL_1 | 右电机 | 绑定 TIMER_0 |
| LEDC_TIMER_1 | 背光 PWM | 5kHz, 10bit |
| LEDC_CHANNEL_2 | 屏幕背光 | 绑定 TIMER_1 |

### 2.3 引脚分配原则

- **ADC2 与 WiFi 冲突**：电源监测使用 ADC1_CH0 (GPIO36)，避免 WiFi 干扰
- **避开 Flash 引脚**：不使用 GPIO 6-11 (ESP32 内部 Flash 占用)
- **仅输入引脚**：GPIO 34-39 无内部上拉/下拉，仅分配给红外传感器输入

---

## 3. 工程目录结构

```
smart_car/
├── CMakeLists.txt                  # 根构建文件，注册 4 个组件
├── partitions.csv                  # OTA 双分区表 (4MB Flash)
├── sdkconfig.defaults              # SDK 预设 (WiFi/BLE/LVGL/OTA)
├── 1.md                            # 需求初稿
├── docs/                           # 技术文档
│   └── technical-spec.md           # 本文档
│
├── main/                           # 程序入口
│   ├── CMakeLists.txt              # 依赖 app 组件
│   └── main.c                      # app_main() 入口
│
├── kernel/                         # 内核层：引脚定义 + 电源管理
│   ├── CMakeLists.txt              # 依赖 driver, esp_adc
│   ├── board_pins.h                # 30+ 引脚宏定义
│   ├── power_mgr.h                 # 电源管理接口
│   └── power_mgr.c                 # ADC 电压采集实现
│
├── hal/                            # 硬件抽象层：统一外设接口
│   ├── CMakeLists.txt              # 依赖 kernel, driver, esp_lcd, lvgl
│   ├── hal_types.h                 # 通用类型 (方向/传感器状态)
│   ├── hal_motor.h                 # 电机 HAL 接口
│   ├── hal_motor.c                 # TB6612 PWM 驱动实现
│   ├── hal_display.h               # 屏幕 HAL 接口
│   ├── hal_display.c               # ST7789 SPI + LVGL flush 回调
│   ├── hal_sensor.h                # 传感器 HAL 接口
│   ├── hal_sensor.c                # HC-SR04 + TCRT5000 实现
│   └── hal_camera.h                # 摄像头 stub (编译期可选)
│
├── middleware/                      # 中间件层：通信 + 存储 + OTA
│   ├── CMakeLists.txt              # 依赖 hal, nvs_flash, app_update, wifi, bt
│   ├── nvs_mgr.h / nvs_mgr.c       # NVS 参数持久化 (6 个存储键)
│   ├── ota_mgr.h / ota_mgr.c       # OTA 升级 + 自动回滚
│   ├── wifi_mgr.h / wifi_mgr.c     # WiFi STA + 自动重连 (5 次)
│   ├── bt_mgr.h / bt_mgr.c         # BLE GATT 透传遥控
│   └── comm_proto.h / comm_proto.c # 统一通信协议 (帧格式 + XOR 校验)
│
└── app/                            # 应用层：业务逻辑 + UI
    ├── CMakeLists.txt              # 依赖 middleware, hal, lvgl
    ├── car_control.h / car_control.c # 遥控/循迹/避障 三种模式
    ├── lvgl_ui.h / lvgl_ui.c        # LVGL 界面 (5 个 UI 组件)
    └── app_main.h / app_main.c      # 统一初始化 + 2 个 FreeRTOS 任务
```

---

## 4. HAL 硬件抽象接口

### 4.1 电机 HAL — `hal_motor.h`

```c
esp_err_t hal_motor_init(void);
esp_err_t hal_motor_set_speed(int8_t left_speed, int8_t right_speed);  // -100~100
esp_err_t hal_motor_move(motor_direction_t direction, uint8_t speed);   // 前进/后退/左转/右转/停止
esp_err_t hal_motor_stop(void);
```

| 函数 | 说明 |
|------|------|
| `hal_motor_init()` | 初始化 LEDC PWM + GPIO 方向引脚 |
| `hal_motor_set_speed(L, R)` | 差速驱动，负值=反转，绝对值=速度百分比 |
| `hal_motor_move(DIR, SPD)` | 封装的方向控制，LEFT/RIGHT 内部差速实现 |
| `hal_motor_stop()` | 紧急停止，PWM 归零 + 方向引脚拉低 |

### 4.2 屏幕 HAL — `hal_display.h`

```c
esp_err_t hal_display_init(void);
esp_err_t hal_display_set_backlight(uint8_t brightness);  // 0-100
void hal_display_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);
```

| 函数 | 说明 |
|------|------|
| `hal_display_init()` | 初始化 SPI 总线 + ST7789 面板 + 背光 PWM |
| `hal_display_set_backlight()` | 调节背光亮度 (10bit PWM) |
| `hal_display_flush_cb()` | LVGL 刷新回调，通过 `esp_lcd_panel_draw_bitmap()` 传输 |

### 4.3 传感器 HAL — `hal_sensor.h`

```c
esp_err_t hal_sensor_init(void);
esp_err_t hal_sensor_get_distance(uint16_t *distance_cm);  // 超声波测距
esp_err_t hal_sensor_get_track(ir_track_state_t *state);    // 红外循迹
```

| 函数 | 说明 |
|------|------|
| `hal_sensor_init()` | 初始化超声波 (TRIG/ECHO) + 红外 (4路输入) |
| `hal_sensor_get_distance()` | HC-SR04 测距，超时返回 0，单位 cm |
| `hal_sensor_get_track()` | 读取四路红外，0=黑线/1=白底 |

### 4.4 摄像头 HAL — `hal_camera.h`

```c
#if CONFIG_ESP_CAMERA_OV2640
esp_err_t hal_camera_init(void);
esp_err_t hal_camera_capture(uint8_t **buf, size_t *len);
esp_err_t hal_camera_release(void);
#else
// 空实现 stub，返回 ESP_ERR_NOT_SUPPORTED
#endif
```

> 当前版本未启用摄像头。如需启用，在 `sdkconfig.defaults` 中开启 `CONFIG_ESP_CAMERA_OV2640=y`。

### 4.5 扩展新外设指南

1. 在 `hal/` 下新建 `hal_xxx.h` 和 `hal_xxx.c`
2. 在 `kernel/board_pins.h` 中添加引脚定义
3. 在 `hal/CMakeLists.txt` 的 `SRCS` 中添加 `hal_xxx.c`
4. 业务代码通过 `#include "hal_xxx.h"` 调用，**无需修改任何其他文件**

---

## 5. LVGL 人机交互界面

### 5.1 界面布局

```
┌──────────────────────────┐
│  🔋 3.85V      📶 -45dBm │  ← 顶部状态栏
│                          │
│       🚗 REMOTE         │  ← 模式显示 (颜色区分)
│     ████████░░ 80%      │  ← 速度条 (lv_bar)
│                          │
│     Dist: 128cm         │  ← 超声波距离
│                          │
│     OTA: idle           │  ← OTA 升级状态
└──────────────────────────┘
        240×240 ST7789
```

### 5.2 UI 更新 API — `lvgl_ui.h`

| 函数 | 刷新频率 | 说明 |
|------|----------|------|
| `lvgl_ui_update_voltage(mV)` | 100ms | 电压值 + 颜色 (绿/橙/红) |
| `lvgl_ui_update_speed(%)` | 100ms | 速度条动画 |
| `lvgl_ui_update_mode(mode)` | 模式切换时 | 模式文字 + 颜色 |
| `lvgl_ui_update_distance(cm)` | 100ms | 超声波距离 |
| `lvgl_ui_update_wifi_rssi(dBm)` | 1s | WiFi 信号强度 |
| `lvgl_ui_update_ota_progress(%)` | OTA 时 | 升级进度 |

### 5.3 LVGL 配置

| 参数 | 值 | 说明 |
|------|-----|------|
| 色深 | 16bit (RGB565) | `CONFIG_LV_COLOR_DEPTH=16` |
| 内存 | 64KB | `CONFIG_LV_MEM_SIZE_KILOBYTES=64` |
| 缓冲区 | 240×20 ×2 | 双缓冲 1/10 屏幕行 |
| 刷新周期 | 5ms | `lv_timer_handler()` 调用间隔 |
| Tick | FreeRTOS | `xTaskGetTickCount() * portTICK_PERIOD_MS` |

---

## 6. 通信协议

### 6.1 协议帧格式 — `comm_proto.h`

```
┌────────┬────────┬────────┬──────────────┬──────────┐
│ Header │  Cmd   │  Len   │   Payload    │ Checksum │
│ 0xAA   │ 1 byte │ 1 byte │  0~28 bytes  │ 1 byte   │
│ 1 byte │        │        │              │ (XOR)    │
└────────┴────────┴────────┴──────────────┴──────────┘
```

### 6.2 命令字定义

| 命令 | 值 | 方向 | 说明 |
|------|-----|------|------|
| `CMD_CAR_CONTROL` | 0x01 | 上位机→小车 | 运动控制 (F/B/L/R/S) |
| `CMD_CAR_SPEED` | 0x02 | 上位机→小车 | 速度设置 0-100 |
| `CMD_CAR_MODE` | 0x03 | 上位机→小车 | 模式切换 (0=遥控/1=循迹/2=避障) |
| `CMD_STATUS_QUERY` | 0x10 | 上位机→小车 | 查询状态 |
| `CMD_STATUS_REPORT` | 0x11 | 小车→上位机 | 上报状态 (电压/速度/模式/信号) |
| `CMD_OTA_TRIGGER` | 0x20 | 上位机→小车 | 触发 OTA 升级 |
| `CMD_OTA_PROGRESS` | 0x21 | 小车→上位机 | OTA 进度上报 |
| `CMD_CONFIG_SET` | 0x30 | 上位机→小车 | 设置配置参数 |
| `CMD_CONFIG_GET` | 0x31 | 上位机→小车 | 读取配置参数 |

### 6.3 BLE 简化指令

> BLE 模式下使用单字节指令 + 单字节参数，无需完整协议帧：

| 指令 | 字符 | 参数 | 示例 |
|------|------|------|------|
| 前进 | `'F'` | 速度 0-100 | `F` `50` |
| 后退 | `'B'` | 速度 0-100 | `B` `30` |
| 左转 | `'L'` | 速度 0-100 | `L` `40` |
| 右转 | `'R'` | 速度 0-100 | `R` `40` |
| 停止 | `'S'` | 无 | `S` `0` |
| 模式 | `'M'` | 0/1/2 | `M` `1` |

---

## 7. OTA 固件升级

### 7.1 分区表 (4MB Flash)

| 分区名 | 类型 | 偏移 | 大小 | 说明 |
|--------|------|------|------|------|
| nvs | data | 0x9000 | 24KB | 参数存储 |
| otadata | data | 0xF000 | 8KB | OTA 状态标记 |
| phy_init | data | 0x11000 | 4KB | PHY 初始化数据 |
| **ota_0** | app | 0x20000 | 1.875MB | 主固件分区 |
| **ota_1** | app | 0x200000 | 1.875MB | 备份固件分区 |

### 7.2 升级流程

```
1. 检查更新  ──>  HTTP GET 固件 URL
2. 下载固件  ──>  写入非活跃 OTA 分区
3. 校验固件  ──>  SHA256 完整性校验
4. 切换分区  ──>  设置新分区为启动分区
5. 重启      ──>  esp_restart()
6. 确认固件  ──>  新固件启动后调用 ota_mgr_confirm()
                 └── 未确认则 bootloader 自动回滚到旧分区
```

### 7.3 防变砖机制

| 机制 | 实现 |
|------|------|
| 双分区备份 | ota_0 / ota_1 互为备份，始终保留一个可用固件 |
| 自动回滚 | `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` |
| 固件确认 | `ota_mgr_confirm()` 取消回滚标记 |
| 分区隔离 | NVS / otadata 独立分区，固件损坏不影响参数 |

### 7.4 OTA API — `ota_mgr.h`

```c
esp_err_t ota_mgr_init(ota_progress_cb_t cb);
esp_err_t ota_mgr_upgrade(const char *firmware_url);
esp_err_t ota_mgr_confirm(void);
ota_state_t ota_mgr_get_state(void);
esp_err_t ota_mgr_get_version(char *version, size_t max_len);
```

---

## 8. 编译与烧录

### 8.1 环境要求

| 组件 | 版本 |
|------|------|
| ESP-IDF | v5.0+ (推荐 v5.2) |
| Python | 3.8+ |
| CMake | 3.16+ |
| 目标芯片 | ESP32 (兼容 ESP32-S3) |

### 8.2 一键编译

```bash
# 1. 激活 ESP-IDF 环境
cd d:\Code\ca
%IDF_PATH%\export.bat

# 2. 设置目标芯片
idf.py set-target esp32

# 3. 编译
idf.py build

# 4. 烧录 + 串口监视
idf.py -p COM3 flash monitor
```

### 8.3 切换 ESP32-S3

```bash
idf.py set-target esp32s3
# 修改 kernel/board_pins.h 中的引脚定义 (S3 引脚与 ESP32 不同)
idf.py build flash monitor
```

### 8.4 配置调整

```bash
idf.py menuconfig
# 可调整: WiFi 参数、BLE 参数、LVGL 内存、日志级别等
```

---

## 9. 硬件升级兼容性

### 9.1 更换场景与影响范围

| 更换硬件 | 影响范围 | 修改文件 | 业务代码改动 |
|----------|----------|----------|-------------|
| 主控芯片 (ESP32→S3) | 内核层 | `board_pins.h` | 零改动 |
| 电机驱动板 (TB6612→L298N) | HAL 层 | `hal_motor.c` | 零改动 |
| 屏幕型号 (ST7789→ILI9341) | HAL 层 | `hal_display.c` | 零改动 |
| 超声波模块 (HC-SR04→JSN-SR04T) | HAL 层 | `hal_sensor.c` | 零改动 |
| 新增传感器/外设 | HAL 层 | 新增 hal_xxx.h/c | 零改动 |
| 新增通信方式 (4G/LoRa) | 中间件层 | 新增 middleware/xxx_mgr | 极小改动 |

### 9.2 解耦验证

```
app/car_control.c  ──调用──>  hal_motor.h    (电机接口)
                              hal_sensor.h   (传感器接口)
                              hal_display.h  (屏幕接口)

app/car_control.c  不依赖──>  driver/gpio.h   (无直接 GPIO 操作)
                              driver/ledc.h   (无直接 PWM 操作)
                              driver/spi.h    (无直接 SPI 操作)
                              kernel/board_pins.h  (无引脚宏引用)
```

---

## 10. 精简裁剪记录

### 10.1 裁剪决策

| 原需求 | 决策 | 原因 |
|--------|------|------|
| 四层架构 (含生态层) | 合并为三层 | 生态层 (BT/WiFi) 本质是通信中间件，并入 middleware |
| 摄像头 OV2640 | 编译期可选 | 基础循迹避障不需要摄像头，保留 stub 接口 |
| Classic BT SPP | 裁剪 | 仅 BLE 即可满足遥控，`CONFIG_BT_CLASSIC_ENABLED=n` |
| 深度休眠管理 | 裁剪 | 小车场景实时运行，仅保留 ADC 电压采集 |
| NVS 加密 | 裁剪 | 小车场景无需加密存储 |
| WiFi AP 模式 | 裁剪 | 仅保留 STA 模式，连接路由器进行 OTA 升级 |
| 性能监控 (FreeRTOS) | 裁剪 | 调试阶段不需要 |
| LVGL 性能监控 | 裁剪 | 发布版本不需要 |

### 10.2 复杂度统计

| 指标 | 数值 |
|------|------|
| 源文件总数 | 34 |
| .h 头文件 | 15 |
| .c 实现文件 | 13 |
| CMakeLists.txt | 6 |
| 总代码行数 | ~1800 行 |
| FreeRTOS 任务数 | 2 (ui_task + car_task) |
| 最大调用深度 | 4 (main → app → middleware → hal → kernel) |

---

> **版本**: v1.0 | **最后更新**: 2026-06-25 | **目标芯片**: ESP32 / ESP32-S3
