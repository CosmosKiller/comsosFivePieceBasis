/*
 * Cosmos SKU5 — C6 Matter security endpoints + Hosted EVT handler.
 * One Matter node (C6): camera (elsewhere) + doorbell / PIR / tamper / siren.
 */

#include "security_endpoints.h"

#include <cstring>

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/boolean-state-server/CodegenIntegration.h>
#include <app/clusters/switch-server/CodegenIntegration.h>
#include <bridge_cmd_defs.h>
#include <clusters/occupancy_sensing/integration.h>
#include <esp_log.h>
#include <esp_matter.h>
#include <esp_matter_attribute_utils.h>
#include <esp_matter_providers.h>
#include <lib/support/CodeUtils.h>
#include <platform/ESP32/StaticESP32DeviceInfoProvider.h>
#include <webrtc_bridge.h>

static const char *TAG = "security_ep";

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::cluster;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;
using chip::CharSpan;
using chip::Span;
using chip::DeviceLayer::StaticESP32DeviceInfoProvider;

static uint16_t s_doorbell_ep = 0;
static uint16_t s_occupancy_ep = 0;
static uint16_t s_tamper_ep = 0;
static uint16_t s_siren_ep = 0;
static bool s_matter_started = false;

#if CONFIG_SUPPORT_FIXED_LABEL_CLUSTER && CONFIG_CUSTOM_DEVICE_INFO_PROVIDER
constexpr char kDoorbellDisplayName[] = "Doorbell";
constexpr char kPirDisplayName[] = "PIR";
constexpr char kTamperDisplayName[] = "Tamper";
constexpr char kSirenDisplayName[] = "Siren";
constexpr char kFixedLabelNameKey[] = "name";
constexpr char kFixedLabelRoleKey[] = "role";
constexpr char kRoleDoorbell[] = "doorbell";
constexpr char kRolePir[] = "pir-occupancy";
constexpr char kRoleTamper[] = "tamper";
constexpr char kRoleSiren[] = "siren";

static StaticESP32DeviceInfoProvider s_device_info_provider;
static StaticESP32DeviceInfoProvider::FixedLabelEntry s_fixed_labels[8];
static size_t s_fixed_label_count = 0;

static void add_label(uint16_t endpoint_id, const char *label, const char *value)
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

static void register_fixed_labels(void)
{
    s_fixed_label_count = 0;
    add_label(s_doorbell_ep, kFixedLabelNameKey, kDoorbellDisplayName);
    add_label(s_doorbell_ep, kFixedLabelRoleKey, kRoleDoorbell);
    add_label(s_occupancy_ep, kFixedLabelNameKey, kPirDisplayName);
    add_label(s_occupancy_ep, kFixedLabelRoleKey, kRolePir);
    add_label(s_tamper_ep, kFixedLabelNameKey, kTamperDisplayName);
    add_label(s_tamper_ep, kFixedLabelRoleKey, kRoleTamper);
    add_label(s_siren_ep, kFixedLabelNameKey, kSirenDisplayName);
    add_label(s_siren_ep, kFixedLabelRoleKey, kRoleSiren);

    CHIP_ERROR err = s_device_info_provider.SetFixedLabels(
        Span<StaticESP32DeviceInfoProvider::FixedLabelEntry>(s_fixed_labels, s_fixed_label_count));
    if (err != CHIP_NO_ERROR) {
        ESP_LOGW(TAG, "SetFixedLabels failed: %" CHIP_ERROR_FORMAT, err.Format());
        return;
    }
    esp_matter::set_custom_device_info_provider(&s_device_info_provider);
}
#endif

static void enrich_endpoint(endpoint_t *ep)
{
    if (ep == nullptr) {
        return;
    }
#if CONFIG_SUPPORT_FIXED_LABEL_CLUSTER
    cluster::fixed_label::config_t fixed_label_cfg;
    cluster::fixed_label::create(ep, &fixed_label_cfg, CLUSTER_FLAG_SERVER);
#endif
}

