# middleware 组件 - 中间件层
# 职责: OTA 升级、BLE 通信
idf_component_register(
    SRCS "ota_mgr.c" "bt_mgr.c"
    INCLUDE_DIRS "."
    REQUIRES hal kernel app_update esp_https_ota bt
)