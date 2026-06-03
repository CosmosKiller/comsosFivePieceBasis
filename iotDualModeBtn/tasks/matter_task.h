/**
 * @file matter_task.h
 * @brief Matter stack callbacks (events, identify, attribute updates) for the dual-mode button.
 */

#ifndef MATTER_TASK_H_
#define MATTER_TASK_H_

#include <esp_err.h>

#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>
#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_ota.h>

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

/**
 * @brief CHIP device-layer event handler (commissioning, fabric, IP changes).
 *
 * @param event Event from the Matter stack.
 * @param arg   User argument (unused).
 */
void app_event_cb(const ChipDeviceEvent *event, intptr_t arg);

/**
 * @brief Identify cluster callback (e.g. blink LED on identify command).
 *
 * @param type            Identify callback type.
 * @param endpoint_id     Target endpoint.
 * @param effect_id       Identify effect id.
 * @param effect_variant  Identify effect variant.
 * @param priv_data       Private data from endpoint creation.
 * @return ESP_OK or an error code.
 */
esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                uint8_t effect_variant, void *priv_data);

/**
 * @brief Attribute update hook; delegate to drivers or return ESP_OK if not handled.
 *
 * @param type          Pre/post update.
 * @param endpoint_id   Target endpoint.
 * @param cluster_id    Cluster id.
 * @param attribute_id  Attribute id.
 * @param val           New attribute value.
 * @param priv_data     Driver handle from endpoint priv_data.
 * @return ESP_OK or an error code.
 */
esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                  uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data);

#endif /* MATTER_TASK_H_ */
