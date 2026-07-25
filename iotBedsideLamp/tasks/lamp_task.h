/**
 * @file lamp_task.h
 * @brief Matter extended color light driver bridge (WS2812 via esp-matter led_driver).
 */

#ifndef LAMP_TASK_H_
#define LAMP_TASK_H_

#include <esp_err.h>
#include <esp_matter.h>

#define STANDARD_BRIGHTNESS 100
#define STANDARD_HUE 360
#define STANDARD_SATURATION 100
#define STANDARD_TEMPERATURE_FACTOR 1000000

#define MATTER_BRIGHTNESS 254
#define MATTER_HUE 254
#define MATTER_SATURATION 254

#define DEFAULT_POWER true
#define DEFAULT_BRIGHTNESS 64
#define DEFAULT_HUE 40
#define DEFAULT_SATURATION 200

typedef void *lamp_task_handle_t;

/**
 * @brief Initialize the WS2812 driver (RMT, 1 LED on DevKit MVP).
 *
 * @return Driver handle, or NULL on failure.
 */
lamp_task_handle_t lamp_task_init(void);

/**
 * @brief Apply persisted Matter attributes to the LED after stack start.
 */
esp_err_t lamp_task_set_defaults(uint16_t endpoint_id);

/**
 * @brief Handle Matter attribute writes (PRE_UPDATE).
 */
esp_err_t lamp_task_attribute_update(lamp_task_handle_t handle, uint16_t endpoint_id, uint32_t cluster_id,
                                     uint32_t attribute_id, esp_matter_attr_val_t *val);

#endif /* LAMP_TASK_H_ */
