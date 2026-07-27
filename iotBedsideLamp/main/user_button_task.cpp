/**
 * @file user_button_task.cpp
 * @brief User button: single-click toggle, long-press cycle preset.
 */

#include <esp_log.h>

#include <button_gpio.h>
#include <driver/gpio.h>
#include <iot_button.h>

#include <esp_matter.h>

#include <led_effects_task.h>
#include <user_button_task.h>

#include <sdkconfig.h>

using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::attribute;

static const char *TAG = "user_button_task";

static uint16_t s_light_endpoint_id = 0;

static void user_button_toggle_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "Toggle");
    uint16_t endpoint_id = s_light_endpoint_id;

    attribute_t *attribute = attribute::get(endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id);
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute::get_val(attribute, &val);
    val.val.b = !val.val.b;
    attribute::update(endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id, &val);
}

static void user_button_long_press_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "Long press — cycle preset");
    led_effects_cycle_preset(s_light_endpoint_id);
}

static void user_button_multipress_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "Multi-press — cycle preset");
    led_effects_cycle_preset(s_light_endpoint_id);
}

esp_err_t user_button_task_start(uint16_t light_endpoint_id)
{
    s_light_endpoint_id = light_endpoint_id;

    button_handle_t handle = NULL;
    button_config_t btn_cfg = {
        .long_press_time = CONFIG_BEDSIDE_LAMP_USER_LONG_PRESS_MS,
    };
    const button_gpio_config_t btn_gpio_cfg = {
        .gpio_num = (gpio_num_t)CONFIG_BEDSIDE_LAMP_USER_BUTTON_GPIO,
        .active_level = 0,
    };

    if (iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create user button on GPIO%d", CONFIG_BEDSIDE_LAMP_USER_BUTTON_GPIO);
        return ESP_FAIL;
    }

    iot_button_register_cb(handle, BUTTON_SINGLE_CLICK, NULL, user_button_toggle_cb, NULL);
    iot_button_register_cb(handle, BUTTON_LONG_PRESS_START, NULL, user_button_long_press_cb, NULL);
    iot_button_register_cb(handle, BUTTON_DOUBLE_CLICK, NULL, user_button_multipress_cb, NULL);

    ESP_LOGI(TAG, "User button on GPIO%d (reset is separate on GPIO9)", CONFIG_BEDSIDE_LAMP_USER_BUTTON_GPIO);
    return ESP_OK;
}
