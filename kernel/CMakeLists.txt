# kernel 组件 - 内核层
# 职责: 引脚定义、电源管理、芯片兼容适配
idf_component_register(
    SRCS "power_mgr.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_adc
)