#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <string.h>

#include <evt_service_task.h>
#include <http_stream_task.h>

#define EVT_QUEUE_SIZE 32

static const char *TAG = "evt_service";

static DRAM_ATTR QueueHandle_t evt_queue = NULL;

/**
 * @brief Handle all events centrally
 */
static void evt_service_task_handler(void *pArg)
{
    evt_service_event_t evt;

    while (1) {
        if (xQueueReceive(evt_queue, &evt, portMAX_DELAY)) {
            ESP_LOGD(TAG, "Event: source=%d, type=%d, value=%d", evt.source, evt.type, evt.value);

            // Handle based on source and type
            switch (evt.source) {
            case EVT_SOURCE_PIR:
                if (evt.type == EVT_TYPE_TRIGGERED) {
                    ESP_LOGI(TAG, "Motion detected!");
                    gpio_set_level(LED_PIN, 1);
                } else if (evt.type == EVT_TYPE_SUSTAINED) {
                    ESP_LOGI(TAG, "Motion sustained - stream active");
                    gpio_set_level(LED_PIN, 1);
                    http_stream_task_service_enabled(true);
                } else if (evt.type == EVT_TYPE_CLEARED) {
                    ESP_LOGI(TAG, "Motion ended.");
                    gpio_set_level(LED_PIN, 0);
                    http_stream_task_service_enabled(false);
                }
                break;

            case EVT_SOURCE_DOORBELL:
                if (evt.type == EVT_TYPE_TRIGGERED) {
                    ESP_LOGI(TAG, "Doorbell pressed! Call requested.");
                    gpio_set_level(LED_PIN, 1);
                    http_stream_task_service_enabled(true);
                }
                break;

            case EVT_SOURCE_INTERCOM:
                if (evt.type == EVT_TYPE_TRIGGERED) {
                    ESP_LOGI(TAG, "Call started.");
                    gpio_set_level(LED_PIN, 1);
                    http_stream_task_service_enabled(true);
                } else if (evt.type == EVT_TYPE_CLEARED) {
                    ESP_LOGI(TAG, "Call ended.");
                    gpio_set_level(LED_PIN, 0);
                    http_stream_task_service_enabled(false);
                }
                break;

            case EVT_SOURCE_PANIC:
                if (evt.type == EVT_TYPE_TRIGGERED) {
                    /* Tamper open: Matter contact_sensor reports via Boolean State; enable stream. */
                    ESP_LOGE(TAG, "TAMPER open — mount break / unit removed");
                    gpio_set_level(LED_PIN, 1);
                    http_stream_task_service_enabled(true);
                    /* Later: local siren GPIO + optional Matter alarm. */
                } else if (evt.type == EVT_TYPE_CLEARED) {
                    ESP_LOGW(TAG, "Tamper cleared — unit seated");
                }
                break;

            default:
                ESP_LOGW(TAG, "Unknown event source: %d", evt.source);
                break;
            }
        }
    }
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

    // LED config
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&led_conf);
    gpio_set_level(LED_PIN, 0);

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