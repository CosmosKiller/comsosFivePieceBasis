idf_component_register(SRCS "cosmos_factory_reset_task.cpp" "cosmos_matter_events.cpp" "cosmos_matter_ota.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_matter espressif__button)

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
