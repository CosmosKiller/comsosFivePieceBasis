/**
 * @file user_button_task.h
 * @brief Dedicated user button — toggle on/off and cycle presets (not factory reset).
 */

#ifndef USER_BUTTON_TASK_H_
#define USER_BUTTON_TASK_H_

#include <stdint.h>

#include <esp_err.h>

/**
 * @brief Start monitoring the user button on CONFIG_BEDSIDE_LAMP_USER_BUTTON_GPIO.
 *
 * @param light_endpoint_id Extended color light endpoint for toggle / preset updates.
 */
esp_err_t user_button_task_start(uint16_t light_endpoint_id);

#endif /* USER_BUTTON_TASK_H_ */
