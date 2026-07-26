#ifndef EVT_SERVICE_TASK_H_
#define EVT_SERVICE_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <driver/gpio.h>
#include <esp_err.h>

#define LED_PIN          GPIO_NUM_21 /*!< XIAO ESP32-S3 user LED */
#define DEBOUNCE_TIME_MS 200

#define EVT_SERVICE_TASK_STACK_SIZE 3072
#define EVT_SERVICE_TASK_PRIORITY   5
#define EVT_SERVICE_TASK_CORE_ID    0

/**
 * @brief Event source types
 */
typedef enum {
    EVT_SOURCE_PIR,
    EVT_SOURCE_DOORBELL,
    EVT_SOURCE_INTERCOM,
    EVT_SOURCE_PANIC, /*!< Tamper / anti-theft (mount break or panic) */
    EVT_SOURCE_MAX,
} evt_source_t;

/**
 * @brief Event types
 */
typedef enum {
    EVT_TYPE_TRIGGERED, // Motion detected, button pressed, tamper opened
    EVT_TYPE_SUSTAINED, // Motion still detected after timer
    EVT_TYPE_CLEARED,   // Motion gone, call ended, tamper closed
} evt_type_t;

/**
 * @brief Unified event structure
 */
typedef struct {
    evt_source_t source;
    evt_type_t type;
    uint32_t timestamp;
    int value;
} evt_service_event_t;

/**
 * @brief Initialize event service
 * @return esp_err_t
 */
esp_err_t evt_service_init(void);

/**
 * @brief Post event to service
 * @param evt Event pointer
 * @return esp_err_t
 */
esp_err_t evt_service_post(evt_service_event_t *evt);

#ifdef __cplusplus
}
#endif

#endif // EVT_SERVICE_TASK_H_
