/**
 * @file door_intercom_task.h
 * @brief P4 doorbell GPIO (tact → 3V3 + 10 kΩ PD). Matter lives on C6 via bridge.
 */

#ifndef DOOR_INTERCOM_TASK_H_
#define DOOR_INTERCOM_TASK_H_

#include <driver/gpio.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Waveshare ESP32-P4-WIFI6 — see docs/HARDWARE.md SKU 5 GPIO map. */
#define DOORBELL_PIN GPIO_NUM_27 /*!< P4 GPIO27 — doorbell tact to 3V3 + 10 kΩ PD */

/**
 * @brief Initialize doorbell GPIO + ISR (posts EVT_SOURCE_DOORBELL). Call once.
 */
esp_err_t door_intercom_task_init(void);

#ifdef __cplusplus
}
#endif

#endif /* DOOR_INTERCOM_TASK_H_ */
