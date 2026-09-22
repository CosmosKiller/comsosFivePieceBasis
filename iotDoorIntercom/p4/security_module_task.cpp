/**
 * @file security_module_task.cpp
 * @brief P4 PIR + tamper — posts EVT (no Matter on P4).
 */

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#include <evt_service_task.h>
#include <security_module_task.h>

static const char *TAG = "security_module_task";

static TimerHandle_t s_detection_timer = NULL;
static bool s_initialized = false;

static void IRAM_ATTR security_module_task_pir_isr_handler(void *pArg)
{
    (void)pArg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int level = gpio_get_level(PIR_PIN);

    evt_service_event_t evt = {
        .source = EVT_SOURCE_PIR,
        .type = level ? EVT_TYPE_TRIGGERED : EVT_TYPE_CLEARED,
        .timestamp = 0,
        .value = level,
    };
    evt_service_post_from_isr(&evt, &xHigherPriorityTaskWoken);

    if (level == 1) {
        xTimerStartFromISR(s_detection_timer, &xHigherPriorityTaskWoken);
    } else {
        xTimerStopFromISR(s_detection_timer, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void IRAM_ATTR security_module_task_tamper_isr_handler(void *pArg)
{
    (void)pArg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int level = gpio_get_level(TAMPER_PIN);
    bool tampered = (level == 1);

    evt_service_event_t evt = {
        .source = EVT_SOURCE_PANIC,
        .type = tampered ? EVT_TYPE_TRIGGERED : EVT_TYPE_CLEARED,
        .timestamp = 0,
        .value = level,
    };
    evt_service_post_from_isr(&evt, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void security_module_task_pir_timer_cb(TimerHandle_t xTimer)
{
    (void)xTimer;
    int level = gpio_get_level(PIR_PIN);
    if (level == 1) {
        evt_service_event_t evt = {
        .source = EVT_SOURCE_PIR,
        .type = EVT_TYPE_SUSTAINED,
        .timestamp = 0,
        .value = level,
    };
        evt_service_post(&evt);
    }
}

bool security_module_tamper_is_open(void)
{
    return gpio_get_level(TAMPER_PIN) == 1;
}

esp_err_t security_module_task_init(void)
{
    if (s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    gpio_config_t pir_conf = {
        .pin_bit_mask = (1ULL << PIR_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&pir_conf);

    s_detection_timer = xTimerCreate("DetectionTimer", pdMS_TO_TICKS(TRIGGER_TIME_MS), pdFALSE, NULL,
                                     security_module_task_pir_timer_cb);
    gpio_isr_handler_add(PIR_PIN, security_module_task_pir_isr_handler, NULL);

    /* Tamper: pull-up, NC to GND when mounted → LOW seated, HIGH = open. */
    gpio_config_t tamper_conf = {
        .pin_bit_mask = (1ULL << TAMPER_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&tamper_conf);
    gpio_isr_handler_add(TAMPER_PIN, security_module_task_tamper_isr_handler, NULL);

    s_initialized = true;

    int tamper_level = gpio_get_level(TAMPER_PIN);
    bool tampered = (tamper_level == 1);
    if (tampered) {
        evt_service_event_t evt = {
        .source = EVT_SOURCE_PANIC,
        .type = EVT_TYPE_TRIGGERED,
        .timestamp = 0,
        .value = tamper_level,
    };
        evt_service_post(&evt);
    }

    ESP_LOGI(TAG, "Security module initialized (PIR=%d, TAMPER=%d, seated=%s)", PIR_PIN, TAMPER_PIN,
             tampered ? "no" : "yes");
    return ESP_OK;
}
