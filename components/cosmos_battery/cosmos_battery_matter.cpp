#include <esp_log.h>
#include <esp_matter.h>
#include <lib/support/CodeUtils.h>
#include <platform/CHIPDeviceLayer.h>

#include <app-common/zap-generated/ids/Clusters.h>

#include <cosmos_battery.h>
#include <cosmos_battery_matter.h>

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace chip::app::Clusters;

static const char *TAG = "cosmos_battery_matter";

uint16_t cosmos_battery_matter_add_endpoint(esp_matter::node_t *node)
{
    esp_matter::endpoint::power_source::config_t config;
    config.power_source.feature_flags = esp_matter::cluster::power_source::feature::battery::get_id() |
                                          esp_matter::cluster::power_source::feature::replaceable::get_id();

    endpoint_t *endpoint = esp_matter::endpoint::power_source::create(node, &config, ENDPOINT_FLAG_NONE, NULL);
    if (!endpoint) {
        ESP_LOGE(TAG, "Failed to create Power Source endpoint");
        return 0;
    }

    cluster_t *power_cluster = esp_matter::cluster::get(endpoint, PowerSource::Id);
    if (!power_cluster) {
        ESP_LOGE(TAG, "Power Source cluster missing on endpoint");
        return 0;
    }

    esp_matter::cluster::power_source::attribute::create_bat_voltage(power_cluster, nullable<uint32_t>(),
                                                                     nullable<uint32_t>(0), nullable<uint32_t>(0xFFFF));
    esp_matter::cluster::power_source::attribute::create_bat_percent_remaining(power_cluster, nullable<uint8_t>(),
                                                                                nullable<uint8_t>(0), nullable<uint8_t>(200));
    esp_matter::cluster::power_source::attribute::create_bat_present(power_cluster, true);

    const uint16_t endpoint_id = esp_matter::endpoint::get_id(endpoint);
    ESP_LOGI(TAG, "Power Source endpoint created with ID: %u", endpoint_id);
    return endpoint_id;
}

void cosmos_battery_matter_update(uint16_t endpoint_id, uint32_t voltage_mv, uint8_t percent_matter)
{
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, voltage_mv, percent_matter]() {
        esp_matter_attr_val_t voltage_val = esp_matter_nullable_uint32(voltage_mv);
        esp_err_t err =
            attribute::update(endpoint_id, PowerSource::Id, PowerSource::Attributes::BatVoltage::Id, &voltage_val);
        if (err != ESP_OK && err != ESP_ERR_NOT_FINISHED) {
            ESP_LOGW(TAG, "BatVoltage update failed: %d", err);
        }

        esp_matter_attr_val_t percent_val = (percent_matter == COSMOS_BATTERY_PERCENT_UNKNOWN)
                                                ? esp_matter_nullable_uint8(nullable<uint8_t>())
                                                : esp_matter_nullable_uint8(percent_matter);
        err = attribute::update(endpoint_id, PowerSource::Id, PowerSource::Attributes::BatPercentRemaining::Id,
                                &percent_val);
        if (err != ESP_OK && err != ESP_ERR_NOT_FINISHED) {
            ESP_LOGW(TAG, "BatPercentRemaining update failed: %d", err);
        }
    });
}
