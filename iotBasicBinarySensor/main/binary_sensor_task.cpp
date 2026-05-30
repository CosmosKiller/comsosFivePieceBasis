#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <lib/support/CodeUtils.h>

#include <binary_sensor_task.h>
#include <evt_service_task.h>

using namespace chip::app::Clusters;
using namespace esp_matter;

static const char *TAG = "binary_sensor_task";
extern uint16_t alarm_led_endpoint_id;

typedef struct {
    binary_sensor_config_t *config;
    bool is_initialized;
} binary_sensor_ctx_t;

static binary_sensor_ctx_t s_ctx;
static bool is_armed = false; // Track alarm state

/**
 * @brief ISR handler for binary sensor GPIO
 *
 * @param pArg
 */
static void IRAM_ATTR binary_sensor_isr_handler(void *pArg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    static bool level = false;
    bool new_level = gpio_get_level(SENSOR_PIN);

    // we only need to notify application layer if level changed
    if (level != new_level) {

        level = new_level;
        if (s_ctx.config->cb) {
            s_ctx.config->cb(s_ctx.config->endpoint_id, new_level, s_ctx.config->user_data);
        }

        if (!new_level && is_armed) { // Only consider trigger on falling edge (Door opened)
            evt_service_event_t evt = {
                .source = EVT_SOURCE_PANIC,
                .type = EVT_TYPE_TRIGGERED,
                .value = 1,
            };
            evt_service_post(&evt);
        } else {

            // Post event to service
            evt_service_event_t evt = {
                .source = EVT_SOURCE_SENSOR,
                .type = level ? EVT_TYPE_CLEARED : EVT_TYPE_TRIGGERED, // Cleared on rising edge (Door closed)
                .value = level ? 1 : 0,
            };
            evt_service_post(&evt);
        }
    }

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

/**
 * @brief Initialize GPIO for binary sensor input
 *
 */
static void binary_sensor_gpio_init(void)
{
    gpio_reset_pin(SENSOR_PIN);
    gpio_set_intr_type(SENSOR_PIN, GPIO_INTR_ANYEDGE);
    gpio_set_direction(SENSOR_PIN, GPIO_MODE_INPUT);

    gpio_set_pull_mode(SENSOR_PIN, GPIO_PULLDOWN_ONLY);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(SENSOR_PIN, binary_sensor_isr_handler, NULL);
}

esp_err_t binary_sensor_attribute_update(binary_sensor_task_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val)
{
    esp_err_t err = ESP_OK;

    if (endpoint_id == alarm_led_endpoint_id) {
        if (cluster_id == OnOff::Id) {
            if (attribute_id == OnOff::Attributes::OnOff::Id) {

                if (val->val.b) {
                    is_armed = true;
                } else {
                    is_armed = false;
                }

                evt_service_event_t evt = {
                    .source = EVT_SOURCE_ALARM,
                    .type = (val->val.b) ? EVT_TYPE_TRIGGERED : EVT_TYPE_CLEARED,
                    .value = val->val.i,
                };
                evt_service_post(&evt);
            }
        }
    } else {
        ESP_LOGW(TAG, "Received attribute update for unknown endpoint: %d", endpoint_id);
    }

    return err;
}

esp_err_t binary_sensor_task_init(binary_sensor_config_t *pConfig)
{
    if (pConfig == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_ctx.is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // Initialize GPIO for binary sensor
    binary_sensor_gpio_init();

    s_ctx.config = pConfig;
    s_ctx.is_initialized = true;
    return ESP_OK;
}
