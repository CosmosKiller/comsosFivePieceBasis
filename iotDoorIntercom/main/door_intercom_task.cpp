#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <lib/support/CodeUtils.h>

#include <door_intercom_task.h>
#include <evt_service_task.h>

using namespace chip::app::Clusters;
using namespace esp_matter;

static const char *TAG = "door_intercom_task";
extern uint16_t intercom_endpoint_id;
extern uint16_t occupancy_endpoint_id;
extern uint16_t tamper_endpoint_id;
extern uint16_t siren_endpoint_id;
extern uint16_t doorlock_endpoint_id;

typedef struct {
    door_intercom_config_t *config;
    bool is_initialized;
} door_intercom_ctx_t;

static door_intercom_ctx_t s_ctx;

/**
 * @brief ISR handler for doorbell button
 *
 * @param pArg
 */
static void IRAM_ATTR door_intercom_task_doorbell_isr_handler(void *pArg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int level = gpio_get_level(DOORBELL_PIN);

    /* Matter updates run from evt_service task — never call Matter APIs from ISR. */
    evt_service_event_t evt = {
        .source = EVT_SOURCE_DOORBELL,
        .type = level ? EVT_TYPE_TRIGGERED : EVT_TYPE_CLEARED,
        .value = level,
    };
    evt_service_post_from_isr(&evt, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

esp_err_t door_intercom_attribute_update(door_intercom_task_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val)
{
    esp_err_t err = ESP_OK;

    if (endpoint_id == intercom_endpoint_id) {
        if (cluster_id == OnOff::Id) {
            if (attribute_id == OnOff::Attributes::OnOff::Id) {
                evt_service_event_t evt = {
                    .source = EVT_SOURCE_INTERCOM,
                    .type = (val->val.b) ? EVT_TYPE_TRIGGERED : EVT_TYPE_CLEARED,
                    .value = val->val.i,
                };
                evt_service_post(&evt);
            }
        }
    } else if (endpoint_id == siren_endpoint_id) {
        if (cluster_id == OnOff::Id) {
            if (attribute_id == OnOff::Attributes::OnOff::Id) {
                evt_service_event_t evt = {
                    .source = EVT_SOURCE_ALARM,
                    .type = (val->val.b) ? EVT_TYPE_TRIGGERED : EVT_TYPE_CLEARED,
                    .value = val->val.i,
                };
                evt_service_post(&evt);
            }
        }
    } else if (endpoint_id == doorlock_endpoint_id) {
        if (cluster_id == OnOff::Id) {
            if (attribute_id == OnOff::Attributes::OnOff::Id) {
                // Handle door lock attribute update if needed
            }
        }
    } else if (endpoint_id == occupancy_endpoint_id || endpoint_id == tamper_endpoint_id) {
        /* PIR/tamper Matter updates originate in main.cpp notification callbacks. */
    } else if (endpoint_id != 0) {
        ESP_LOGW(TAG, "Unknown endpoint ID: %d", endpoint_id);
    }
    return err;
}

esp_err_t door_intercom_task_init(door_intercom_config_t *pConfig)
{
    if (pConfig == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_ctx.is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // Doorbell button config
    gpio_config_t doorbell_conf = {
        .pin_bit_mask = (1ULL << DOORBELL_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE};
    gpio_config(&doorbell_conf);

    gpio_isr_handler_add(DOORBELL_PIN, door_intercom_task_doorbell_isr_handler, NULL);

    ESP_LOGI(TAG, "Door intercom initialized (DOORBELL=%d)", DOORBELL_PIN);

    s_ctx.config = pConfig;
    s_ctx.is_initialized = true;
    return ESP_OK;
}