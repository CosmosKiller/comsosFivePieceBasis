#ifndef HTTP_STREAM_TASK_H
#define HTTP_STREAM_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <esp_err.h>
#include <esp_http_server.h>

/**
 * @brief Enable/disable streaming without stopping server
 *
 * @param enable true to enable stream, false to disable
 */
void http_stream_task_service_enabled(bool enable);

/**
 * @brief Check if streaming is enabled
 *
 * @return true if streaming enabled, false otherwise
 */
bool http_stream_task_service_check(void);

/**
 * @brief Start the MJPEG stream server (HTTPS by default)
 *
 * With CONFIG_IOT_DOOR_INTERCOM_HTTPS_STREAM (default): listens on port 443
 * using the embedded Beta self-signed cert. Otherwise plain HTTP on port 80.
 *
 * @param server Unused; pass NULL. Kept for call-site compatibility.
 * @return httpd_handle_t Server handle, or NULL on failure
 */
httpd_handle_t http_server_task_start(httpd_handle_t server);

#ifdef __cplusplus
}
#endif

#endif // HTTP_STREAM_TASK_H
