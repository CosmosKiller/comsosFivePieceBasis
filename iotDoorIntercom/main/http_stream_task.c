#include <esp_camera.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>
#include <stdlib.h>
#include <string.h>

#include <http_stream_task.h>

#if CONFIG_IOT_DOOR_INTERCOM_HTTPS_STREAM
#include <esp_https_server.h>
#else
#include <esp_http_server.h>
#endif

#define TAG "http_stream_task"

#define PART_BOUNDARY "frame"
/* Cap MJPEG send rate so TLS I/O does not starve the camera DMA ring. */
#define STREAM_MIN_FRAME_INTERVAL_MS 66 /* ~15 FPS */

static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %zu\r\n\r\n";

static bool stream_enabled = false;

static esp_err_t http_stream_task_handler(httpd_req_t *pReq)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    char part_buf[64];

    if (!stream_enabled) {
        return httpd_resp_send_404(pReq);
    }

    res = httpd_resp_set_type(pReq, _STREAM_CONTENT_TYPE);
    httpd_resp_set_hdr(pReq, "Cache-Control", "no-store");
    httpd_resp_set_hdr(pReq, "Pragma", "no-cache");
    if (res != ESP_OK) {
        return res;
    }

    while (stream_enabled) {
        const int64_t t0 = esp_timer_get_time();
        uint8_t *jpg_copy = NULL;
        size_t jpg_len = 0;

        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }

        /*
         * Copy JPEG out of the camera ring and return the FB before any TLS
         * send — holding the FB across httpd_resp_send_chunk causes FB-OVF.
         */
        if (fb->format != PIXFORMAT_JPEG) {
            if (!frame2jpg(fb, 80, &jpg_copy, &jpg_len)) {
                ESP_LOGE(TAG, "JPEG compression failed");
                esp_camera_fb_return(fb);
                res = ESP_FAIL;
                break;
            }
            esp_camera_fb_return(fb);
            fb = NULL;
        } else {
            jpg_len = fb->len;
            jpg_copy = (uint8_t *)malloc(jpg_len);
            if (!jpg_copy) {
                ESP_LOGE(TAG, "JPEG copy alloc failed (%zu)", jpg_len);
                esp_camera_fb_return(fb);
                res = ESP_ERR_NO_MEM;
                break;
            }
            memcpy(jpg_copy, fb->buf, jpg_len);
            esp_camera_fb_return(fb);
            fb = NULL;
        }

        res = httpd_resp_send_chunk(pReq, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        if (res == ESP_OK) {
            int hlen = snprintf(part_buf, sizeof(part_buf), _STREAM_PART, jpg_len);
            if (hlen < 0 || hlen >= (int)sizeof(part_buf)) {
                ESP_LOGE(TAG, "Header truncated (%d bytes needed >= %zu buffer)", hlen, sizeof(part_buf));
                res = ESP_FAIL;
            } else {
                res = httpd_resp_send_chunk(pReq, part_buf, (size_t)hlen);
            }
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(pReq, (const char *)jpg_copy, jpg_len);
        }
        free(jpg_copy);

        if (res != ESP_OK) {
            break;
        }

        const int64_t elapsed_ms = (esp_timer_get_time() - t0) / 1000;
        if (elapsed_ms < STREAM_MIN_FRAME_INTERVAL_MS) {
            vTaskDelay(pdMS_TO_TICKS(STREAM_MIN_FRAME_INTERVAL_MS - elapsed_ms));
        }
    }

    httpd_resp_send_chunk(pReq, "\r\n--" PART_BOUNDARY "--\r\n", strlen("\r\n--" PART_BOUNDARY "--\r\n"));
    return res;
}

httpd_handle_t http_server_task_start(httpd_handle_t server)
{
    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = http_stream_task_handler,
        .user_ctx = NULL,
    };

#if CONFIG_IOT_DOOR_INTERCOM_HTTPS_STREAM
    extern const unsigned char servercert_pem_start[] asm("_binary_servercert_pem_start");
    extern const unsigned char servercert_pem_end[] asm("_binary_servercert_pem_end");
    extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
    extern const unsigned char prvtkey_pem_end[] asm("_binary_prvtkey_pem_end");

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
    conf.httpd.stack_size = 16384;
    conf.httpd.core_id = 1;
    /* Long-lived MJPEG chunks need a longer send timeout than the SSL default. */
    conf.httpd.send_wait_timeout = 30;
    conf.httpd.recv_wait_timeout = 30;
    conf.servercert = servercert_pem_start;
    conf.servercert_len = servercert_pem_end - servercert_pem_start;
    conf.prvtkey_pem = prvtkey_pem_start;
    conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    esp_err_t ret = httpd_ssl_start(&server, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ssl_start failed: %d", ret);
        return NULL;
    }
    httpd_register_uri_handler(server, &stream_uri);
    ESP_LOGI(TAG, "HTTPS MJPEG ready at https://<device-ip>:%u/stream (self-signed Beta cert)",
             conf.port_secure);
#else
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.core_id = 1;
    config.task_priority = 5;

    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed");
        return NULL;
    }
    httpd_register_uri_handler(server, &stream_uri);
    ESP_LOGW(TAG, "HTTP (insecure) MJPEG ready at http://<device-ip>:%u/stream", config.server_port);
#endif

    return server;
}

void http_stream_task_service_enabled(bool enable)
{
    ESP_LOGI(TAG, "Stream %s", enable ? "enabled" : "disabled");
    stream_enabled = enable;
}

bool http_stream_task_service_check(void)
{
    return stream_enabled;
}
