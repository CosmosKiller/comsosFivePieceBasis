/**
 * @file main.cpp
 * @brief Matter door intercom: doorbell, PIR, tamper, siren OnOff, MJPEG, OTA.
 */

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <driver/gpio.h>

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/boolean-state-server/CodegenIntegration.h>
#include <esp_matter.h>
#include <lib/support/CodeUtils.h>

#include <cam_task.h>
#include <cosmos_battery.h>
#include <cosmos_battery_matter.h>
#include <cosmos_matter_ota.h>
#include <door_intercom_task.h>
#include <evt_service_task.h>
#include <factory_reset_task.h>
#include <http_stream_task.h>
#include <matter_task.h>
#include <security_module_task.h>

static const char *TAG = "app_main";

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::cluster;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

uint16_t intercom_endpoint_id = 0;
uint16_t doorbell_endpoint_id = 0;
uint16_t tamper_endpoint_id = 0;
uint16_t siren_endpoint_id = 0;
uint16_t doorlock_endpoint_id = 0;
httpd_handle_t cam_server;

static void occupancy_sensor_notification(uint16_t endpoint_id, bool occupancy, void *user_data);
static void doorbell_notification(uint16_t endpoint_id, bool pressed, void *user_data);
static void tamper_notification(uint16_t endpoint_id, bool tampered, void *user_data);

extern "C" void app_main(void)
{
    esp_err_t err = ESP_OK;

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

    err = gpio_install_isr_service(0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO ISR service install failed: %d", err);
        return;
    }

    err = cam_task_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "cam_task_init failed: %d", err);
        return;
    }

    err = evt_service_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "evt_service_init failed: %d", err);
        return;
    }

    node::config_t node_cfg;
    node_t *node = node::create(&node_cfg, app_attribute_update_cb, app_identification_cb);
    if (!node) {
        ESP_LOGE(TAG, "Failed to create Matter node");
        return;
    }

    on_off_plug_in_unit::config_t intercom_config;
    intercom_config.on_off.on_off = false;

    endpoint_t *intercom_ep = on_off_plug_in_unit::create(node, &intercom_config, ENDPOINT_FLAG_NONE, NULL);
    if (!intercom_ep) {
        ESP_LOGE(TAG, "Failed to create intercom endpoint");
        return;
    }
    intercom_endpoint_id = endpoint::get_id(intercom_ep);
    ESP_LOGI(TAG, "Intercom OnOff (stream gate) endpoint ID: %d", intercom_endpoint_id);

    occupancy_sensor::config_t occupancy_sensor_config;
    occupancy_sensor_config.occupancy_sensing.occupancy_sensor_type =
        chip::to_underlying(OccupancySensing::OccupancySensorTypeEnum::kPir);
    occupancy_sensor_config.occupancy_sensing.occupancy_sensor_type_bitmap =
        chip::to_underlying(OccupancySensing::OccupancySensorTypeBitmap::kPir);
    occupancy_sensor_config.occupancy_sensing.feature_flags =
        chip::to_underlying(OccupancySensing::Feature::kPassiveInfrared);

    endpoint_t *occupancy_sensor_ep = occupancy_sensor::create(node, &occupancy_sensor_config, ENDPOINT_FLAG_NONE, NULL);
    if (!occupancy_sensor_ep) {
        ESP_LOGE(TAG, "Failed to create occupancy sensor endpoint");
        return;
    }
    ESP_LOGI(TAG, "Occupancy (PIR) endpoint ID: %d", endpoint::get_id(occupancy_sensor_ep));

    /* HA-first: Generic Switch (0x000F) so python-matter-server creates event.*.
     * True Matter Doorbell (0x0148) after esp-matter + HA support bump. */
    generic_switch::config_t doorbell_config;
    doorbell_config.switch_cluster.feature_flags =
        cluster::switch_cluster::feature::momentary_switch::get_id() |
        cluster::switch_cluster::feature::momentary_switch_release::get_id();

    endpoint_t *doorbell_ep = generic_switch::create(node, &doorbell_config, ENDPOINT_FLAG_NONE, NULL);
    if (!doorbell_ep) {
        ESP_LOGE(TAG, "Failed to create doorbell generic_switch endpoint");
        return;
    }
    doorbell_endpoint_id = endpoint::get_id(doorbell_ep);
    ESP_LOGI(TAG, "Doorbell generic_switch endpoint ID: %d", doorbell_endpoint_id);

    /* Tamper / mount break — Contact Sensor (Boolean State): open=tampered, closed=seated. */
    contact_sensor::config_t tamper_config;
    endpoint_t *tamper_ep = contact_sensor::create(node, &tamper_config, ENDPOINT_FLAG_NONE, NULL);
    if (!tamper_ep) {
        ESP_LOGE(TAG, "Failed to create tamper contact_sensor endpoint");
        return;
    }
    tamper_endpoint_id = endpoint::get_id(tamper_ep);
    ESP_LOGI(TAG, "Tamper contact endpoint ID: %d", tamper_endpoint_id);

    /* Siren / alarm clear — HA turns Off to silence latched tamper siren (GPIO4). */
    mounted_on_off_control::config_t siren_config;
    siren_config.on_off.on_off = false;
    endpoint_t *siren_ep = mounted_on_off_control::create(node, &siren_config, ENDPOINT_FLAG_NONE, NULL);
    if (!siren_ep) {
        ESP_LOGE(TAG, "Failed to create siren/alarm OnOff endpoint");
        return;
    }
    siren_endpoint_id = endpoint::get_id(siren_ep);
    ESP_LOGI(TAG, "Siren alarm OnOff endpoint ID: %d", siren_endpoint_id);

    static security_module_config_t sec_mod_config = {
        .pir_sensor =
            {
                .cb = occupancy_sensor_notification,
                .endpoint_id = endpoint::get_id(occupancy_sensor_ep),
            },
        .tamper =
            {
                .cb = tamper_notification,
                .endpoint_id = tamper_endpoint_id,
            },
        .user_data = NULL,
    };

    err = security_module_task_init(&sec_mod_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "security_module_task_init failed: %d", err);
        return;
    }

    static door_intercom_config_t door_intercom_config = {
        .doorbell =
            {
                .cb = doorbell_notification,
                .endpoint_id = doorbell_endpoint_id,
            },
        .user_data = NULL,
    };

    err = door_intercom_task_init(&door_intercom_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "door_intercom_task_init failed: %d", err);
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

    err = esp_matter::start(app_event_cb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_matter::start failed: %d", err);
        return;
    }

    /* Matter stack is up — publish initial tamper Boolean State. */
    tamper_notification(tamper_endpoint_id, security_module_tamper_is_open(), NULL);

    err = cosmos_matter_ota_configure();
    if (err != ESP_OK && err != ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGE(TAG, "cosmos_matter_ota_configure failed: %d", err);
        return;
    }

    cam_server = http_server_task_start(NULL);
    if (!cam_server) {
        ESP_LOGE(TAG, "Failed to start MJPEG stream server");
        return;
    }

    err = cosmos_battery_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "cosmos_battery_start failed: %d", err);
        return;
    }

    factory_reset_task();
}

