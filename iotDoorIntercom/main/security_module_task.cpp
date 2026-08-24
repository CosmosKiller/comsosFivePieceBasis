#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <lib/support/CodeUtils.h>

#include <evt_service_task.h>
#include <security_module_task.h>

using namespace chip::app::Clusters;
using namespace esp_matter;

static const char *TAG = "security_module_task";

static DRAM_ATTR TimerHandle_t detection_timer = NULL;

typedef struct {
    security_module_config_t *config;
    bool is_initialized;
} security_module_ctx_t;

static security_module_ctx_t s_ctx;

/**
 * @brief ISR handler for PIR sensor
 */
static void IRAM_ATTR security_module_task_pir_isr_handler(void *pArg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int level = gpio_get_level(PIR_PIN);

    evt_service_event_t evt = {
        .source = EVT_SOURCE_PIR,
        .type = level ? EVT_TYPE_TRIGGERED : EVT_TYPE_CLEARED,
        .value = level,
    };
    evt_service_post_from_isr(&evt, &xHigherPriorityTaskWoken);

    if (level == 1) {
        xTimerStartFromISR(detection_timer, &xHigherPriorityTaskWoken);
    } else {
        xTimerStopFromISR(detection_timer, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief Tamper contact: NC→GND when seated (LOW). HIGH = open = unit removed / tampered.
 */
static void IRAM_ATTR security_module_task_tamper_isr_handler(void *pArg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int level = gpio_get_level(TAMPER_PIN);
    bool tampered = (level == 1);

    evt_service_event_t evt = {
        .source = EVT_SOURCE_PANIC,
        .type = tampered ? EVT_TYPE_TRIGGERED : EVT_TYPE_CLEARED,
        .value = level,
    };
    evt_service_post_from_isr(&evt, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void security_module_task_pir_timer_cb(TimerHandle_t xTimer)
{
    int level = gpio_get_level(PIR_PIN);

    if (level == 1) {
        evt_service_event_t evt = {
            .source = EVT_SOURCE_PIR,
            .type = EVT_TYPE_SUSTAINED,
            .value = level,
        };
        evt_service_post(&evt);
    }
}

bool security_module_tamper_is_open(void)
{
    return gpio_get_level(TAMPER_PIN) == 1;
}

esp_err_t security_module_task_init(security_module_config_t *pConfig)
{
    if (pConfig == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_ctx.is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    gpio_config_t pir_conf = {
        .pin_bit_mask = (1ULL << PIR_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE};
    gpio_config(&pir_conf);

    detection_timer = xTimerCreate(
        "DetectionTimer",
        pdMS_TO_TICKS(TRIGGER_TIME_MS),
        pdFALSE,
        NULL,
        security_module_task_pir_timer_cb);

    gpio_isr_handler_add(PIR_PIN, security_module_task_pir_isr_handler, NULL);

    /* Tamper: pull-up, NC to GND when mounted → LOW seated, HIGH = open/tampered. */
    gpio_config_t tamper_conf = {
        .pin_bit_mask = (1ULL << TAMPER_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE};
    gpio_config(&tamper_conf);
    gpio_isr_handler_add(TAMPER_PIN, security_module_task_tamper_isr_handler, NULL);

    s_ctx.config = pConfig;
    s_ctx.is_initialized = true;

    /* Do not call Matter callbacks here — esp_matter::start() has not run yet.
     * Initial Boolean State sync is done from app_main after the stack starts. */
    int tamper_level = gpio_get_level(TAMPER_PIN);
    bool tampered = (tamper_level == 1);
    if (tampered) {
        evt_service_event_t evt = {
            .source = EVT_SOURCE_PANIC,
            .type = EVT_TYPE_TRIGGERED,
            .value = tamper_level,
        };
        evt_service_post(&evt);
    }

    ESP_LOGI(TAG, "Security module initialized (PIR=%d, TAMPER=%d, seated=%s)",
             PIR_PIN, TAMPER_PIN, tampered ? "no" : "yes");
    return ESP_OK;
}
