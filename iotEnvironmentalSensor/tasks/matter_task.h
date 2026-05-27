#ifndef MATTER_TASK_H_
#define MATTER_TASK_H_

// Include ESP-IDF libraries
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

// Include ESP-MATTER libraries
#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_ota.h>

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/ESP32/OpenthreadLauncher.h>
#endif

#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

/**
 * @brief Matter event callback
 *
 * @param event
 * @param arg
 */
void app_event_cb(const ChipDeviceEvent *event, intptr_t arg);

/**
 * @brief This function is called when an Identify cluster command is received.
 *        In the callback implementation, an endpoint can identify itself. (e.g., by flashing an LED or light).
 *
 * @param type
 * @param endpoint_id
 * @param effect_id
 * @param effect_variant
 * @param priv_data
 * @return esp_err_t
 */
esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                uint8_t effect_variant, void *priv_data);

/**
 * @brief This callback is called for every attribute update. The callback implementation shall
 *        handle the desired attributes and return an appropriate error code. If the attribute
 *        is not of your interest, please do not return an error code and strictly return ESP
 *
 * @param type
 * @param endpoint_id
 * @param cluster_id
 * @param attribute_id
 * @param val
 * @param priv_data
 * @return esp_err_t
 */
esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                  uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data);

#endif /* MATTER_TASK_H_ */