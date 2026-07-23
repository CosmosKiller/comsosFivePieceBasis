/**
 * @file binary_sensor_task.h
 * @brief GPIO binary sensor (reed switch) driver and occupancy/state callbacks.
 */

#ifndef BINARY_SENSOR_TASK_H_
#define BINARY_SENSOR_TASK_H_

#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_matter.h>

#define SENSOR_PIN GPIO_NUM_20 /*!< Binary sensor input GPIO */

using binary_sensor_cb_t = void (*)(uint16_t endpoint_id, bool occupied, void *user_data);

typedef void *binary_sensor_task_handle_t;

typedef struct {
    binary_sensor_cb_t cb; /*!< Called when sensor state changes */
    uint16_t endpoint_id;  /*!< Matter endpoint for Boolean State */
    void *user_data;       /*!< Opaque pointer passed to cb */
} binary_sensor_config_t;

/**
 * @brief Update driver when a Matter attribute changes (from `app_attribute_update_cb()`).
 *
 * @param driver_handle  Handle from init (if used).
 * @param endpoint_id    Matter endpoint id.
 * @param cluster_id     Cluster id.
 * @param attribute_id   Attribute id.
 * @param val            New attribute value.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t binary_sensor_attribute_update(binary_sensor_task_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val);

/**
 * @brief Initialize GPIO ISR and binary sensor driver (call once).
 *
 * @param pConfig Configuration; must remain valid for the driver lifetime.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t binary_sensor_task_init(binary_sensor_config_t *pConfig);

/**
 * @brief Enable GPIO interrupts and publish the initial sensor state.
 *
 * Call after esp_matter::start() so Matter callbacks run on the CHIP thread.
 *
 * @return ESP_OK on success, or an error code.
 */
esp_err_t binary_sensor_task_start(void);

#endif /* BINARY_SENSOR_TASK_H_ */
