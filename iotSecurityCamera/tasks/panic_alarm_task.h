/**
 * @file panic_alarm_task.h
 * @brief Siren blink on XIAO D0 (GPIO1). Latched until Matter siren OnOff is Off.
 *
 * Same cadence as the SKU 5 P4 siren. Do not use camera DVP pins, GPIO0 (BOOT),
 * GPIO5 (battery ADC), or GPIO43/44 (console UART).
 */

#ifndef PANIC_ALARM_TASK_H_
#define PANIC_ALARM_TASK_H_

#include <stdbool.h>

#include <driver/gpio.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ALARM_LED_PIN GPIO_NUM_1 /*!< XIAO D0 — active-high siren drive */

#define PANIC_ALARM_STACK_SIZE    3072
#define PANIC_ALARM_TASK_PRIORITY 4

/**
 * @brief Configure the siren pin as an output held low.
 *
 * Call once at boot so the pin is not floating before the first alarm.
 */
esp_err_t panic_alarm_task_prepare(void);

/**
 * @brief Start the siren blink task (idempotent if already running).
 */
esp_err_t panic_alarm_task_init(void);

/**
 * @brief Stop the siren task and force the pin low (idempotent if stopped).
 */
esp_err_t panic_alarm_task_deinit(void);

/**
 * @brief True while the siren FreeRTOS task is running.
 */
bool panic_alarm_task_is_active(void);

#ifdef __cplusplus
}
#endif

#endif /* PANIC_ALARM_TASK_H_ */
