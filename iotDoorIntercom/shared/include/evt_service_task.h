/**
 * @file evt_service_task.h
 * @brief P4 event aggregator — GPIO events → bridge EVT to C6 Matter.
 */

#ifndef EVT_SERVICE_TASK_H_
#define EVT_SERVICE_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <driver/gpio.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>

#define LED_PIN          GPIO_NUM_22 /*!< P4 GPIO22 — status LED (event aggregator) */
#define DEBOUNCE_TIME_MS 200

#define EVT_SERVICE_TASK_STACK_SIZE 3072
#define EVT_SERVICE_TASK_PRIORITY   5
#define EVT_SERVICE_TASK_CORE_ID    0

typedef enum {
    EVT_SOURCE_PIR,
    EVT_SOURCE_DOORBELL,
    EVT_SOURCE_PANIC, /*!< Tamper open — starts local siren; CLEARED does not stop it */
    EVT_SOURCE_ALARM, /*!< BRIDGE_CMD_SET_SIREN — TRIGGERED starts, CLEARED stops */
    EVT_SOURCE_MAX,
} evt_source_t;

typedef enum {
    EVT_TYPE_TRIGGERED,
    EVT_TYPE_SUSTAINED,
    EVT_TYPE_CLEARED,
} evt_type_t;

typedef struct {
    evt_source_t source;
    evt_type_t type;
    uint32_t timestamp;
    int value;
} evt_service_event_t;

esp_err_t evt_service_init(void);
esp_err_t evt_service_post(evt_service_event_t *evt);
esp_err_t evt_service_post_from_isr(evt_service_event_t *evt, BaseType_t *woken);

#ifdef __cplusplus
}
#endif

#endif /* EVT_SERVICE_TASK_H_ */
