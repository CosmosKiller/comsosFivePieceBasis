# Component CMake for lilFlowerPal 'src' component
# Collect all C/C++ sources in this directory and export needed include dirs

idf_component_register(SRCS "main.cpp""matter_task.cpp" "binary_sensor_task.cpp" "factory_reset_task.cpp" "evt_service_task.c" "panic_alarm_task.c"
                       INCLUDE_DIRS "." "../tasks"
                       REQUIRES esp_matter)

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")