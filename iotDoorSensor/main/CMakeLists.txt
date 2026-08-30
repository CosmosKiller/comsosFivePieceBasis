# Main component: application sources and task headers (../tasks)

set(MAIN_SRCS
    "main.cpp"
    "matter_task.cpp"
    "binary_sensor_task.cpp"
    "evt_service_task.c"
    "panic_alarm_task.c")

# Custom device info provider: chip excludes ESP32DeviceInfoProvider.cpp; link base + static impl here.
if(CONFIG_CUSTOM_DEVICE_INFO_PROVIDER AND CONFIG_SUPPORT_FIXED_LABEL_CLUSTER)
    list(APPEND MAIN_SRCS
         "$ENV{ESP_MATTER_PATH}/connectedhomeip/connectedhomeip/src/platform/ESP32/ESP32DeviceInfoProvider.cpp"
         "$ENV{ESP_MATTER_PATH}/connectedhomeip/connectedhomeip/src/platform/ESP32/StaticESP32DeviceInfoProvider.cpp")
endif()

idf_component_register(SRCS ${MAIN_SRCS}
                       INCLUDE_DIRS "." "../tasks"
                       REQUIRES esp_matter cosmos_matter_common cosmos_battery)

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
