# 根 CMakeLists.txt
# 四层分层架构：kernel(内核层) -> hal(硬件抽象层) -> middleware(中间件层) -> app(应用层)
cmake_minimum_required(VERSION 3.16)

# 项目名称
set(PROJECT_NAME "car")

# kernel: 引脚定义、电源管理
# hal: 硬件抽象接口（依赖 kernel）
# middleware: OTA/NVS/WiFi/BLE 服务（依赖 hal）
# app: 业务逻辑 + LVGL 界面（依赖 middleware）
set(EXTRA_COMPONENT_DIRS
    ${CMAKE_SOURCE_DIR}/kernel
    ${CMAKE_SOURCE_DIR}/hal
    ${CMAKE_SOURCE_DIR}/middleware
    ${CMAKE_SOURCE_DIR}/app
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${PROJECT_NAME})