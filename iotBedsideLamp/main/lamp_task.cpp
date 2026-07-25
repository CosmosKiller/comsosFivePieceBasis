/**
 * @file lamp_task.cpp
 * @brief WS2812 driver bridge for the extended color light endpoint.
 */

#include <esp_log.h>

#include <driver/gpio.h>
#include <device.h>
#include <led_driver.h>

#include <lamp_task.h>

#include <sdkconfig.h>

using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::attribute;

static const char *TAG = "lamp_task";

static uint16_t s_current_x = 0;
static uint16_t s_current_y = 0;

#define REMAP_TO_RANGE(value, from, to) ((value * to) / from)
#define REMAP_TO_RANGE_INVERSE(value, factor) (factor / ((value) ? (value) : 1))

static esp_err_t lamp_set_power(led_driver_handle_t handle, esp_matter_attr_val_t *val)
{
    return led_driver_set_power(handle, val->val.b);
}

static esp_err_t lamp_set_brightness(led_driver_handle_t handle, esp_matter_attr_val_t *val)
{
    int value = REMAP_TO_RANGE(val->val.u8, MATTER_BRIGHTNESS, STANDARD_BRIGHTNESS);
    return led_driver_set_brightness(handle, value);
}

static esp_err_t lamp_set_hue(led_driver_handle_t handle, esp_matter_attr_val_t *val)
{
    int value = REMAP_TO_RANGE(val->val.u8, MATTER_HUE, STANDARD_HUE);
    return led_driver_set_hue(handle, value);
}

static esp_err_t lamp_set_saturation(led_driver_handle_t handle, esp_matter_attr_val_t *val)
{
    int value = REMAP_TO_RANGE(val->val.u8, MATTER_SATURATION, STANDARD_SATURATION);
    return led_driver_set_saturation(handle, value);
}

static esp_err_t lamp_set_temperature(led_driver_handle_t handle, esp_matter_attr_val_t *val)
{
    uint32_t value = REMAP_TO_RANGE_INVERSE(val->val.u16, STANDARD_TEMPERATURE_FACTOR);
    return led_driver_set_temperature(handle, value);
}

static esp_err_t lamp_set_xy(led_driver_handle_t handle, uint16_t x, uint16_t y)
{
    return led_driver_set_xy(handle, x, y);
}

lamp_task_handle_t lamp_task_init(void)
{
    led_driver_config_t config = led_driver_get_config();
    config.gpio = (gpio_num_t)CONFIG_BEDSIDE_LAMP_LED_GPIO;
    led_driver_handle_t handle = led_driver_init(&config);
    if (!handle) {
        ESP_LOGE(TAG, "led_driver_init failed");
        return NULL;
    }
    ESP_LOGI(TAG, "WS2812 driver ready on GPIO%d", config.gpio);
    return (lamp_task_handle_t)handle;
}

esp_err_t lamp_task_attribute_update(lamp_task_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                     uint32_t attribute_id, esp_matter_attr_val_t *val)
{
    esp_err_t err = ESP_OK;
    led_driver_handle_t handle = (led_driver_handle_t)driver_handle;

    if (cluster_id == OnOff::Id) {
        if (attribute_id == OnOff::Attributes::OnOff::Id) {
            err = lamp_set_power(handle, val);
        }
    } else if (cluster_id == LevelControl::Id) {
        if (attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
            err = lamp_set_brightness(handle, val);
        }
    } else if (cluster_id == ColorControl::Id) {
        if (attribute_id == ColorControl::Attributes::CurrentHue::Id) {
            err = lamp_set_hue(handle, val);
        } else if (attribute_id == ColorControl::Attributes::CurrentSaturation::Id) {
            err = lamp_set_saturation(handle, val);
        } else if (attribute_id == ColorControl::Attributes::ColorTemperatureMireds::Id) {
            err = lamp_set_temperature(handle, val);
        } else if (attribute_id == ColorControl::Attributes::CurrentX::Id) {
            s_current_x = val->val.u16;
            err = lamp_set_xy(handle, s_current_x, s_current_y);
        } else if (attribute_id == ColorControl::Attributes::CurrentY::Id) {
            s_current_y = val->val.u16;
            err = lamp_set_xy(handle, s_current_x, s_current_y);
        }
    }

    return err;
}

esp_err_t lamp_task_set_defaults(uint16_t endpoint_id)
{
    esp_err_t err = ESP_OK;
    void *priv_data = endpoint::get_priv_data(endpoint_id);
    led_driver_handle_t handle = (led_driver_handle_t)priv_data;
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);

    attribute_t *attribute = attribute::get(endpoint_id, LevelControl::Id, LevelControl::Attributes::CurrentLevel::Id);
    attribute::get_val(attribute, &val);
    err |= lamp_set_brightness(handle, &val);

    attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorMode::Id);
    attribute::get_val(attribute, &val);
    if (val.val.u8 == (uint8_t)ColorControl::ColorMode::kCurrentHueAndCurrentSaturation) {
        attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentHue::Id);
        attribute::get_val(attribute, &val);
        err |= lamp_set_hue(handle, &val);

        attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentSaturation::Id);
        attribute::get_val(attribute, &val);
        err |= lamp_set_saturation(handle, &val);
    } else if (val.val.u8 == (uint8_t)ColorControl::ColorMode::kColorTemperature) {
        attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorTemperatureMireds::Id);
        attribute::get_val(attribute, &val);
        err |= lamp_set_temperature(handle, &val);
    } else if (val.val.u8 == (uint8_t)ColorControl::ColorMode::kCurrentXAndCurrentY) {
        attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentX::Id);
        attribute::get_val(attribute, &val);
        s_current_x = val.val.u16;

        attribute = attribute::get(endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentY::Id);
        attribute::get_val(attribute, &val);
        s_current_y = val.val.u16;
        err |= lamp_set_xy(handle, s_current_x, s_current_y);
    }

    attribute = attribute::get(endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id);
    attribute::get_val(attribute, &val);
    err |= lamp_set_power(handle, &val);

    return err;
}
