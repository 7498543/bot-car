# main 组件 - 程序入口
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES app
)