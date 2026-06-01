/**
 * @file main.c
 * @author Marcel Nahir Samur (mnsamur2014@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-06-09
 *
 * @copyright Copyright (c) 2024
 *
 */

// Include ESP-IDF libraries
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

// Include ESP-MATTER libraries
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/boolean-state-server/CodegenIntegration.h>
#include <esp_matter.h>
#include <esp_matter_ota.h>

// Include project libraries
#include <binary_sensor_task.h>
#include <evt_service_task.h>
#include <factory_reset_task.h>
#include <matter_task.h>

static const char *TAG = "app_main";

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace esp_matter::cluster;
using namespace chip::app::Clusters;

// Definitions
uint16_t alarm_led_endpoint_id = 0;

// Function declarations
static void binary_sensor_notification(uint16_t endpoint_id, bool triggered, void *user_data);

extern "C" void app_main()
{

    esp_err_t err = ESP_OK;

    // Robust NVS init
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

    // Create a Matter node and add the mandatory Root Node device type on endpoint 0
    node::config_t node_cfg;
    node_t *node = node::create(&node_cfg, app_attribute_update_cb, app_identification_cb);
    if (!node) {
        ESP_LOGE(TAG, "Failed to create Matter node");
        return;
    }

#if CONFIG_BINARY_SENSOR_TYPE_CONTACT
    // Add binary sensor endpoint
    contact_sensor::config_t contact_sensor_config;
    endpoint_t *contact_sensor_ep = contact_sensor::create(node, &contact_sensor_config, ENDPOINT_FLAG_NONE, NULL);
    if (!contact_sensor_ep) {
        ESP_LOGE(TAG, "Failed to create contact sensor endpoint");
        return;
    }

    // Initialize binary sensor driver
    static binary_sensor_config_t binary_sensor_config = {
        .cb = binary_sensor_notification,
        .endpoint_id = endpoint::get_id(contact_sensor_ep),
    };
#endif

#if BINARY_SENSOR_TYPE_WATER_LEAK
    // Add binary sensor endpoint
    water_leak_detector::config_t water_leak_sensor_config;
    endpoint_t *water_leak_sensor_ep = water_leak_detector::create(node, &water_leak_sensor_config, ENDPOINT_FLAG_NONE, NULL);
    if (!water_leak_sensor_ep) {
        ESP_LOGE(TAG, "Failed to create water leak sensor endpoint");
        return;
    }

    // Initialize binary sensor driver
    static binary_sensor_config_t binary_sensor_config = {
        .cb = binary_sensor_notification,
        .endpoint_id = endpoint::get_id(water_leak_sensor_ep),
    };
#endif

    ESP_LOGI(TAG, "Binary sensor endpoint created with ID: %d", binary_sensor_config.endpoint_id);

    err = binary_sensor_task_init(&binary_sensor_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "binary_sensor_task_init failed: %d", err);
        return;
    }

    // Add alarm endpoint
    mounted_on_off_control::config_t alarm_config;
    alarm_config.on_off.on_off = false;
    endpoint_t *alarm_ep = mounted_on_off_control::create(node, &alarm_config, ENDPOINT_FLAG_NONE, NULL);
    if (!alarm_ep) {
        ESP_LOGE(TAG, "Failed to create alarm endpoint");
        return;
    }
    alarm_led_endpoint_id = endpoint::get_id(alarm_ep);
    ESP_LOGI(TAG, "Alarm endpoint created with ID: %d", alarm_led_endpoint_id);

    // Initialize event service
    evt_service_init();

    err = esp_matter::ota::requestor_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA requestor initialization failed: %d", err);
        return;
    }

    // Start Matter stack (this starts transports, commissioning, etc.)
    err = esp_matter::start(app_event_cb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_matter::start failed: %d", err);
        return;
    }

    // Start factory reset task
    factory_reset_task();
}

static void binary_sensor_notification(uint16_t endpoint_id, bool triggered, void *user_data)
{
    // schedule the attribute update so that we can report it from matter thread
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, triggered]() {
        ESP_LOGI(TAG, "Binary sensor state changed: endpoint_id=%d, triggered=%d", endpoint_id, triggered);

        auto booleanState = BooleanState::FindClusterOnEndpoint(endpoint_id);
        VerifyOrReturn(booleanState != nullptr);
        booleanState->SetStateValue(triggered);
    });
}