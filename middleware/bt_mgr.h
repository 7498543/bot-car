/**
 * @file    bt_mgr.h
 * @brief   BLE 蓝牙管理器 - 简化版遥控通讯
 *
 * 设计原则:
 *   - 仅支持 BLE (低功耗蓝牙)，裁剪 Classic BT SPP
 *   - GATT Service 透传控制指令
 *   - 上位机通过 BLE 发送运动指令控制小车
 *
 * 通信协议:
 *   指令格式: 单字节命令 + 可选参数
 *     'F' + speed   : 前进
 *     'B' + speed   : 后退
 *     'L' + speed   : 左转
 *     'R' + speed   : 右转
 *     'S'           : 停止
 *     'M' + mode    : 切换模式 (0=遥控 1=循迹 2=避障)
 */

#ifndef BT_MGR_H
#define BT_MGR_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* BLE 控制指令 */
#define BT_CMD_FORWARD 'F'  /* 前进 */
#define BT_CMD_BACKWARD 'B' /* 后退 */
#define BT_CMD_LEFT 'L'     /* 左转 */
#define BT_CMD_RIGHT 'R'    /* 右转 */
#define BT_CMD_STOP 'S'     /* 停止 */
#define BT_CMD_MODE 'M'     /* 模式切换 */

    /* BLE 控制指令结构体 */
    typedef struct
    {
        uint8_t command; /* 指令类型 */
        uint8_t param;   /* 参数 (速度/模式) */
    } bt_control_cmd_t;

    /* BLE 数据接收回调 */
    typedef void (*bt_data_cb_t)(const bt_control_cmd_t *cmd);

    /**
     * @brief 初始化 BLE 服务
     * @param cb 数据接收回调
     * @return ESP_OK 成功
     */
    esp_err_t bt_mgr_init(bt_data_cb_t cb);

    /**
     * @brief 启动 BLE 广播
     * @param device_name 设备名称
     * @return ESP_OK 成功
     */
    esp_err_t bt_mgr_start_advertising(const char *device_name);

    /**
     * @brief 停止 BLE 广播
     * @return ESP_OK 成功
     */
    esp_err_t bt_mgr_stop_advertising(void);

#ifdef __cplusplus
}
#endif

#endif /* BT_MGR_H */