/**
 * @file led_effects_task.cpp
 * @brief Built-in color-temperature presets cycled from the user button.
 */

#include <esp_log.h>

#include <esp_matter.h>

#include <led_effects_task.h>

using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::attribute;

static const char *TAG = "led_effects";

typedef struct {
    const char *name;
    uint16_t mireds;
    uint8_t level;
    bool on;
} lamp_preset_t;

static const lamp_preset_t s_presets[] = {
    {"warm", 454, 180, true},
    {"daylight", 370, 220, true},
    {"cool", 250, 220, true},
    {"relax", 500, 120, true},
    {"night", 600, 40, true},
};

static size_t s_preset_index = 0;

static void apply_preset(uint16_t endpoint_id, const lamp_preset_t *preset)
{
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);

    val = esp_matter_bool(preset->on);
    attribute::update(endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id, &val);

    val = esp_matter_uint8(preset->level);
    attribute::update(endpoint_id, LevelControl::Id, LevelControl::Attributes::CurrentLevel::Id, &val);

    val = esp_matter_uint8((uint8_t)ColorControl::ColorMode::kColorTemperature);
    attribute::update(endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorMode::Id, &val);
    attribute::update(endpoint_id, ColorControl::Id, ColorControl::Attributes::EnhancedColorMode::Id, &val);

    val = esp_matter_uint16(preset->mireds);
    attribute::update(endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorTemperatureMireds::Id, &val);
}

void led_effects_cycle_preset(uint16_t endpoint_id)
{
    s_preset_index = (s_preset_index + 1) % (sizeof(s_presets) / sizeof(s_presets[0]));
    const lamp_preset_t *preset = &s_presets[s_preset_index];
    ESP_LOGI(TAG, "Preset: %s (%u mireds)", preset->name, preset->mireds);
    apply_preset(endpoint_id, preset);
}
