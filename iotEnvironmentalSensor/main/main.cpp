/**
 * @file main.c
 * @author Marcel Nahir Samur (mnsamur2014@gmail.com)
 * @brief Matter environmental app: temp/humidity/pressure endpoints, BME680 driver, OTA, and stack start.
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
#include <bme680_task.h>
#include <factory_reset_task.h>
#include <matter_task.h>

static const char *TAG = "app_main";

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

// Function declarations
static void temperature_sensor_notification(uint16_t endpoint_id, float temp, void *user_data);
static void humidity_sensor_notification(uint16_t endpoint_id, float humidity, void *user_data);
static void pressure_sensor_notification(uint16_t endpoint_id, float pressure, void *user_data);

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

    // Add temperature sensor device
    temperature_sensor::config_t temp_sensor_config;
    endpoint_t *temp_sensor_ep = temperature_sensor::create(node, &temp_sensor_config, ENDPOINT_FLAG_NONE, NULL);
    // Confirm that temperature_sensor endpoint was created successfully
    if (!temp_sensor_ep) {
        ESP_LOGE(TAG, "Failed to create temperature_sensor endpoint");
        return;
    }

    // Add the humidity sensor device
    humidity_sensor::config_t humidity_sensor_config;
    endpoint_t *humidity_sensor_ep = humidity_sensor::create(node, &humidity_sensor_config, ENDPOINT_FLAG_NONE, NULL);
    // Confirm that humidity_sensor endpoint was created successfully
    if (!humidity_sensor_ep) {
        ESP_LOGE(TAG, "Failed to create humidity_sensor endpoint");
        return;
    }

    // Add the pressure sensor device
    pressure_sensor::config_t pressure_sensor_config;
    endpoint_t *pressure_sensor_ep = pressure_sensor::create(node, &pressure_sensor_config, ENDPOINT_FLAG_NONE, NULL);
    // Confirm that pressure_sensor endpoint was created successfully
    if (!pressure_sensor_ep) {
        ESP_LOGE(TAG, "Failed to create pressure_sensor endpoint");
        return;
    }

    // Set BME680 config
    static bme680_sensor_config_t bme680_sensor_config = {
        .temperature =
            {
                .cb = temperature_sensor_notification,
                .endpoint_id = endpoint::get_id(temp_sensor_ep),
            },
        .humidity =
            {
                .cb = humidity_sensor_notification,
                .endpoint_id = endpoint::get_id(humidity_sensor_ep),
            },
        .pressure =
            {
                .cb = pressure_sensor_notification,
                .endpoint_id = endpoint::get_id(pressure_sensor_ep),
            },
    };

    // Initialize BME680 sensor task
    err = bme680_task_sensor_init(&bme680_sensor_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bme680_task_sensor_init failed: %d", err);
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

    err = cosmos_battery_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "cosmos_battery_start failed: %d", err);
        return;
    }

    // Start factory reset task
    factory_reset_task();
}

/*
 * Application cluster specification, 2.3.4.1. Temperature
 * represents a temperature on the Celsius scale with a resolution of 0.01°C.
 * temp = (temperature in °C) x 100
 */
static void temperature_sensor_notification(uint16_t endpoint_id, float temp, void *user_data)
{
    // schedule the attribute update so that we can report it from matter thread
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, temp]() {
        attribute_t *attribute = attribute::get(endpoint_id,
                                                TemperatureMeasurement::Id,
                                                TemperatureMeasurement::Attributes::MeasuredValue::Id);

        esp_matter_attr_val_t val = esp_matter_invalid(NULL);
        attribute::get_val(attribute, &val);
        val.val.i16 = static_cast<int16_t>(temp * 100);

        attribute::update(endpoint_id, TemperatureMeasurement::Id, TemperatureMeasurement::Attributes::MeasuredValue::Id, &val);
    });
}

/*
 * Application cluster specification, 2.6.4.1. Relative Humidity
 * represents the humidity in percent.
 * humidity = (humidity in %) x 100
 *
 */
static void humidity_sensor_notification(uint16_t endpoint_id, float humidity, void *user_data)
{
    // schedule the attribute update so that we can report it from matter thread
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, humidity]() {
        attribute_t *attribute = attribute::get(endpoint_id,
                                                RelativeHumidityMeasurement::Id,
                                                RelativeHumidityMeasurement::Attributes::MeasuredValue::Id);

        esp_matter_attr_val_t val = esp_matter_invalid(NULL);
        attribute::get_val(attribute, &val);
        val.val.u16 = static_cast<uint16_t>(humidity * 100);

        attribute::update(endpoint_id, RelativeHumidityMeasurement::Id, RelativeHumidityMeasurement::Attributes::MeasuredValue::Id, &val);
    });
}

/*
 * Application cluster specification, 2.4.5.1. Pressure
 * represents the pressure in Kilopascals (kPa).
 * pressure = (pressure in kPa) x 10
 *
 */
static void pressure_sensor_notification(uint16_t endpoint_id, float pressure, void *user_data)
{
    // schedule the attribute update so that we can report it from matter thread
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, pressure]() {
        attribute_t *attribute = attribute::get(endpoint_id,
                                                PressureMeasurement::Id,
                                                PressureMeasurement::Attributes::MeasuredValue::Id);

        esp_matter_attr_val_t val = esp_matter_invalid(NULL);
        attribute::get_val(attribute, &val);
        val.val.u32 = static_cast<uint32_t>(pressure);

        attribute::update(endpoint_id, PressureMeasurement::Id, PressureMeasurement::Attributes::MeasuredValue::Id, &val);
    });
}

/* static void gas_sensor_notification(uint16_t endpoint_id, float gas_resistance, void *user_data)
{
    // schedule the attribute update so that we can report it from matter thread
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, gas_resistance]() {
        attribute_t *attribute = attribute::get(endpoint_id,
                                                TotalVolatileOrganicCompoundsConcentrationMeasurement::Id,
                                                TotalVolatileOrganicCompoundsConcentrationMeasurement::Attributes::MeasuredValue::Id);

        esp_matter_attr_val_t val = esp_matter_invalid(NULL);
        attribute::get_val(attribute, &val);
        val.val.u32 = gas_resistance;

        attribute::update(endpoint_id, 0x042E, 0x0000, &val);
    });
} */