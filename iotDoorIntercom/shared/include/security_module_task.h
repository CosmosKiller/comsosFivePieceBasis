/**
 * @file security_module_task.h
 * @brief P4 PIR + tamper GPIOs. Matter lives on C6 via bridge.
 */

#ifndef SECURITY_MODULE_TASK_H_
#define SECURITY_MODULE_TASK_H_

#include <driver/gpio.h>
#include <esp_err.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PIR_PIN         GPIO_NUM_32 /*!< P4 GPIO32 — PIR (AM312) + 10 kΩ PD */
#define TAMPER_PIN      GPIO_NUM_33 /*!< P4 GPIO33 — mount/tamper NC to GND when seated */
#define TRIGGER_TIME_MS 5000

/**
 * @brief Initialize PIR + tamper GPIOs. Call once after evt_service_init().
 */
esp_err_t security_module_task_init(void);

/**
 * @brief True when tamper contact is open (unit removed).
 */
bool security_module_tamper_is_open(void);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_MODULE_TASK_H_ */
