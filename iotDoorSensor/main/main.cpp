/**
 * @file main.cpp
 * @author Marcel Nahir Samur (mnsamur2014@gmail.com)
 * @brief Matter door/window contact sensor: contact endpoint, alarm output, event service, and stack start.
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
#include <esp_matter_providers.h>
#include <platform/ESP32/StaticESP32DeviceInfoProvider.h>

// Include project libraries
#include <binary_sensor_task.h>
#include <cosmos_battery.h>
#include <cosmos_battery_matter.h>
#include <cosmos_matter_ota.h>
#include <door_sensor_matter_notify.h>
#include <evt_service_task.h>
#include <factory_reset_task.h>
#include <matter_task.h>

static const char *TAG = "app_main";

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace esp_matter::cluster;
using namespace chip::app::Clusters;
using namespace chip::DeviceLayer;

using chip::CharSpan;
using chip::Span;

namespace
{
constexpr uint8_t kNamespaceCommonPosition = 8;
constexpr uint8_t kNamespaceSwitches       = 0x43;

const Descriptor::Structs::SemanticTagStruct::Type kContactSemanticTags[] = {
    {.namespaceID = kNamespaceCommonPosition, .tag = 0}, /* Left — primary opening sensor */
};
const Descriptor::Structs::SemanticTagStruct::Type kArmSemanticTags[] = {
    {.namespaceID = kNamespaceSwitches, .tag = 0}, /* On — arm/disarm control */
};
const Descriptor::Structs::SemanticTagStruct::Type kPanicSemanticTags[] = {
    {.namespaceID = kNamespaceCommonPosition, .tag = 1}, /* Right — intrusion indicator */
};
const Descriptor::Structs::SemanticTagStruct::Type kSirenSemanticTags[] = {
    {.namespaceID = kNamespaceSwitches, .tag = 1}, /* Off — siren remote / clear */
};

#if CONFIG_SUPPORT_FIXED_LABEL_CLUSTER && CONFIG_CUSTOM_DEVICE_INFO_PROVIDER
constexpr char kContactDisplayName[] = "Door contact";
constexpr char kArmDisplayName[]     = "Arm / disarm";
constexpr char kPanicDisplayName[]   = "Panic alarm";
constexpr char kSirenDisplayName[]   = "Siren";
constexpr char kFixedLabelNameKey[] = "name";
constexpr char kFixedLabelRoleKey[] = "role";
constexpr char kRoleContact[]        = "door-contact";
constexpr char kRoleArm[]            = "arm-control";
constexpr char kRolePanic[]          = "panic-indicator";
constexpr char kRoleSiren[]          = "siren";

StaticESP32DeviceInfoProvider s_device_info_provider;
StaticESP32DeviceInfoProvider::FixedLabelEntry s_fixed_labels[8];
size_t s_fixed_label_count = 0;

static void door_sensor_matter_add_label(uint16_t endpoint_id, const char *label, const char *value)
{
    if (s_fixed_label_count >= sizeof(s_fixed_labels) / sizeof(s_fixed_labels[0])) {
        return;
    }
    s_fixed_labels[s_fixed_label_count++] = {
        endpoint_id,
        CharSpan::fromCharString(label),
        CharSpan::fromCharString(value),
    };
}
#endif

static void door_sensor_matter_enrich_endpoint(endpoint_t *endpoint)
{
    if (endpoint == nullptr) {
        return;
    }

    cluster_t *descriptor = cluster::get(endpoint, Descriptor::Id);
    if (descriptor != nullptr) {
        descriptor::feature::tag_list::add(descriptor);
    }

#if CONFIG_SUPPORT_FIXED_LABEL_CLUSTER
    cluster::fixed_label::config_t fixed_label_cfg;
    cluster::fixed_label::create(endpoint, &fixed_label_cfg, CLUSTER_FLAG_SERVER);
#endif
}

