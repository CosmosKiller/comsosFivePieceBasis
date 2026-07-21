/**
 * @file cosmos_matter_events.h
 * @brief Shared Matter device-layer events and identify callback helpers.
 */

#ifndef COSMOS_MATTER_EVENTS_H_
#define COSMOS_MATTER_EVENTS_H_

#include <esp_err.h>
#include <platform/CHIPDeviceEvent.h>

#ifdef __cplusplus
#include <esp_matter.h>
#endif

typedef void (*cosmos_matter_commissioning_complete_cb_t)(void);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Optional hook invoked after the last fabric is removed and commissioning re-opens.
 */
void cosmos_matter_set_commissioning_complete_hook(cosmos_matter_commissioning_complete_cb_t cb);

/**
 * @brief Handle standard Matter / CHIP device-layer events (commissioning, fabric, IP).
 *
 * @param event Event from the Matter stack.
 * @param arg   User argument (unused).
 */
void cosmos_matter_handle_device_event(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);

#ifdef __cplusplus
}

esp_err_t cosmos_matter_app_identification_cb(esp_matter::identification::callback_type_t type, uint16_t endpoint_id,
                                              uint8_t effect_id, uint8_t effect_variant, void *priv_data);
#endif

#endif /* COSMOS_MATTER_EVENTS_H_ */
