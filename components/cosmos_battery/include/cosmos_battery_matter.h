/**
 * @file cosmos_battery_matter.h
 * @brief Matter Power Source endpoint helper for cosmos_battery.
 */

#ifndef COSMOS_BATTERY_MATTER_H_
#define COSMOS_BATTERY_MATTER_H_

#include <stdint.h>

#ifdef __cplusplus

#include <esp_matter.h>

/**
 * @brief Add a Matter Power Source endpoint with the battery feature.
 *
 * @param node Matter node created by the application.
 * @return Endpoint id, or 0 on failure.
 */
uint16_t cosmos_battery_matter_add_endpoint(esp_matter::node_t *node);

/**
 * @brief Publish battery voltage and percent on the Matter system layer.
 *
 * @param endpoint_id Power Source endpoint id.
 * @param voltage_mv  Battery voltage in millivolts.
 * @param percent_matter BatPercentRemaining (0 = unknown, 1–200 = 0.5%–100%).
 */
void cosmos_battery_matter_update(uint16_t endpoint_id, uint32_t voltage_mv, uint8_t percent_matter);

#endif /* __cplusplus */

#endif /* COSMOS_BATTERY_MATTER_H_ */
