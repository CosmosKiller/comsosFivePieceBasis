/**
 * @file panic_alarm_task.h
 * @brief Panic alarm buzzer/LED sequence after arming and trigger.
 */

#ifndef PANIC_ALARM_TASK_H_
#define PANIC_ALARM_TASK_H_

#include <driver/gpio.h>
#include <esp_err.h>

#define CONFIRM_LED_PIN GPIO_NUM_22 /*!< LED shown during arming confirm */
#define ALARM_LED_PIN   GPIO_NUM_23 /*!< LED driven during panic alarm */

#define PANIC_ALARM_STACK_SIZE    3072 /*!< FreeRTOS stack size */
#define PANIC_ALARM_TASK_PRIORITY 4    /*!< Task priority */
#define PANIC_ALARM_TASK_CORE_ID  0    /*!< CPU core */

/**
 * @brief Start the panic-alarm task and GPIO outputs.
 *
 * @param alarm_armed If true, enter alarm immediately; if false, run arming sequence first.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t panic_alarm_task_init(bool alarm_armed);

/**
 * @brief Stop the panic-alarm task and release resources.
 *
 * @return ESP_OK on success, or ESP_ERR_INVALID_STATE if not running.
 */
esp_err_t panic_alarm_task_deinit(void);

#endif /* PANIC_ALARM_TASK_H_ */