#if CONFIG_SUPPORT_FIXED_LABEL_CLUSTER && CONFIG_CUSTOM_DEVICE_INFO_PROVIDER
static void door_sensor_matter_register_fixed_labels(uint16_t contact_ep, uint16_t arm_ep, uint16_t panic_ep,
                                                    uint16_t siren_ep)
{
    s_fixed_label_count = 0;
    door_sensor_matter_add_label(contact_ep, kFixedLabelNameKey, kContactDisplayName);
    door_sensor_matter_add_label(contact_ep, kFixedLabelRoleKey, kRoleContact);
    door_sensor_matter_add_label(arm_ep, kFixedLabelNameKey, kArmDisplayName);
    door_sensor_matter_add_label(arm_ep, kFixedLabelRoleKey, kRoleArm);
    door_sensor_matter_add_label(panic_ep, kFixedLabelNameKey, kPanicDisplayName);
    door_sensor_matter_add_label(panic_ep, kFixedLabelRoleKey, kRolePanic);
    door_sensor_matter_add_label(siren_ep, kFixedLabelNameKey, kSirenDisplayName);
    door_sensor_matter_add_label(siren_ep, kFixedLabelRoleKey, kRoleSiren);

    CHIP_ERROR err = s_device_info_provider.SetFixedLabels(
        Span<StaticESP32DeviceInfoProvider::FixedLabelEntry>(s_fixed_labels, s_fixed_label_count));
    if (err != CHIP_NO_ERROR) {
        ESP_LOGW(TAG, "SetFixedLabels failed: %" CHIP_ERROR_FORMAT, err.Format());
        return;
    }

    esp_matter::set_custom_device_info_provider(&s_device_info_provider);
}
#endif

static void door_sensor_matter_apply_semantic_tags(endpoint_t *contact_ep, endpoint_t *arm_ep, endpoint_t *panic_ep,
                                                  endpoint_t *siren_ep)
{
    if (contact_ep != nullptr) {
        endpoint::set_semantic_tags(contact_ep, kContactSemanticTags,
                                    sizeof(kContactSemanticTags) / sizeof(kContactSemanticTags[0]));
    }
    if (arm_ep != nullptr) {
        endpoint::set_semantic_tags(arm_ep, kArmSemanticTags, sizeof(kArmSemanticTags) / sizeof(kArmSemanticTags[0]));
    }
    if (panic_ep != nullptr) {
        endpoint::set_semantic_tags(panic_ep, kPanicSemanticTags, sizeof(kPanicSemanticTags) / sizeof(kPanicSemanticTags[0]));
    }
    if (siren_ep != nullptr) {
        endpoint::set_semantic_tags(siren_ep, kSirenSemanticTags, sizeof(kSirenSemanticTags) / sizeof(kSirenSemanticTags[0]));
    }
}

} // namespace

// Definitions
uint16_t alarm_led_endpoint_id = 0;
uint16_t siren_endpoint_id = 0;
uint16_t panic_indicator_endpoint_id = 0;

static bool s_matter_started = false;

// Function declarations
static void binary_sensor_notification(uint16_t endpoint_id, bool contact_closed, void *user_data);

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

    contact_sensor::config_t contact_sensor_config;
    endpoint_t *contact_sensor_ep = contact_sensor::create(node, &contact_sensor_config, ENDPOINT_FLAG_NONE, NULL);
    if (!contact_sensor_ep) {
        ESP_LOGE(TAG, "Failed to create contact sensor endpoint");
        return;
    }
    door_sensor_matter_enrich_endpoint(contact_sensor_ep);

    static binary_sensor_config_t binary_sensor_config = {
        .cb = binary_sensor_notification,
        .endpoint_id = endpoint::get_id(contact_sensor_ep),
    };

    ESP_LOGI(TAG, "Contact sensor endpoint created with ID: %d", binary_sensor_config.endpoint_id);

    err = binary_sensor_task_init(&binary_sensor_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "binary_sensor_task_init failed: %d", err);
        return;
    }

    // Arm/disarm control — HA OnOff ON starts arming sequence, OFF disarms.
    mounted_on_off_control::config_t alarm_config;
    alarm_config.on_off.on_off = false;
    endpoint_t *alarm_ep = mounted_on_off_control::create(node, &alarm_config, ENDPOINT_FLAG_NONE, NULL);
    if (!alarm_ep) {
        ESP_LOGE(TAG, "Failed to create arm/disarm endpoint");
        return;
    }
    door_sensor_matter_enrich_endpoint(alarm_ep);
    alarm_led_endpoint_id = endpoint::get_id(alarm_ep);
    ESP_LOGI(TAG, "Arm/disarm endpoint created with ID: %d", alarm_led_endpoint_id);

    /* Panic indicator — read-only Boolean State (HA binary_sensor); firmware-only updates.
     * Contact-sensor device type: state_value true = inactive (HA off), false = active (HA on). */
    contact_sensor::config_t panic_indicator_config;
    panic_indicator_config.boolean_state.state_value = true;
    endpoint_t *panic_indicator_ep =
        contact_sensor::create(node, &panic_indicator_config, ENDPOINT_FLAG_NONE, NULL);
    if (!panic_indicator_ep) {
        ESP_LOGE(TAG, "Failed to create panic indicator endpoint");
        return;
    }
    door_sensor_matter_enrich_endpoint(panic_indicator_ep);
    panic_indicator_endpoint_id = endpoint::get_id(panic_indicator_ep);
    ESP_LOGI(TAG, "Panic indicator endpoint created with ID: %d", panic_indicator_endpoint_id);

    /* Siren — HA/Alexa/Google OnOff: ON = sound buzzer, OFF = silence (any automation). */
    mounted_on_off_control::config_t siren_config;
    siren_config.on_off.on_off = false;
    endpoint_t *siren_ep = mounted_on_off_control::create(node, &siren_config, ENDPOINT_FLAG_NONE, NULL);
    if (!siren_ep) {
        ESP_LOGE(TAG, "Failed to create siren endpoint");
        return;
    }
    door_sensor_matter_enrich_endpoint(siren_ep);
    siren_endpoint_id = endpoint::get_id(siren_ep);
    ESP_LOGI(TAG, "Siren endpoint created with ID: %d", siren_endpoint_id);

