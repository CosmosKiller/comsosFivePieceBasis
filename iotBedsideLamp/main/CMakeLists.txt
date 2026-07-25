# Main component: application sources and task headers (../tasks)

idf_component_register(SRCS "main.cpp" "matter_task.cpp" "lamp_task.cpp" "led_effects_task.cpp" "user_button_task.cpp"
                       INCLUDE_DIRS "." "../tasks"
                       REQUIRES esp_matter cosmos_matter_common device led_driver espressif__button)

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
