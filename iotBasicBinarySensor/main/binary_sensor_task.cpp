/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <lib/support/CodeUtils.h>

#include <binary_sensor_task.h>

using namespace chip::app::Clusters;
using namespace esp_matter;

static const char *TAG = "binary_sensor_task";
extern uint16_t alarm_led_endpoint_id;

typedef struct {
    binary_sensor_config_t *config;
    bool is_initialized;
} binary_sensor_ctx_t;

static binary_sensor_ctx_t s_ctx;

/**
 * @brief ISR handler for binary sensor GPIO
 *
 * @param pArg
 */
static void IRAM_ATTR binary_sensor_isr_handler(void *pArg)
{
    static bool level = false;
    bool new_level = gpio_get_level(SENSOR_PIN);

    // we only need to notify application layer if level changed
    if (level != new_level) {
        level = new_level;
        if (s_ctx.config->cb) {
            s_ctx.config->cb(s_ctx.config->endpoint_id, new_level, s_ctx.config->user_data);
        }
        gpio_set_level(STATE_LED_PIN, level);
    }
}

static void binary_sensor_gpio_init(gpio_num_t pin)
{
    gpio_reset_pin(pin);
    gpio_set_intr_type(pin, GPIO_INTR_ANYEDGE);
    gpio_set_direction(pin, GPIO_MODE_INPUT);

    gpio_set_pull_mode(pin, GPIO_PULLDOWN_ONLY);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(pin, binary_sensor_isr_handler, NULL);
}

esp_err_t binary_sensor_attribute_update(binary_sensor_task_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val)
{
    esp_err_t err = ESP_OK;

    if (endpoint_id == alarm_led_endpoint_id) {
        if (cluster_id == OnOff::Id) {
            if (attribute_id == OnOff::Attributes::OnOff::Id) {
                gpio_set_level(ALARM_LED_PIN, val->val.b);
            }
        }
    } else {
        ESP_LOGW(TAG, "Received attribute update for unknown endpoint: %d", endpoint_id);
    }

    return err;
}

esp_err_t binary_sensor_task_init(binary_sensor_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_ctx.is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    binary_sensor_gpio_init(SENSOR_PIN);

    // STATE LED config
    gpio_config_t state_led_conf = {
        .pin_bit_mask = (1ULL << STATE_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&state_led_conf);
    gpio_set_level(STATE_LED_PIN, 0);

    // ALARM LED config
    gpio_config_t alarm_led_conf = {
        .pin_bit_mask = (1ULL << ALARM_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&alarm_led_conf);
    gpio_set_level(ALARM_LED_PIN, 0);

    s_ctx.config = config;
    return ESP_OK;
}
