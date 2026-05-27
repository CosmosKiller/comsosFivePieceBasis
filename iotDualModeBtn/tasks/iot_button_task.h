#ifndef IOT_BUTTON_TASK_H_
#define IOT_BUTTON_TASK_H_

#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_matter.h>

#define BUTTON_GPIO_PIN      GPIO_NUM_20
#define SINGLE_PRESS_LED_PIN GPIO_NUM_21
#define MULTI_PRESS_LED_PIN  GPIO_NUM_19

typedef void *iot_button_task_handle_t;

typedef struct {
    gpio_num_t button_pin; /*!< The GPIO pin connected to the button */
} iot_button_config_t;

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
esp_err_t iot_button_attribute_update(iot_button_task_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                      uint32_t attribute_id, esp_matter_attr_val_t *val);

/**
 * @brief Initialize iot button driver. This function should be called only once
 *
 * @param button Button configurations. This should last for the lifetime of the driver
 *               as driver layer do not make a copy of this object.
 *
 * @return esp_err_t - ESP_OK on success,
 *                     ESP_ERR_INVALID_ARG if pConfig is NULL
 *                     ESP_ERR_INVALID_STATE if driver is already initialized
 *                     appropriate error code otherwise
 */

iot_button_task_handle_t iot_button_task_init(iot_button_config_t *button = NULL);

#endif // IOT_BUTTON_TASK_H_