#if CONFIG_SUPPORT_FIXED_LABEL_CLUSTER && CONFIG_CUSTOM_DEVICE_INFO_PROVIDER
    door_sensor_matter_register_fixed_labels(binary_sensor_config.endpoint_id, alarm_led_endpoint_id,
                                            panic_indicator_endpoint_id, siren_endpoint_id);
#endif

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

    // Initialize event service
    evt_service_init();

    // Start Matter stack (this starts transports, commissioning, etc.)
    err = esp_matter::start(app_event_cb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_matter::start failed: %d", err);
        return;
    }
    s_matter_started = true;

    /* Boolean State cluster boots as false; HA contact sensors show that as ON until we publish. */
    door_sensor_matter_notify_panic(false);

    door_sensor_matter_apply_semantic_tags(contact_sensor_ep, alarm_ep, panic_indicator_ep, siren_ep);

    err = cosmos_matter_ota_configure();
    if (err != ESP_OK && err != ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGE(TAG, "cosmos_matter_ota_configure failed: %d", err);
        return;
    }

    err = binary_sensor_task_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "binary_sensor_task_start failed: %d", err);
        return;
    }

    err = cosmos_battery_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "cosmos_battery_start failed: %d", err);
        return;
    }

    // Start factory reset task
    factory_reset_task();
}

static void binary_sensor_notification(uint16_t endpoint_id, bool contact_closed, void *user_data)
{
    (void)user_data;
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, contact_closed]() {
        ESP_LOGI(TAG, "Contact ep=%u closed=%d", endpoint_id, contact_closed);
        auto booleanState = BooleanState::FindClusterOnEndpoint(endpoint_id);
        VerifyOrReturn(booleanState != nullptr);
        booleanState->SetStateValue(contact_closed);
    });
}

void door_sensor_matter_notify_panic(bool panic_active)
{
    if (!s_matter_started || panic_indicator_endpoint_id == 0) {
        return;
    }

    /* Match intercom tamper: StateValue true = inactive/closed (HA off). */
    const bool contact_closed = !panic_active;
    chip::DeviceLayer::SystemLayer().ScheduleLambda([contact_closed]() {
        ESP_LOGI(TAG, "Panic indicator ep=%u active=%d", panic_indicator_endpoint_id, !contact_closed);
        auto booleanState = BooleanState::FindClusterOnEndpoint(panic_indicator_endpoint_id);
        VerifyOrReturn(booleanState != nullptr);
        booleanState->SetStateValue(contact_closed);
    });
}

void door_sensor_matter_notify_siren(bool on)
{
    if (!s_matter_started || siren_endpoint_id == 0) {
        return;
    }

    chip::DeviceLayer::SystemLayer().ScheduleLambda([on]() {
        attribute_t *attr = attribute::get(siren_endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id);
        if (attr == nullptr) {
            return;
        }
        esp_matter_attr_val_t val = esp_matter_invalid(NULL);
        attribute::get_val(attr, &val);
        if (val.val.b == on) {
            return;
        }
        val.val.b = on;
        attribute::update(siren_endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id, &val);
    });
}
