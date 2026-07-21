/**
 * @file matter_task.cpp
 * @brief Matter callbacks for the binary sensor (shared events + sensor attribute driver).
 */

#include <binary_sensor_task.h>
#include <cosmos_matter_events.h>
#include <matter_task.h>

using namespace esp_matter;
using namespace esp_matter::attribute;

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
    esp_err_t err = ESP_OK;

    if (type == PRE_UPDATE) {
        binary_sensor_task_handle_t binary_sensor_handle = (binary_sensor_task_handle_t)priv_data;
        err = binary_sensor_attribute_update(binary_sensor_handle, endpoint_id, cluster_id, attribute_id, val);
    }

    return err;
}
