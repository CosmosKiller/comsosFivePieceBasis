/**
 * @file main.cpp
 * @brief Matter bedside lamp — extended color light with WS2812 and separate user/reset buttons.
 */

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_matter.h>

#include <cosmos_matter_ota.h>
#include <factory_reset_task.h>
#include <lamp_task.h>
#include <matter_task.h>
#include <user_button_task.h>

static const char *TAG = "app_main";

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

uint16_t light_endpoint_id = 0;

extern "C" void app_main(void)
{
    esp_err_t err = ESP_OK;

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS needs erase (err=%d). Erasing and retrying...", err);
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %d", err);
        return;
    }

    lamp_task_handle_t lamp_handle = lamp_task_init();
    if (!lamp_handle) {
        ESP_LOGE(TAG, "lamp_task_init failed");
        return;
    }

    node::config_t node_cfg;
    node_t *node = node::create(&node_cfg, app_attribute_update_cb, app_identification_cb);
    if (!node) {
        ESP_LOGE(TAG, "Failed to create Matter node");
        return;
    }

    extended_color_light::config_t light_config;
    light_config.on_off.on_off = DEFAULT_POWER;
    light_config.on_off_lighting.start_up_on_off = nullptr;
    light_config.level_control.current_level = DEFAULT_BRIGHTNESS;
    light_config.level_control.on_level = DEFAULT_BRIGHTNESS;
    light_config.level_control_lighting.start_up_current_level = DEFAULT_BRIGHTNESS;
    light_config.color_control.color_mode = (uint8_t)ColorControl::ColorMode::kColorTemperature;
    light_config.color_control.enhanced_color_mode = (uint8_t)ColorControl::ColorMode::kColorTemperature;
    light_config.color_control_color_temperature.color_temperature_mireds = 370;
    light_config.color_control_color_temperature.start_up_color_temperature_mireds = nullptr;

    endpoint_t *light_ep = extended_color_light::create(node, &light_config, ENDPOINT_FLAG_NONE, lamp_handle);
    if (!light_ep) {
        ESP_LOGE(TAG, "Failed to create extended color light endpoint");
        return;
    }

    light_endpoint_id = endpoint::get_id(light_ep);
    ESP_LOGI(TAG, "Light endpoint ID: %d", light_endpoint_id);

    attribute_t *current_level_attribute =
        attribute::get(light_endpoint_id, LevelControl::Id, LevelControl::Attributes::CurrentLevel::Id);
    attribute::set_deferred_persistence(current_level_attribute);

    attribute_t *color_temp_attribute =
        attribute::get(light_endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorTemperatureMireds::Id);
    attribute::set_deferred_persistence(color_temp_attribute);

    err = esp_matter::start(app_event_cb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_matter::start failed: %d", err);
        return;
    }

    err = cosmos_matter_ota_configure();
    if (err != ESP_OK && err != ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGE(TAG, "cosmos_matter_ota_configure failed: %d", err);
        return;
    }

    err = lamp_task_set_defaults(light_endpoint_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "lamp_task_set_defaults failed: %d", err);
        return;
    }

    err = user_button_task_start(light_endpoint_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "user_button_task_start failed: %d", err);
        return;
    }

    factory_reset_task();
}
