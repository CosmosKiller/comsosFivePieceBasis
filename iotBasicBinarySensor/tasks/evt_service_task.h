/**
 * @file evt_service_task.h
 * @brief Central event queue and status LED handling for sensor, alarm, and panic sources.
 */

#ifndef EVT_SERVICE_TASK_H_
#define EVT_SERVICE_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <driver/gpio.h>
#include <esp_err.h>

#define STATE_LED_PIN GPIO_NUM_21 /*!< Status LED for aggregated events */

#define EVT_SERVICE_TASK_STACK_SIZE 3072 /*!< FreeRTOS stack size */
#define EVT_SERVICE_TASK_PRIORITY   5    /*!< Task priority */
#define EVT_SERVICE_TASK_CORE_ID    0    /*!< CPU core */

/**
 * @brief Origin of an event posted to the service.
 */
typedef enum {
    EVT_SOURCE_SENSOR, /*!< Reed switch / binary input */
    EVT_SOURCE_ALARM,  /*!< Alarm endpoint activity */
    EVT_SOURCE_PANIC,  /*!< Panic alarm sequence */
    EVT_SOURCE_MAX,    /*!< Sentinel (not a valid source) */
} evt_source_t;

/**
 * @brief Phase of an event (edge semantics).
 */
typedef enum {
    EVT_TYPE_TRIGGERED, /*!< Motion, open, or press detected */
    EVT_TYPE_SUSTAINED, /*!< Condition still active after debounce/timer */
    EVT_TYPE_CLEARED,   /*!< Condition cleared */
} evt_type_t;

/**
 * @brief Event message passed to the service task.
 */
typedef struct {
    evt_source_t source; /*!< Event origin */
    evt_type_t type;     /*!< Trigger / sustained / cleared */
    uint32_t timestamp;  /*!< Timestamp (implementation-defined units) */
    int value;           /*!< Optional payload (e.g. level or flag) */
} evt_service_event_t;

/**
 * @brief Create the event-service task and GPIO for the status LED.
 *
 * @return ESP_OK on success, or an error code.
 */
esp_err_t evt_service_init(void);

/**
 * @brief Enqueue an event for the service task (non-blocking).
 *
 * @param evt Event to post; must remain valid until consumed if stack-allocated.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t evt_service_post(evt_service_event_t *evt);

#ifdef __cplusplus
}
#endif

#endif /* EVT_SERVICE_TASK_H_ */
