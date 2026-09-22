/**
 * @file panic_alarm_task.h
 * @brief Panic alarm LED‖buzzer sequence (P4 GPIO46); latched until C6 clears siren OnOff.
 */

#ifndef PANIC_ALARM_TASK_H_
#define PANIC_ALARM_TASK_H_

#include <stdbool.h>

#include <driver/gpio.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ALARM_LED_PIN GPIO_NUM_46 /*!< P4 GPIO46 — siren LED + buzzer via NPN (active-high) */

#define PANIC_ALARM_STACK_SIZE    3072
#define PANIC_ALARM_TASK_PRIORITY 4

/**
 * @brief Start the panic-alarm blink task (idempotent if already running).
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
