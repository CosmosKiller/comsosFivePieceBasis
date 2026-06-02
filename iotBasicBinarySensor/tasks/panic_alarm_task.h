#ifndef PANIC_ALARM_TASK_H_
#define PANIC_ALARM_TASK_H_

#include <driver/gpio.h>
#include <esp_err.h>

#define CONFIRM_LED_PIN GPIO_NUM_22
#define ALARM_LED_PIN   GPIO_NUM_23

#define PANIC_ALARM_STACK_SIZE    3072
#define PANIC_ALARM_TASK_PRIORITY 4
#define PANIC_ALARM_TASK_CORE_ID  0

/**
 * @brief Initialize panic alarm task. This function should be called only once
 *
 * @param alarm_armed - Set to true to start panic alarm immediately, false to run arming sequence first
 *
 * @return esp_err_t - ESP_OK on success,
 *                     ESP_ERR_INVALID_ARG if pConfig is NULL
 *                     ESP_ERR_INVALID_STATE if task is already initialized
 *                     appropriate error code otherwise
 */
esp_err_t panic_alarm_task_init(bool alarm_armed);

/**
 * @brief Deinitialize panic alarm task
 *        This function will delete the panic alarm task and free any resources allocated for it.
 *
 * @return esp_err_t - ESP_OK on success,
 *                     ESP_ERR_INVALID_STATE if task is not initialized
 *                     appropriate error code otherwise
 */
esp_err_t panic_alarm_task_deinit(void);

#endif // PANIC_ALARM_TASK_H_