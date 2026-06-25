# hal 组件 - 硬件抽象层
# 职责: 统一外设接口，硬件解耦。新增外设仅扩展此层，业务代码无需修改。
idf_component_register(
    SRCS "hal_motor.c" "hal_display.c" "hal_sensor.c"
    INCLUDE_DIRS "."
    REQUIRES kernel driver esp_lcd lvgl
)