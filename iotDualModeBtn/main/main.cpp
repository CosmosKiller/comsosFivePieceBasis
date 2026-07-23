/**
 * @file main.c
 * @author Marcel Nahir Samur (mnsamur2014@gmail.com)
 * @brief Matter generic-switch app: NVS init, switch endpoint, semantic tags, and stack start.
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
#include <esp_matter.h>

// Include project libraries
#include <cosmos_battery.h>
#include <cosmos_battery_matter.h>
#include <cosmos_matter_ota.h>
#include <factory_reset_task.h>
#include <iot_button_task.h>
#include <matter_task.h>

static const char *TAG = "app_main";

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace esp_matter::cluster;
using namespace chip::app::Clusters;

namespace
{
    // Please refer to https://github.com/CHIP-Specifications/connectedhomeip-spec/blob/master/src/namespaces
    constexpr const uint8_t kNamespaceSwitches = 0x43;
    // Switches Namespace: 0x43, tag 0 (On)
    constexpr const uint8_t kTagSwitchOn = 0;
    // Switches Namespace: 0x43, tag 1 (Off)
    constexpr const uint8_t kTagSwitchOff = 1;

    constexpr const uint8_t kNamespacePosition = 8;
    // Common Position Namespace: 8, tag: 0 (Left)
    constexpr const uint8_t kTagPositionLeft = 0;
    // Common Position Namespace: 8, tag: 1 (Right)
    constexpr const uint8_t kTagPositionRight = 1;

    const Descriptor::Structs::SemanticTagStruct::Type gEp1TagList[] = {
        {.namespaceID = kNamespaceSwitches, .tag = kTagSwitchOn},
        {.namespaceID = kNamespacePosition, .tag = kTagPositionRight}};
    const Descriptor::Structs::SemanticTagStruct::Type gEp2TagList[] = {
        {.namespaceID = kNamespaceSwitches, .tag = kTagSwitchOff},
        {.namespaceID = kNamespacePosition, .tag = kTagPositionLeft}};

}

// Definitons
uint16_t iot_button_endpoint_id = 0;

// Function declarations
static esp_err_t create_button(iot_button_config_t *button_config, node_t *node);

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

    err = create_button(NULL, node);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create button: %d", err);
        return;
    }

    cosmos_battery_config_t battery_config;
    cosmos_battery_config_set_defaults(&battery_config);
    battery_config.endpoint_id = cosmos_battery_matter_add_endpoint(node);
    if (battery_config.endpoint_id == 0) {
        ESP_LOGE(TAG, "Failed to create battery endpoint");
        return;
    }
    err = cosmos_battery_init(&battery_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "cosmos_battery_init failed: %d", err);
        return;
    }

    // Start Matter stack (this starts transports, commissioning, etc.)
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

    endpoint_t *ep1 = endpoint::get(1);
    endpoint::set_semantic_tags(ep1, gEp1TagList, 2);
    endpoint_t *ep2 = endpoint::get(2);
    endpoint::set_semantic_tags(ep2, gEp2TagList, 2);

    err = cosmos_battery_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "cosmos_battery_start failed: %d", err);
        return;
    }

    // Start factory reset task
    factory_reset_task();
}

static esp_err_t create_button(iot_button_config_t *button_config, node_t *node)
{
    esp_err_t err = ESP_OK;

    /* Initialize driver */
    iot_button_task_handle_t button_handle = iot_button_task_init(button_config);
    if (button_handle == NULL) {
        ESP_LOGE(TAG, "Failed to initialize button driver");
        return ESP_FAIL;
    }

    /* Create a new endpoint. */
    generic_switch::config_t switch_config;
    switch_config.switch_cluster.feature_flags = cluster::switch_cluster::feature::momentary_switch::get_id();

    endpoint_t *endpoint = generic_switch::create(node, &switch_config, ENDPOINT_FLAG_NONE, button_handle);

    cluster_t *descriptor = cluster::get(endpoint, Descriptor::Id);
    descriptor::feature::tag_list::add(descriptor);

    /* These node and endpoint handles can be used to create/add other endpoints and clusters. */
    if (!node || !endpoint) {
        ESP_LOGE(TAG, "Matter node creation failed");
        err = ESP_FAIL;
        return err;
    }

    iot_button_endpoint_id = endpoint::get_id(endpoint);
    ESP_LOGI(TAG, "Generic Switch created with endpoint_id %d", iot_button_endpoint_id);

    /* Add additional features to the node */
    cluster_t *cluster = cluster::get(endpoint, Switch::Id);

    cluster::switch_cluster::feature::action_switch::add(cluster);
    cluster::switch_cluster::feature::momentary_switch_multi_press::config_t msm;
    msm.multi_press_max = 5;
    cluster::switch_cluster::feature::momentary_switch_multi_press::add(cluster, &msm);

    return err;
}