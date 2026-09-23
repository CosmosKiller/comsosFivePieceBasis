/**
 * @file csi_presence_task.h
 * @brief Wi-Fi CSI presence (ESPectre) published as a Matter occupancy endpoint.
 *
 * Matter already owns the station. This module does not start a second Wi-Fi
 * stack. Sensing starts after the station has an IPv4 address.
 *
 * @note A distributed iotSecurityCamera image that links ESPectre is a GPLv3
 *       combined work. Cosmos-authored files in this repository stay MIT.
 */

#ifndef CSI_PRESENCE_TASK_H_
#define CSI_PRESENCE_TASK_H_

#include <esp_err.h>
#include <esp_matter.h>

/**
 * @brief Create the occupancy endpoint (RF sensing feature).
 *
 * Call before esp_matter::start(). OccupancySensorTypeEnum has no RF value;
 * the feature map bit is Feature::kRFSensing.
 *
 * @param node Matter node created by the app.
 * @return ESP_OK on success, or an ESP_ERR_* code.
 */
esp_err_t csi_presence_endpoint_create(esp_matter::node_t *node);

/**
 * @brief Start the CSI runtime task.
 *
 * Call after esp_matter::start() so the station netif exists. The task waits
 * for IPv4, then runs ESPectre setup() and loop() on that task.
 *
 * @return ESP_OK when the task is created.
 */
esp_err_t csi_presence_task_start(void);

#endif /* CSI_PRESENCE_TASK_H_ */
