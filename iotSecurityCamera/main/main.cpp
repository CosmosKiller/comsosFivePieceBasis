/**
 * @file main.cpp
 * @brief SKU6 Matter security camera: HTTPS MJPEG, stream gate, CSI occupancy, siren.
 *
 * Doorbell / PIR / tamper belong to SKU5 (iotDoorIntercom). The siren blink
 * matches that app; HA OnOff is the only start/stop on this board.
 */

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_matter.h>

#include <cam_task.h>
#include <cosmos_battery.h>
#include <cosmos_battery_matter.h>
#include <cosmos_matter_ota.h>
#include <csi_presence_task.h>
#include <factory_reset_task.h>
#include <http_stream_task.h>
#include <matter_task.h>
#include <panic_alarm_task.h>

static const char *TAG = "app_main";

using namespace esp_matter;
using namespace esp_matter::endpoint;

uint16_t stream_gate_endpoint_id = 0;
uint16_t siren_endpoint_id = 0;
httpd_handle_t cam_server;

extern "C" void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS needs erase (err=%d). Erasing and retrying...", err);
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %d", err);
        return;
    }

    err = panic_alarm_task_prepare();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "panic_alarm_task_prepare failed: %d", err);
        return;
    }

    err = cam_task_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "cam_task_init failed: %d", err);
        return;
    }

    node::config_t node_cfg;
    node_t *node = node::create(&node_cfg, app_attribute_update_cb, app_identification_cb);
    if (!node) {
        ESP_LOGE(TAG, "Failed to create Matter node");
        return;
    }

    on_off_plug_in_unit::config_t stream_cfg;
    stream_cfg.on_off.on_off = false;
    endpoint_t *stream_ep = on_off_plug_in_unit::create(node, &stream_cfg, ENDPOINT_FLAG_NONE, NULL);
    if (!stream_ep) {
        ESP_LOGE(TAG, "Failed to create stream-gate endpoint");
        return;
    }
    stream_gate_endpoint_id = endpoint::get_id(stream_ep);
    ESP_LOGI(TAG, "MJPEG stream-gate OnOff endpoint ID: %d", stream_gate_endpoint_id);

    mounted_on_off_control::config_t siren_cfg;
    siren_cfg.on_off.on_off = false;
    endpoint_t *siren_ep = mounted_on_off_control::create(node, &siren_cfg, ENDPOINT_FLAG_NONE, NULL);
    if (!siren_ep) {
        ESP_LOGE(TAG, "Failed to create siren endpoint");
        return;
    }
    siren_endpoint_id = endpoint::get_id(siren_ep);
    ESP_LOGI(TAG, "Siren OnOff endpoint ID: %d", siren_endpoint_id);

    err = csi_presence_endpoint_create(node);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "csi_presence_endpoint_create failed: %d", err);
        return;
    }

    cosmos_battery_config_t battery_config;
    cosmos_battery_config_set_defaults(&battery_config);
    battery_config.endpoint_id = cosmos_battery_matter_add_endpoint(node);
    if (battery_config.endpoint_id == 0) {
        ESP_LOGE(TAG, "Failed to create battery endpoint");
        return;
    }
    err = cosmos_battery_init(&battery_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "cosmos_battery_init failed: %d", err);
        return;
    }

    err = esp_matter::start(app_event_cb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_matter::start failed: %d", err);
        return;
    }

    err = cosmos_matter_ota_configure();
    if (err != ESP_OK && err != ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGE(TAG, "cosmos_matter_ota_configure failed: %d", err);
        return;
    }

    cam_server = http_server_task_start(NULL);
    if (!cam_server) {
        ESP_LOGE(TAG, "Failed to start MJPEG stream server");
        return;
    }

    err = csi_presence_task_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "csi_presence_task_start failed: %d", err);
        return;
    }

    err = cosmos_battery_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "cosmos_battery_start failed: %d", err);
        return;
    }

    factory_reset_task();
}
