/**
 * @file panic_alarm_task.h
 * @brief Panic alarm LED‖buzzer sequence (GPIO4); latched until HA clears Matter OnOff.
 */

#ifndef PANIC_ALARM_TASK_H_
#define PANIC_ALARM_TASK_H_

#include <stdbool.h>

#include <driver/gpio.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ALARM_LED_PIN GPIO_NUM_4 /*!< XIAO D3 — LED + buzzer (active-high) */

#define PANIC_ALARM_STACK_SIZE    3072
#define PANIC_ALARM_TASK_PRIORITY 4

/**
 * @brief Start the panic-alarm blink task (idempotent if already running).
 *
 * Short accelerating warning, then continuous 250/250 ms blink.
 */
esp_err_t panic_alarm_task_init(void);

/**
 * @brief Stop the panic-alarm task and force the pin low (idempotent if stopped).
 */
esp_err_t panic_alarm_task_deinit(void);

/**
 * @brief True while the panic-alarm FreeRTOS task is running.
 */
bool panic_alarm_task_is_active(void);

#ifdef __cplusplus
}
#endif

#endif /* PANIC_ALARM_TASK_H_ */
