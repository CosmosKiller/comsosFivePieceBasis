idf_component_register(SRCS "cosmos_battery_adc.cpp" "cosmos_battery_task.cpp" "cosmos_battery_matter.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_adc driver esp_timer esp_matter)

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
