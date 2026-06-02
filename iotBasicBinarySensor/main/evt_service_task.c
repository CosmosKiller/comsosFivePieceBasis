#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <string.h>

#include <evt_service_task.h>
#include <panic_alarm_task.h>

#define EVT_QUEUE_SIZE 32

static const char *TAG = "evt_service";

static DRAM_ATTR QueueHandle_t evt_queue = NULL;

/**
 * @brief Handle all events centrally
 *
 * This task will receive events from various sources (sensor, alarm, panic) and handle them accordingly.
 * For example, it can control LEDs, buzzers, or trigger other actions based on the event type and source.
 *
 * @param pArg
 */
static void evt_service_task_handler(void *pArg)
{
    evt_service_event_t evt;

    while (1) {

        if (xQueueReceive(evt_queue, &evt, portMAX_DELAY)) {
            ESP_LOGD(TAG, "Event: source=%d, type=%d, value=%d", evt.source, evt.type, evt.value);

            // Handle based on source and type
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

            case EVT_SOURCE_ALARM:
                if (evt.type == EVT_TYPE_TRIGGERED) {
                    ESP_LOGW(TAG, "Alarm is being armed.");
                    panic_alarm_task_deinit();    // Ensure any existing alarm is stopped before arming
                    panic_alarm_task_init(false); // Run arming sequence
                } else if (evt.type == EVT_TYPE_CLEARED) {
                    ESP_LOGW(TAG, "Alarm disarmed.");
                    panic_alarm_task_deinit(); // Reset panic state when alarm is disarmed
                    for (int i = 0; i < 4; i++) {
                        gpio_set_level(CONFIRM_LED_PIN, 1);
                        vTaskDelay(pdMS_TO_TICKS(250));
                        gpio_set_level(CONFIRM_LED_PIN, 0);
                        vTaskDelay(pdMS_TO_TICKS(250));
                    }
                }
                break;

            case EVT_SOURCE_PANIC:
                if (evt.type == EVT_TYPE_TRIGGERED) {
                    ESP_LOGE(TAG, "Warning! Alarm secuence started.");
                    panic_alarm_task_deinit();   // Ensure any existing alarm is stopped before starting a new one
                    panic_alarm_task_init(true); // Ensure panic alarm task is running
                }
                if (evt.type == EVT_TYPE_SUSTAINED) {
                    ESP_LOGE(TAG, "Alarm is active. Disarm the system to stop the alarm.");
                }
                break;

            default:
                ESP_LOGW(TAG, "Unknown event source: %d", evt.source);
                break;
            }
        }
    }
}

/**
 * @brief Initialize event service LEDs
 *
 */
static void evt_service_led_init(void)
{
    // State LED config
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

    // Create event queue
    evt_queue = xQueueCreate(EVT_QUEUE_SIZE, sizeof(evt_service_event_t));
    if (!evt_queue) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_ERR_NO_MEM;
    }

    // Create event handling task
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

    // Initialize GPIO for LEDs
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