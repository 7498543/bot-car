/**
 * @file    comm_proto.h
 * @brief   通信协议定义 - 统一的上位机通信协议
 *
 * 设计原则:
 *   - WiFi (TCP/UDP) 和 BLE 共用同一套协议格式
 *   - 协议帧: [Header 0xAA] [Cmd] [Len] [Payload...] [Checksum]
 *   - 简化版: 固定长度帧，减少解析复杂度
 */

#ifndef COMM_PROTO_H
#define COMM_PROTO_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 协议常量 */
#define PROTO_FRAME_HEADER     0xAA   /* 帧头 */
#define PROTO_FRAME_MAX_LEN    32     /* 最大帧长度 */
#define PROTO_CHECKSUM_SEED    0x00   /* 校验和初始值 */

/* ======================== 协议帧结构 ======================== */
typedef struct __attribute__((packed)) {
    uint8_t  header;       /* 帧头: 0xAA */
    uint8_t  command;      /* 命令字 */
    uint8_t  length;       /* 数据长度 */
    uint8_t  data[PROTO_FRAME_MAX_LEN - 4];  /* 数据 */
    uint8_t  checksum;     /* 校验和 */
} proto_frame_t;

/* ======================== 命令字定义 ======================== */
typedef enum {
    CMD_CAR_CONTROL   = 0x01,  /* 小车控制 (前进/后退/转向/停止) */
    CMD_CAR_SPEED     = 0x02,  /* 速度设置 */
    CMD_CAR_MODE      = 0x03,  /* 模式切换 (遥控/循迹/避障) */
    CMD_STATUS_QUERY  = 0x10,  /* 状态查询 */
    CMD_STATUS_REPORT = 0x11,  /* 状态上报 */
    CMD_OTA_TRIGGER   = 0x20,  /* 触发 OTA 升级 */
    CMD_OTA_PROGRESS  = 0x21,  /* OTA 进度上报 */
    CMD_CONFIG_SET    = 0x30,  /* 配置设置 */
    CMD_CONFIG_GET    = 0x31,  /* 配置获取 */
} proto_command_t;

/* ======================== 控制参数 ======================== */
typedef enum {
    CAR_MODE_REMOTE  = 0,  /* 遥控模式 */
    CAR_MODE_TRACK   = 1,  /* 循迹模式 */
    CAR_MODE_AVOID   = 2,  /* 避障模式 */
} car_mode_t;

/* 状态上报数据结构 */
typedef struct __attribute__((packed)) {
    uint16_t voltage_mv;     /* 电池电压 (mV) */
    uint8_t  speed;          /* 当前速度 */
    uint8_t  mode;           /* 当前模式 */
    int8_t   wifi_rssi;      /* WiFi 信号强度 */
    uint8_t  ota_progress;   /* OTA 进度 */
} status_report_t;

/* ======================== API 接口 ======================== */

/**
 * @brief 构建协议帧
 * @param[out] frame   输出帧
 * @param[in]  command 命令字
 * @param[in]  data    数据
 * @param[in]  len     数据长度
 * @return ESP_OK 成功
 */
esp_err_t proto_build_frame(proto_frame_t *frame, proto_command_t command,
                            const uint8_t *data, uint8_t len);

/**
 * @brief 解析协议帧
 * @param[in]  raw_data 原始数据
 * @param[in]  raw_len  原始数据长度
 * @param[out] frame    解析后的帧
 * @return ESP_OK 成功, ESP_ERR_INVALID_CRC 校验失败
 */
esp_err_t proto_parse_frame(const uint8_t *raw_data, uint8_t raw_len,
                            proto_frame_t *frame);

/**
 * @brief 计算校验和
 * @param data 数据
 * @param len  长度
 * @return 校验和
 */
uint8_t proto_calc_checksum(const uint8_t *data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* COMM_PROTO_H */