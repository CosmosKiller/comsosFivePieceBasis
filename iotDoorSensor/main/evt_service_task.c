#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <string.h>

#include <door_sensor_matter_notify.h>
#include <evt_service_task.h>
#include <panic_alarm_task.h>

#define EVT_QUEUE_SIZE 32

static const char *TAG = "evt_service";

static DRAM_ATTR QueueHandle_t evt_queue = NULL;

static void evt_service_task_handler(void *pArg)
{
    evt_service_event_t evt;

    while (1) {

        if (xQueueReceive(evt_queue, &evt, portMAX_DELAY)) {
            ESP_LOGD(TAG, "Event: source=%d, type=%d, value=%d", evt.source, evt.type, evt.value);

            switch (evt.source) {
            case EVT_SOURCE_SENSOR:
                if (evt.type == EVT_TYPE_TRIGGERED) {
                    ESP_LOGW(TAG, "Door/window opened!");

                    for (size_t i = 0; i < 3; i++) {
                        gpio_set_level(STATE_LED_PIN, 1);
                        vTaskDelay(pdMS_TO_TICKS(500));
                        gpio_set_level(STATE_LED_PIN, 0);
                        vTaskDelay(pdMS_TO_TICKS(500));
                    }

                } else if (evt.type == EVT_TYPE_CLEARED) {
                    ESP_LOGW(TAG, "Door/window closed.");

                    for (size_t i = 0; i < 2; i++) {
                        gpio_set_level(STATE_LED_PIN, 1);
                        vTaskDelay(pdMS_TO_TICKS(500));
                        gpio_set_level(STATE_LED_PIN, 0);
                        vTaskDelay(pdMS_TO_TICKS(500));
                    }
                }
                break;

            case EVT_SOURCE_ARM:
                if (evt.type == EVT_TYPE_TRIGGERED) {
                    ESP_LOGW(TAG, "Arming sequence started.");
                    panic_alarm_task_start_arming();
                } else if (evt.type == EVT_TYPE_CLEARED) {
                    ESP_LOGW(TAG, "Disarmed (siren and panic indicator unchanged).");
                    panic_alarm_task_disarm();
                }
                break;

            case EVT_SOURCE_SIREN:
                if (evt.type == EVT_TYPE_TRIGGERED) {
                    ESP_LOGW(TAG, "Siren On — buzzer start (remote or latched).");
                    panic_alarm_task_start_siren();
                } else if (evt.type == EVT_TYPE_CLEARED) {
                    ESP_LOGI(TAG, "Siren Off — buzzer stopped.");
                    panic_alarm_task_stop_siren();
                }
                break;

            case EVT_SOURCE_PANIC:
                if (evt.type == EVT_TYPE_TRIGGERED) {
                    ESP_LOGE(TAG, "Intrusion! Reed open while armed.");
                    door_sensor_matter_notify_panic(true);
                    door_sensor_matter_notify_siren(true);
                    panic_alarm_task_start_siren();
                } else if (evt.type == EVT_TYPE_CLEARED) {
                    ESP_LOGW(TAG, "Contact closed — clearing panic indicator (siren stays until Off).");
                    door_sensor_matter_notify_panic(false);
                } else if (evt.type == EVT_TYPE_SUSTAINED) {
                    ESP_LOGE(TAG, "Intrusion still active — siren latched until cleared.");
                }
                break;

            default:
                ESP_LOGW(TAG, "Unknown event source: %d", evt.source);
                break;
            }
        }
    }
}

static void evt_service_led_init(void)
{
    gpio_config_t state_led_conf = {
        .pin_bit_mask = (1ULL << STATE_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&state_led_conf);
    gpio_set_level(STATE_LED_PIN, 0);
}

esp_err_t evt_service_init(void)
{
    if (evt_queue != NULL) {
        ESP_LOGW(TAG, "Event service already initialized");
        return ESP_OK;
    }

    evt_queue = xQueueCreate(EVT_QUEUE_SIZE, sizeof(evt_service_event_t));
    if (!evt_queue) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ret = xTaskCreatePinnedToCore(
        evt_service_task_handler,
        "evt_service_task_handler",
        EVT_SERVICE_TASK_STACK_SIZE,
        NULL,
        EVT_SERVICE_TASK_PRIORITY,
        NULL,
        EVT_SERVICE_TASK_CORE_ID);
    if (ret != pdPASS) {
        vQueueDelete(evt_queue);
        evt_queue = NULL;
        ESP_LOGE(TAG, "Failed to create event service task");
        return ESP_ERR_NO_MEM;
    }

    evt_service_led_init();

    ESP_LOGI(TAG, "Event service initialized");
    return ESP_OK;
}

esp_err_t evt_service_post(evt_service_event_t *evt)
{
    if (!evt_queue) {
        ESP_LOGW(TAG, "Event queue not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!evt) {
        return ESP_ERR_INVALID_ARG;
    }

    evt->timestamp = esp_log_timestamp();

    BaseType_t ret = xQueueSend(evt_queue, evt, pdMS_TO_TICKS(100));
    if (ret != pdPASS) {
        ESP_LOGW(TAG, "Event queue full, dropping event from source %d", evt->source);
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t evt_service_post_from_isr(evt_service_event_t *evt, BaseType_t *woken)
{
    if (!evt_queue || evt == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    evt->timestamp = (uint32_t)(xTaskGetTickCountFromISR() * portTICK_PERIOD_MS);

    if (xQueueSendFromISR(evt_queue, evt, woken) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}
