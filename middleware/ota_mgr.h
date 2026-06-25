/**
 * @file    ota_mgr.h
 * @brief   OTA 固件升级管理器 - 远程下载 + 校验 + 自动回滚
 *
 * 升级流程:
 *   1. 通过 WiFi 从固件服务器下载新固件
 *   2. 写入非活跃 OTA 分区
 *   3. 校验固件完整性 (SHA256)
 *   4. 标记新分区为启动分区
 *   5. 重启后 bootloader 自动切换，失败则回滚
 *
 * 防变砖机制:
 *   - 双 OTA 分区 (ota_0/ota_1) 互为备份
 *   - CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y
 *   - 新固件启动后需调用 ota_mgr_confirm() 确认，否则自动回滚
 */

#ifndef OTA_MGR_H
#define OTA_MGR_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* OTA 升级状态 */
typedef enum {
    OTA_STATE_IDLE = 0,       /* 空闲 */
    OTA_STATE_CHECKING,       /* 检查更新中 */
    OTA_STATE_DOWNLOADING,    /* 下载中 */
    OTA_STATE_INSTALLING,     /* 安装中 */
    OTA_STATE_SUCCESS,        /* 升级成功 */
    OTA_STATE_FAILED,         /* 升级失败 */
} ota_state_t;

/* OTA 进度回调 */
typedef void (*ota_progress_cb_t)(ota_state_t state, uint8_t progress);

/**
 * @brief 初始化 OTA 管理器
 * @param cb 进度回调函数 (可选, 传 NULL 则不回调)
 * @return ESP_OK 成功
 */
esp_err_t ota_mgr_init(ota_progress_cb_t cb);

/**
 * @brief 检查并执行固件升级
 * @param firmware_url 固件下载 URL
 * @return ESP_OK 升级成功并准备重启
 */
esp_err_t ota_mgr_upgrade(const char *firmware_url);

/**
 * @brief 确认当前固件有效 (新固件启动后调用)
 * @return ESP_OK 成功
 */
esp_err_t ota_mgr_confirm(void);

/**
 * @brief 获取当前 OTA 状态
 * @return 当前状态枚举
 */
ota_state_t ota_mgr_get_state(void);

/**
 * @brief 获取当前固件版本号
 * @param[out] version 版本号字符串缓冲区
 * @param[in]  max_len 缓冲区大小
 * @return ESP_OK 成功
 */
esp_err_t ota_mgr_get_version(char *version, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* OTA_MGR_H */