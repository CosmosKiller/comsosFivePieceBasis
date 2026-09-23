/**
 * @file cosmos_p4_security_io.c
 * @brief Bring up P4 security I/O for split-mode streaming_only.
 */

#include <esp_log.h>
#include <driver/gpio.h>

#include <bridge_cmd_defs.h>
#include <webrtc_bridge.h>

#include <cosmos_p4_audio.h>
#include <cosmos_p4_security_io.h>
#include <door_intercom_task.h>
#include <evt_service_task.h>
#include <panic_alarm_task.h>
#include <security_module_task.h>
#include <sku5_io.h>

static const char *TAG = "cosmos_p4_io";

static esp_err_t handle_set_siren(uint32_t cmd_id, const uint8_t *req_data, size_t req_len, uint8_t **resp_data,
                                  size_t *resp_len)
{
    (void)cmd_id;
    *resp_data = NULL;
    *resp_len = 0;

    if (req_data == NULL || req_len < sizeof(bridge_cmd_set_siren_t)) {
        ESP_LOGW(TAG, "SET_SIREN bad payload len=%u", (unsigned)req_len);
        return ESP_ERR_INVALID_ARG;
    }

    const bridge_cmd_set_siren_t *req = (const bridge_cmd_set_siren_t *)req_data;
    ESP_LOGI(TAG, "SET_SIREN on=%u", req->on);

    /* Drive immediately — do not wait for EVT queue. */
    if (req->on) {
        panic_alarm_task_init();
    } else {
        panic_alarm_task_deinit();
    }

    evt_service_event_t evt = {
        .source = EVT_SOURCE_ALARM,
        .type = req->on ? EVT_TYPE_TRIGGERED : EVT_TYPE_CLEARED,
        .timestamp = 0,
        .value = req->on,
    };
    (void)evt_service_post(&evt);
    return ESP_OK;
}

esp_err_t cosmos_p4_security_io_init(void)
{
    esp_err_t isr_err = gpio_install_isr_service(0);
    if (isr_err != ESP_OK && isr_err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "gpio_install_isr_service: %s", esp_err_to_name(isr_err));
        return isr_err;
    }

    esp_err_t err = evt_service_init();
    if (err != ESP_OK) {
        return err;
    }

    err = door_intercom_task_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "door_intercom_task_init: %s", esp_err_to_name(err));
        return err;
    }

    err = security_module_task_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "security_module_task_init: %s", esp_err_to_name(err));
        return err;
    }

    err = bridge_cmd_register_handler(BRIDGE_CMD_SET_SIREN, handle_set_siren);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SET_SIREN register failed: %s", esp_err_to_name(err));
        return err;
    }

    err = cosmos_p4_audio_init();
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "P4 security I/O ready (EVT→C6, SET_SIREN/SET_AUDIO←C6)");
    return ESP_OK;
}

esp_err_t sku5_io_init(void)
{
    return cosmos_p4_security_io_init();
}
