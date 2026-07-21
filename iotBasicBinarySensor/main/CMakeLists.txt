# Main component: application sources and task headers (../tasks)

idf_component_register(SRCS "main.cpp" "matter_task.cpp" "binary_sensor_task.cpp" "evt_service_task.c" "panic_alarm_task.c"
                       INCLUDE_DIRS "." "../tasks"
                       REQUIRES esp_matter cosmos_matter_common)

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")