static void occupancy_apply(uint16_t endpoint_id, bool occupancy)
{
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, occupancy]() {
        auto *cluster = OccupancySensing::FindClusterOnEndpoint(endpoint_id);
        if (cluster == nullptr) {
            ESP_LOGE(TAG, "Occupancy cluster missing on ep=%u", endpoint_id);
            return;
        }
        cluster->SetOccupancy(occupancy);
        ESP_LOGI(TAG, "Occupancy ep=%u occupied=%d", endpoint_id, occupancy);
    });
}

static void doorbell_apply(uint16_t endpoint_id, bool pressed)
{
    /* Switch is code-driven: attribute::update(CurrentPosition) → ESP_ERR_NOT_SUPPORTED.
     * Use SwitchCluster::SetCurrentPosition + event helpers (CHIP Switch README). */
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, pressed]() {
        constexpr uint8_t kPressPosition = 1;
        constexpr uint8_t kIdlePosition = 0;

        auto *sw = Switch::FindClusterOnEndpoint(endpoint_id);
        if (sw == nullptr) {
            ESP_LOGE(TAG, "Switch cluster missing on ep=%u", endpoint_id);
            return;
        }
        CHIP_ERROR cerr = sw->SetCurrentPosition(pressed ? kPressPosition : kIdlePosition);
        if (cerr != CHIP_NO_ERROR) {
            ESP_LOGE(TAG, "SetCurrentPosition failed: %" CHIP_ERROR_FORMAT, cerr.Format());
            return;
        }
        if (pressed) {
            ESP_LOGI(TAG, "Doorbell InitialPress (ep=%u)", endpoint_id);
            switch_cluster::event::send_initial_press(endpoint_id, kPressPosition);
        } else {
            ESP_LOGI(TAG, "Doorbell ShortRelease (ep=%u)", endpoint_id);
            switch_cluster::event::send_short_release(endpoint_id, kPressPosition);
        }
    });
}

static void tamper_apply(uint16_t endpoint_id, bool tampered)
{
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, tampered]() {
        /* Matter StateValue true = contact closed (seated). HA binary_sensor on = open. */
        const bool contact_closed = !tampered;
        ESP_LOGI(TAG, "Tamper ep=%u open=%d StateValue(closed)=%d", endpoint_id, tampered, contact_closed);

        esp_matter_attr_val_t val = esp_matter_bool(contact_closed);
        esp_err_t err =
            attribute::update(endpoint_id, BooleanState::Id, BooleanState::Attributes::StateValue::Id, &val);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Tamper StateValue update failed: %s", esp_err_to_name(err));
        }

        auto booleanState = BooleanState::FindClusterOnEndpoint(endpoint_id);
        if (booleanState != nullptr) {
            booleanState->SetStateValue(contact_closed);
        } else {
            ESP_LOGW(TAG, "BooleanState cluster missing on ep=%u (attr update still attempted)", endpoint_id);
        }

        if (tampered && s_siren_ep != 0) {
            attribute_t *attr = attribute::get(s_siren_ep, OnOff::Id, OnOff::Attributes::OnOff::Id);
            if (attr) {
                esp_matter_attr_val_t onoff = esp_matter_invalid(NULL);
                attribute::get_val(attr, &onoff);
                onoff.val.b = true;
                attribute::update(s_siren_ep, OnOff::Id, OnOff::Attributes::OnOff::Id, &onoff);
            }
        }
    });
}

static void security_io_evt_cb(uint32_t cmd_id, esp_err_t status, uint8_t *data, size_t len)
{
    (void)cmd_id;
    if (status != ESP_OK || data == nullptr || len < sizeof(bridge_evt_security_io_t)) {
        free(data);
        return;
    }
    if (!s_matter_started) {
        free(data);
        return;
    }

    bridge_evt_security_io_t evt;
    memcpy(&evt, data, sizeof(evt));
    free(data);

    ESP_LOGI(TAG, "SEC_IO doorbell=%u pir=%u tamper=%u flags=0x%02x", evt.doorbell, evt.pir, evt.tamper,
             evt.flags);

    if (evt.flags & BRIDGE_EVT_SEC_FLAG_DOORBELL) {
        doorbell_apply(s_doorbell_ep, evt.doorbell != 0);
    }
    if (evt.flags & BRIDGE_EVT_SEC_FLAG_PIR) {
        occupancy_apply(s_occupancy_ep, evt.pir != 0);
    }
    if (evt.flags & BRIDGE_EVT_SEC_FLAG_TAMPER) {
        tamper_apply(s_tamper_ep, evt.tamper != 0);
    }
}

