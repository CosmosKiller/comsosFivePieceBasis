/**
 * @file matter_task.cpp
 * @brief Matter callbacks for the environmental sensor (shared events; optional LVGL hook).
 */

#if CONFIG_ENABLE_LVGL_UI
#include <lvgl_task.h>
#endif

#include <cosmos_matter_events.h>
#include <matter_task.h>

using namespace esp_matter;
using namespace esp_matter::attribute;

#if CONFIG_ENABLE_LVGL_UI
namespace
{
void commissioning_complete_hook(void)
{
    lvgl_task_device_commissioned();
}

struct LvglCommissioningHook {
    LvglCommissioningHook()
    {
        cosmos_matter_set_commissioning_complete_hook(commissioning_complete_hook);
    }
};

static LvglCommissioningHook s_lvgl_commissioning_hook;
} // namespace
#endif

void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    cosmos_matter_handle_device_event(event, arg);
}

esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                uint8_t effect_variant, void *priv_data)
{
    return cosmos_matter_app_identification_cb(type, endpoint_id, effect_id, effect_variant, priv_data);
}

esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                  uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    (void)endpoint_id;
    (void)cluster_id;
    (void)attribute_id;
    (void)val;
    (void)priv_data;
    (void)type;
    return ESP_OK;
}
