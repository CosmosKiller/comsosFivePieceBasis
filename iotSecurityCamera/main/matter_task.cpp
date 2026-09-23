/**
 * @file matter_task.cpp
 * @brief Matter callbacks for SKU6 MJPEG camera (stream-gate and siren OnOff).
 */

#include <esp_log.h>

#include <cosmos_matter_events.h>
#include <http_stream_task.h>
#include <matter_task.h>
#include <panic_alarm_task.h>

static const char *TAG = "matter_task";

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace chip::app::Clusters;

extern uint16_t stream_gate_endpoint_id;
extern uint16_t siren_endpoint_id;

void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    cosmos_matter_handle_device_event(event, arg);
}

esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                uint8_t effect_variant, void *priv_data)
{
    return cosmos_matter_app_identification_cb(type, endpoint_id, effect_id, effect_variant, priv_data);
}

esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                  uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    (void)priv_data;

    if (type == PRE_UPDATE && endpoint_id == stream_gate_endpoint_id && cluster_id == OnOff::Id &&
        attribute_id == OnOff::Attributes::OnOff::Id && val != nullptr) {
        ESP_LOGI(TAG, "Stream gate %s", val->val.b ? "ON" : "OFF");
        http_stream_task_service_enabled(val->val.b);
    }

    if (type == PRE_UPDATE && endpoint_id == siren_endpoint_id && cluster_id == OnOff::Id &&
        attribute_id == OnOff::Attributes::OnOff::Id && val != nullptr) {
        ESP_LOGI(TAG, "Siren %s", val->val.b ? "ON" : "OFF");
        if (val->val.b) {
            panic_alarm_task_init();
        } else {
            panic_alarm_task_deinit();
        }
    }

    return ESP_OK;
}
