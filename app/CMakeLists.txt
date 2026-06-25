# app 组件 - 应用层
# 职责: 小车业务逻辑 (遥控/循迹/避障) + LVGL 人机界面
idf_component_register(
    SRCS "car_control.c" "lvgl_ui.c" "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES middleware hal kernel lvgl
)