esp_err_t security_endpoints_create(node_t *node)
{
    if (node == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    occupancy_sensor::config_t occupancy_cfg;
    occupancy_cfg.occupancy_sensing.occupancy_sensor_type =
        chip::to_underlying(OccupancySensing::OccupancySensorTypeEnum::kPir);
    occupancy_cfg.occupancy_sensing.occupancy_sensor_type_bitmap =
        chip::to_underlying(OccupancySensing::OccupancySensorTypeBitmap::kPir);
    occupancy_cfg.occupancy_sensing.feature_flags =
        chip::to_underlying(OccupancySensing::Feature::kPassiveInfrared);
    endpoint_t *occupancy_ep = occupancy_sensor::create(node, &occupancy_cfg, ENDPOINT_FLAG_NONE, NULL);
    if (!occupancy_ep) {
        ESP_LOGE(TAG, "Failed to create occupancy endpoint");
        return ESP_FAIL;
    }
    enrich_endpoint(occupancy_ep);
    s_occupancy_ep = endpoint::get_id(occupancy_ep);
    ESP_LOGI(TAG, "Occupancy (PIR) endpoint ID: %d", s_occupancy_ep);

    generic_switch::config_t doorbell_cfg;
    doorbell_cfg.switch_cluster.feature_flags =
        cluster::switch_cluster::feature::momentary_switch::get_id() |
        cluster::switch_cluster::feature::momentary_switch_release::get_id();
    endpoint_t *doorbell_ep = generic_switch::create(node, &doorbell_cfg, ENDPOINT_FLAG_NONE, NULL);
    if (!doorbell_ep) {
        ESP_LOGE(TAG, "Failed to create doorbell endpoint");
        return ESP_FAIL;
    }
    enrich_endpoint(doorbell_ep);
    s_doorbell_ep = endpoint::get_id(doorbell_ep);
    ESP_LOGI(TAG, "Doorbell endpoint ID: %d", s_doorbell_ep);

    contact_sensor::config_t tamper_cfg;
    endpoint_t *tamper_ep = contact_sensor::create(node, &tamper_cfg, ENDPOINT_FLAG_NONE, NULL);
    if (!tamper_ep) {
        ESP_LOGE(TAG, "Failed to create tamper endpoint");
        return ESP_FAIL;
    }
    enrich_endpoint(tamper_ep);
    s_tamper_ep = endpoint::get_id(tamper_ep);
    ESP_LOGI(TAG, "Tamper endpoint ID: %d", s_tamper_ep);

    mounted_on_off_control::config_t siren_cfg;
    siren_cfg.on_off.on_off = false;
    endpoint_t *siren_ep = mounted_on_off_control::create(node, &siren_cfg, ENDPOINT_FLAG_NONE, NULL);
    if (!siren_ep) {
        ESP_LOGE(TAG, "Failed to create siren endpoint");
        return ESP_FAIL;
    }
    enrich_endpoint(siren_ep);
    s_siren_ep = endpoint::get_id(siren_ep);
    ESP_LOGI(TAG, "Siren endpoint ID: %d", s_siren_ep);

#if CONFIG_SUPPORT_FIXED_LABEL_CLUSTER && CONFIG_CUSTOM_DEVICE_INFO_PROVIDER
    register_fixed_labels();
#endif

    return ESP_OK;
}

esp_err_t security_endpoints_register_bridge(void)
{
    return bridge_cmd_register_event_handler(BRIDGE_EVT_SECURITY_IO, security_io_evt_cb);
}

void security_endpoints_mark_matter_started(void)
{
    s_matter_started = true;
}

esp_err_t security_endpoints_drive_siren(bool on)
{
    bridge_cmd_set_siren_t req = {.on = static_cast<uint8_t>(on ? 1 : 0)};
    return bridge_cmd_send(BRIDGE_CMD_SET_SIREN, (const uint8_t *)&req, sizeof(req));
}

uint16_t security_endpoints_siren_id(void)
{
    return s_siren_ep;
}