static void occupancy_sensor_notification(uint16_t endpoint_id, bool occupancy, void *user_data)
{
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, occupancy]() {
        attribute_t *attribute =
            attribute::get(endpoint_id, OccupancySensing::Id, OccupancySensing::Attributes::Occupancy::Id);

        esp_matter_attr_val_t val = esp_matter_invalid(NULL);
        attribute::get_val(attribute, &val);
        val.val.b = occupancy;

        attribute::update(endpoint_id, OccupancySensing::Id, OccupancySensing::Attributes::Occupancy::Id, &val);
    });
}

static void doorbell_notification(uint16_t endpoint_id, bool pressed, void *user_data)
{
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, pressed]() {
        constexpr uint8_t kPressPosition = 1;
        constexpr uint8_t kIdlePosition = 0;

        if (pressed) {
            ESP_LOGI(TAG, "Doorbell InitialPress (ep=%u)", endpoint_id);
            chip::app::Clusters::Switch::Attributes::CurrentPosition::Set(endpoint_id, kPressPosition);
            switch_cluster::event::send_initial_press(endpoint_id, kPressPosition);
        } else {
            ESP_LOGI(TAG, "Doorbell ShortRelease (ep=%u)", endpoint_id);
            chip::app::Clusters::Switch::Attributes::CurrentPosition::Set(endpoint_id, kIdlePosition);
            switch_cluster::event::send_short_release(endpoint_id, kPressPosition);
        }
    });
}

static void tamper_notification(uint16_t endpoint_id, bool tampered, void *user_data)
{
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, tampered]() {
        ESP_LOGW(TAG, "Tamper state: endpoint_id=%d, tampered=%d", endpoint_id, tampered);
        auto booleanState = BooleanState::FindClusterOnEndpoint(endpoint_id);
        VerifyOrReturn(booleanState != nullptr);
        booleanState->SetStateValue(tampered);

        /* Latch: only force alarm OnOff ON when tampered. Remount leaves OnOff/siren alone. */
        if (tampered && siren_endpoint_id != 0) {
            attribute_t *attr = attribute::get(siren_endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id);
            if (attr) {
                esp_matter_attr_val_t val = esp_matter_invalid(NULL);
                attribute::get_val(attr, &val);
                val.val.b = true;
                attribute::update(siren_endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id, &val);
            }
        }
    });
}
