/**
 * @file door_intercom_task.cpp
 * @brief P4 doorbell GPIO — tact → 3V3 + PD; posts EVT (no Matter on P4).
 */

#include <esp_log.h>
#include <freertos/FreeRTOS.h>

#include <door_intercom_task.h>
#include <evt_service_task.h>

static const char *TAG = "door_intercom_task";

static bool s_initialized = false;

static void IRAM_ATTR door_intercom_task_doorbell_isr_handler(void *pArg)
{
    (void)pArg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int level = gpio_get_level(DOORBELL_PIN);

    /* HIGH = pressed (tact to 3V3 + PD). Matter updates run on C6 via bridge. */
    evt_service_event_t evt = {
        .source = EVT_SOURCE_DOORBELL,
        .type = level ? EVT_TYPE_TRIGGERED : EVT_TYPE_CLEARED,
        .timestamp = 0,
        .value = level,
    };
    evt_service_post_from_isr(&evt, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

esp_err_t door_intercom_task_init(void)
{
    if (s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    gpio_config_t doorbell_conf = {
        .pin_bit_mask = (1ULL << DOORBELL_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&doorbell_conf);
    gpio_isr_handler_add(DOORBELL_PIN, door_intercom_task_doorbell_isr_handler, NULL);

    ESP_LOGI(TAG, "Doorbell initialized (GPIO%d, tact→3V3+PD)", DOORBELL_PIN);
    s_initialized = true;
    return ESP_OK;
}
