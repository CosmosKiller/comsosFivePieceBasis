/**
 * @file panic_alarm_task.h
 * @brief Arming sequence and siren/buzzer output (independent tasks).
 */

#ifndef PANIC_ALARM_TASK_H_
#define PANIC_ALARM_TASK_H_

#include <driver/gpio.h>
#include <esp_err.h>
#include <stdbool.h>

#define CONFIRM_LED_PIN GPIO_NUM_22 /*!< LED shown during arming confirm */
#define ALARM_LED_PIN   GPIO_NUM_23 /*!< Buzzer / alarm output */

#define PANIC_ALARM_STACK_SIZE    3072 /*!< FreeRTOS stack size */
#define PANIC_ALARM_TASK_PRIORITY 4    /*!< Task priority */
#define PANIC_ALARM_TASK_CORE_ID  0    /*!< CPU core */

/** Run exit-delay arming sequence, then armed standby heartbeat. */
esp_err_t panic_alarm_task_start_arming(void);

/** Cancel arming/standby; clears armed state only (siren may still run). */
esp_err_t panic_alarm_task_disarm(void);

/** Start siren countdown then latched buzzer (intrusion or remote OnOff). */
esp_err_t panic_alarm_task_start_siren(void);

/** Stop buzzer only; does not change armed or panic-indicator state. */
esp_err_t panic_alarm_task_stop_siren(void);

bool panic_alarm_task_siren_is_active(void);

#endif /* PANIC_ALARM_TASK_H_ */
