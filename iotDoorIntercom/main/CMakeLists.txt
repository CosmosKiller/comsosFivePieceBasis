# Main component: application sources and task headers (../tasks)

idf_component_register(SRCS "main.cpp" "security_module_task.cpp" "matter_task.cpp"
                            "http_stream_task.c" "cam_task.c" "door_intercom_task.cpp" "evt_service_task.c"
                       INCLUDE_DIRS "." "../tasks"
                       REQUIRES esp_matter cosmos_matter_common esp_http_server esp_https_server esp_psram
                       EMBED_TXTFILES "certs/servercert.pem" "certs/prvtkey.pem")

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE
    "-DCHIP_HAVE_CONFIG_H"
    -Os
    -ffunction-sections
    -fdata-sections
)
