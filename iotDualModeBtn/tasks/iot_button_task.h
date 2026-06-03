/**
 * @file iot_button_task.h
 * @brief GPIO momentary switch driver and Matter Switch cluster event reporting.
 */

#ifndef IOT_BUTTON_TASK_H_
#define IOT_BUTTON_TASK_H_

#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_matter.h>

#define BUTTON_GPIO_PIN      GPIO_NUM_20 /*!< Action button GPIO */
#define SINGLE_PRESS_LED_PIN GPIO_NUM_21 /*!< LED indicating single press */
#define MULTI_PRESS_LED_PIN  GPIO_NUM_19 /*!< LED indicating multi-press */

typedef void *iot_button_task_handle_t;

typedef struct {
    gpio_num_t button_pin; /*!< GPIO connected to the action button */
} iot_button_config_t;

/**
 * @brief Update driver state when a Matter Switch attribute changes.
 *
 * Usually called from `app_attribute_update_cb()`.
 *
 * @param driver_handle  Handle from `iot_button_task_init()`.
 * @param endpoint_id    Matter endpoint id.
 * @param cluster_id     Cluster id.
 * @param attribute_id   Attribute id.
 * @param val            New attribute value.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t iot_button_attribute_update(iot_button_task_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                      uint32_t attribute_id, esp_matter_attr_val_t *val);

/**
 * @brief Initialize the button driver and Matter Switch event hooks (call once).
 *
 * @param button Optional config; uses defaults when NULL. Must outlive the driver.
 * @return Opaque driver handle, or NULL on failure.
 */
iot_button_task_handle_t iot_button_task_init(iot_button_config_t *button = NULL);

#endif /* IOT_BUTTON_TASK_H_ */
