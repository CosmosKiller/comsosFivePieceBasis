#include <esp_log.h>
#include <esp_matter.h>
#include <lib/support/CodeUtils.h>
#include <platform/CHIPDeviceLayer.h>

#include <app-common/zap-generated/ids/Clusters.h>

#include <cosmos_battery_matter.h>

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::cluster;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

static const char *TAG = "cosmos_battery_matter";

uint16_t cosmos_battery_matter_add_endpoint(esp_matter::node_t *node)
{
    power_source::config_t config;
    config.power_source.feature_flags = power_source::feature::battery::get_id() | power_source::feature::replaceable::get_id();

    endpoint_t *endpoint = power_source::create(node, &config, ENDPOINT_FLAG_NONE, NULL);
    if (!endpoint) {
        ESP_LOGE(TAG, "Failed to create Power Source endpoint");
        return 0;
    }

    cluster_t *power_cluster = cluster::get(endpoint, PowerSource::Id);
    if (!power_cluster) {
        ESP_LOGE(TAG, "Power Source cluster missing on endpoint");
        return 0;
    }

    power_source::attribute::create_bat_voltage(power_cluster, nullable<uint32_t>());
    power_source::attribute::create_bat_percent_remaining(power_cluster, nullable<uint8_t>());
    power_source::attribute::create_bat_present(power_cluster, true);

    const uint16_t endpoint_id = endpoint::get_id(endpoint);
    ESP_LOGI(TAG, "Power Source endpoint created with ID: %u", endpoint_id);
    return endpoint_id;
}

void cosmos_battery_matter_update(uint16_t endpoint_id, uint32_t voltage_mv, uint8_t percent_matter)
{
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, voltage_mv, percent_matter]() {
        nullable<uint32_t> bat_voltage = voltage_mv;
        esp_matter_attr_val_t voltage_val = esp_matter_attr_val(bat_voltage);
        attribute::update(endpoint_id, PowerSource::Id, PowerSource::Attributes::BatVoltage::Id, &voltage_val);

        nullable<uint8_t> bat_percent = percent_matter;
        esp_matter_attr_val_t percent_val = esp_matter_attr_val(bat_percent);
        attribute::update(endpoint_id, PowerSource::Id, PowerSource::Attributes::BatPercentRemaining::Id, &percent_val);

        esp_matter_attr_val_t present_val = esp_matter_attr_val(true);
        attribute::update(endpoint_id, PowerSource::Id, PowerSource::Attributes::BatPresent::Id, &present_val);
    });
}
