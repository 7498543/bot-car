# middleware 组件 - 中间件层
# 职责: OTA 升级、NVS 存储、WiFi 通信、BLE 通信、通信协议
# 合并了原"生态层"的 BT/WiFi 上位机控制功能
idf_component_register(
    SRCS "ota_mgr.c" "nvs_mgr.c" "wifi_mgr.c" "bt_mgr.c" "comm_proto.c"
    INCLUDE_DIRS "."
    REQUIRES hal kernel nvs_flash app_update esp_https_ota esp_wifi bt
)