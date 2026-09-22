/**
 * @file evt_service_task.c
 * @brief P4 event aggregator — GPIO → BRIDGE_EVT_SECURITY_IO (no Matter on P4).
 */

#include <string.h>

#include <bridge_cmd_defs.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <webrtc_bridge.h>

#include <evt_service_task.h>
#include <panic_alarm_task.h>

#define EVT_QUEUE_SIZE 32

static const char *TAG = "evt_service";

static DRAM_ATTR QueueHandle_t s_evt_queue = NULL;

static bool s_doorbell = false;
static bool s_pir = false;
static bool s_tamper = false;

static void publish_security_io(uint8_t flags)
{
    bridge_evt_security_io_t evt = {
        .doorbell = s_doorbell ? 1u : 0u,
        .pir = s_pir ? 1u : 0u,
        .tamper = s_tamper ? 1u : 0u,
        .flags = flags,
    };
    esp_err_t err = bridge_cmd_send_event(BRIDGE_EVT_SECURITY_IO, (const uint8_t *)&evt, sizeof(evt));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "BRIDGE_EVT_SECURITY_IO failed: %s", esp_err_to_name(err));
    }
}

static void evt_service_task_handler(void *pArg)
{
    (void)pArg;
    evt_service_event_t evt;

    while (1) {
        if (xQueueReceive(s_evt_queue, &evt, portMAX_DELAY)) {
            ESP_LOGD(TAG, "Event: source=%d, type=%d, value=%d", evt.source, evt.type, evt.value);

            switch (evt.source) {
            case EVT_SOURCE_PIR:
                if (evt.type == EVT_TYPE_TRIGGERED) {
                    ESP_LOGI(TAG, "Motion detected");
                    gpio_set_level(LED_PIN, 1);
                    s_pir = true;
                    publish_security_io(BRIDGE_EVT_SEC_FLAG_PIR);
                } else if (evt.type == EVT_TYPE_SUSTAINED) {
                    ESP_LOGI(TAG, "Motion sustained");
                    gpio_set_level(LED_PIN, 1);
                } else if (evt.type == EVT_TYPE_CLEARED) {
                    ESP_LOGI(TAG, "Motion ended");
                    gpio_set_level(LED_PIN, 0);
                    s_pir = false;
                    publish_security_io(BRIDGE_EVT_SEC_FLAG_PIR);
                }
                break;

            case EVT_SOURCE_DOORBELL:
                if (evt.type == EVT_TYPE_TRIGGERED) {
                    ESP_LOGI(TAG, "Doorbell pressed");
                    gpio_set_level(LED_PIN, 1);
                    s_doorbell = true;
                    publish_security_io(BRIDGE_EVT_SEC_FLAG_DOORBELL);
                } else if (evt.type == EVT_TYPE_CLEARED) {
                    ESP_LOGI(TAG, "Doorbell released");
                    gpio_set_level(LED_PIN, 0);
                    s_doorbell = false;
                    publish_security_io(BRIDGE_EVT_SEC_FLAG_DOORBELL);
                }
                break;

            case EVT_SOURCE_PANIC:
                if (evt.type == EVT_TYPE_TRIGGERED) {
                    ESP_LOGE(TAG, "TAMPER open — mount break / unit removed");
                    gpio_set_level(LED_PIN, 1);
                    panic_alarm_task_init();
                    s_tamper = true;
                    publish_security_io(BRIDGE_EVT_SEC_FLAG_TAMPER);
                } else if (evt.type == EVT_TYPE_CLEARED) {
                    /* Remount — do not silence siren (HA / C6 OnOff clears). */
                    ESP_LOGW(TAG, "Tamper cleared — unit seated (siren stays latched until C6 clears)");
                    s_tamper = false;
                    publish_security_io(BRIDGE_EVT_SEC_FLAG_TAMPER);
                }
                break;

            case EVT_SOURCE_ALARM:
                if (evt.type == EVT_TYPE_TRIGGERED) {
                    ESP_LOGW(TAG, "Siren OnOff ON — panic alarm start");
                    panic_alarm_task_init();
                } else if (evt.type == EVT_TYPE_CLEARED) {
                    ESP_LOGI(TAG, "Siren OnOff OFF — panic alarm clear");
                    panic_alarm_task_deinit();
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
    if (s_evt_queue != NULL) {
        ESP_LOGW(TAG, "Event service already initialized");
        return ESP_OK;
    }

    s_evt_queue = xQueueCreate(EVT_QUEUE_SIZE, sizeof(evt_service_event_t));
    if (!s_evt_queue) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_ERR_NO_MEM;
    }

    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(LED_PIN, 0);

    BaseType_t ret = xTaskCreatePinnedToCore(evt_service_task_handler, "evt_service_task_handler",
                                             EVT_SERVICE_TASK_STACK_SIZE, NULL, EVT_SERVICE_TASK_PRIORITY, NULL,
                                             EVT_SERVICE_TASK_CORE_ID);
    if (ret != pdPASS) {
        vQueueDelete(s_evt_queue);
        s_evt_queue = NULL;
        ESP_LOGE(TAG, "Failed to create event service task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Event service initialized (bridge EVT to C6)");
    return ESP_OK;
}

esp_err_t evt_service_post(evt_service_event_t *evt)
{
    if (!s_evt_queue) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!evt) {
        return ESP_ERR_INVALID_ARG;
    }

    evt->timestamp = esp_log_timestamp();
    if (xQueueSend(s_evt_queue, evt, pdMS_TO_TICKS(100)) != pdPASS) {
        ESP_LOGW(TAG, "Event queue full, dropping source %d", evt->source);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t evt_service_post_from_isr(evt_service_event_t *evt, BaseType_t *woken)
{
    if (!s_evt_queue || evt == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    evt->timestamp = (uint32_t)(xTaskGetTickCountFromISR() * portTICK_PERIOD_MS);
    if (xQueueSendFromISR(s_evt_queue, evt, woken) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
