#ifndef BINARTY_SENSOR_TASK_H_
#define BINARTY_SENSOR_TASK_H_

#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_matter.h>

#define SENSOR_PIN GPIO_NUM_20

using binary_sensor_cb_t = void (*)(uint16_t endpoint_id, bool occupied, void *user_data);

typedef void *binary_sensor_task_handle_t;

typedef struct {
    binary_sensor_cb_t cb = NULL; /*!< This callback functon will be called periodically to report the temperature.*/
    uint16_t endpoint_id;         /*!< Endpoint_id associated with temperature sensor */
    void *user_data = NULL;       /*!< User data*/
} binary_sensor_config_t;

/** Driver Update
 *
 * @brief This API should be called to update the driver for the attribute being updated.
 *        This is usually called from the common `app_attribute_update_cb()`.
 *
 * @param driver_handle Handle to the driver instance. This is usually passed as `priv_data` while creating the endpoint.
 * @param endpoint_id Endpoint ID of the attribute.
 * @param cluster_id Cluster ID of the attribute.
 * @param attribute_id Attribute ID of the attribute.
 * @param val Pointer to `esp_matter_attr_val_t`. Use appropriate elements as per the value type.
 *
 * @return error in case of failure.
 */
esp_err_t binary_sensor_attribute_update(binary_sensor_task_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val);

/**
 * @brief Initialize binary sensor driver. This function should be called only once
 *
 * @param pConfig sensor configurations. This should last for the lifetime of the driver
 *               as driver layer do not make a copy of this object.
 *
 * @return esp_err_t - ESP_OK on success,
 *                     ESP_ERR_INVALID_ARG if pConfig is NULL
 *                     ESP_ERR_INVALID_STATE if driver is already initialized
 *                     appropriate error code otherwise
 */

esp_err_t binary_sensor_task_init(binary_sensor_config_t *pConfig);

#endif // BINARTY_SENSOR_TASK_H_