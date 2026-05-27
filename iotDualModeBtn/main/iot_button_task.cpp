#include <esp_log.h>
#include <stdlib.h>
#include <string.h>

#include <app-common/zap-generated/attributes/Accessors.h>
#include <esp_matter.h>

#include <button_gpio.h>
#include <iot_button.h>

#include <iot_button_task.h>

using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::cluster;

static const char *TAG = "iot_button_task";
extern uint16_t iot_button_endpoint_id;

static int current_number_of_presses_counted = 1;
static bool is_multipress = 0;
static uint8_t idlePosition = 0;

static void iot_button_task_initial_pressed(void *arg, void *data)
{
    if (!is_multipress) {
        ESP_LOGI(TAG, "Initial button pressed");
        int switch_endpoint_id = iot_button_endpoint_id;

        // Press moves Position from 0 (idle) to 1 (press)
        uint8_t newPosition = 1;
        chip::DeviceLayer::SystemLayer().ScheduleLambda([switch_endpoint_id, newPosition]() {
            chip::app::Clusters::Switch::Attributes::CurrentPosition::Set(switch_endpoint_id, newPosition);
            // InitialPress event takes newPosition as event data
            switch_cluster::event::send_initial_press(switch_endpoint_id, newPosition);
        });
        is_multipress = 1;

        gpio_set_level(SINGLE_PRESS_LED_PIN, 1);
    }
}

static void iot_button_task_release(void *arg, void *data)
{
    ESP_LOGI(TAG, "Button released");
    int switch_endpoint_id = iot_button_endpoint_id;

    chip::DeviceLayer::SystemLayer().ScheduleLambda([switch_endpoint_id]() {
        chip::app::Clusters::Switch::Attributes::CurrentPosition::Set(switch_endpoint_id, idlePosition);
    });

    gpio_set_level(SINGLE_PRESS_LED_PIN, 0);
}

static void iot_button_task_long_pressed(void *arg, void *data)
{
    ESP_LOGI(TAG, "Button long pressed");
    int switch_endpoint_id = iot_button_endpoint_id;

    // Press moves Position from 0 (idle) to 1 (press)
    uint8_t newPosition = 1;
    chip::DeviceLayer::SystemLayer().ScheduleLambda([switch_endpoint_id, newPosition]() {
        chip::app::Clusters::Switch::Attributes::CurrentPosition::Set(switch_endpoint_id, newPosition);
        // LongPress event takes newPosition as event data
        switch_cluster::event::send_long_press(switch_endpoint_id, newPosition);
    });
}

static void iot_button_task_multipress_ongoing(void *arg, void *data)
{
    ESP_LOGI(TAG, "Multipress Ongoing");
    int switch_endpoint_id = iot_button_endpoint_id;

    // Press moves Position from 0 (idle) to 1 (press)
    uint8_t newPosition = 1;
    current_number_of_presses_counted++;
    uint16_t endpoint_id = iot_button_endpoint_id;
    uint32_t cluster_id = Switch::Id;
    uint32_t attribute_id = Switch::Attributes::FeatureMap::Id;

    attribute_t *attribute = attribute::get(endpoint_id, cluster_id, attribute_id);

    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute::get_val(attribute, &val);

    uint32_t feature_map = val.val.u32;
    uint32_t msm_feature_map = switch_cluster::feature::momentary_switch_multi_press::get_id();
    uint32_t as_feature_map = switch_cluster::feature::action_switch::get_id();
    if (((feature_map & msm_feature_map) == msm_feature_map) && ((feature_map & as_feature_map) != as_feature_map)) {
        chip::DeviceLayer::SystemLayer().ScheduleLambda([switch_endpoint_id, newPosition]() {
            chip::app::Clusters::Switch::Attributes::CurrentPosition::Set(switch_endpoint_id, newPosition);
            // MultiPress Ongoing event takes newPosition and current_number_of_presses_counted as event data
            switch_cluster::event::send_multi_press_ongoing(switch_endpoint_id, newPosition, current_number_of_presses_counted);
        });
    }

    gpio_set_level(MULTI_PRESS_LED_PIN, 1);
}

static void iot_button_task_multipress_complete(void *arg, void *data)
{
    ESP_LOGI(TAG, "Multipress Complete");
    int switch_endpoint_id = iot_button_endpoint_id;

    // Press moves Position from 0 (idle) to 1 (press)
    uint8_t previousPosition = 1;
    uint16_t endpoint_id = iot_button_endpoint_id;
    uint32_t cluster_id = Switch::Id;
    uint32_t attribute_id = Switch::Attributes::MultiPressMax::Id;

    attribute_t *attribute = attribute::get(endpoint_id, cluster_id, attribute_id);

    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute::get_val(attribute, &val);
    uint8_t multipress_max = val.val.u8;
    int total_number_of_presses_counted = (current_number_of_presses_counted > multipress_max) ? 0 : current_number_of_presses_counted;
    chip::DeviceLayer::SystemLayer().ScheduleLambda([switch_endpoint_id, previousPosition, total_number_of_presses_counted]() {
        chip::app::Clusters::Switch::Attributes::CurrentPosition::Set(switch_endpoint_id, idlePosition);
        // MultiPress Complete event takes previousPosition and total_number_of_presses_counted as event data
        switch_cluster::event::send_multi_press_complete(switch_endpoint_id, previousPosition, total_number_of_presses_counted);
        // Reset current_number_of_presses_counted to initial value
        current_number_of_presses_counted = 1;
    });

    gpio_set_level(MULTI_PRESS_LED_PIN, 0);

    is_multipress = 0;
}

static void iot_button_task_led_init()
{
    gpio_reset_pin(SINGLE_PRESS_LED_PIN);
    gpio_set_direction(SINGLE_PRESS_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(SINGLE_PRESS_LED_PIN, 0);

    gpio_reset_pin(MULTI_PRESS_LED_PIN);
    gpio_set_direction(MULTI_PRESS_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(MULTI_PRESS_LED_PIN, 0);
}

esp_err_t iot_button_attribute_update(iot_button_task_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                      uint32_t attribute_id, esp_matter_attr_val_t *val)
{
    esp_err_t err = ESP_OK;
    return err;
}

iot_button_task_handle_t iot_button_task_init(iot_button_config_t *button)
{
    /* Initialize button */
    button_handle_t handle = NULL;
    const button_config_t btn_cfg = {0};

    if (button != NULL) {
        const button_gpio_config_t btn_gpio_cfg = {
            .gpio_num = button->button_pin,
            .active_level = 0,
        };
        if (iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &handle) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create button device");
            return NULL;
        }
    } else {
        const button_gpio_config_t btn_gpio_cfg = {
            .gpio_num = BUTTON_GPIO_PIN,
            .active_level = 0,
        };
        if (iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &handle) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create button device");
            return NULL;
        }
    }

    iot_button_register_cb(handle, BUTTON_PRESS_DOWN, NULL, iot_button_task_initial_pressed, button);
    iot_button_register_cb(handle, BUTTON_PRESS_UP, NULL, iot_button_task_release, button);
    iot_button_register_cb(handle, BUTTON_PRESS_REPEAT, NULL, iot_button_task_multipress_ongoing, button);
    iot_button_register_cb(handle, BUTTON_PRESS_REPEAT_DONE, NULL, iot_button_task_multipress_complete, button);

    iot_button_task_led_init();

    return (iot_button_task_handle_t)handle;
}