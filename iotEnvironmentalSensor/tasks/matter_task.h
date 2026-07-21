/**
 * @file matter_task.h
 * @brief Matter stack callbacks (events, identify, attribute updates) for the environmental sensor.
 */

#ifndef MATTER_TASK_H_
#define MATTER_TASK_H_

#include <esp_err.h>
#include <esp_matter.h>
#include <platform/CHIPDeviceEvent.h>

void app_event_cb(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);

esp_err_t app_identification_cb(esp_matter::identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                  uint8_t effect_variant, void *priv_data);

esp_err_t app_attribute_update_cb(esp_matter::attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                  uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data);

#endif /* MATTER_TASK_H_ */
