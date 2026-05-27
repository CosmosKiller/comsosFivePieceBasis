/**
 * @file bme680_task.cpp
 * @author Marcel Nahir Samur (mnsamur2014@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-06-06
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <esp_err.h>
#include <esp_log.h>
#include <esp_random.h>
#include <esp_timer.h>

#include <lib/support/CodeUtils.h>

#include <bme680_task.h>

static const char *TAG = "bme680_task";

/**
 * @brief Context structure for the bme680 sensor
 *        Holds all the state and configuration needed for the driver.
 */
typedef struct {
    bme680_sensor_config_t *config;
    esp_timer_handle_t timer;
    bool is_initialized = false;
} bme680_sensor_ctx_t;

static bme680_sensor_ctx_t s_ctx;

static bme680_t sensor;
static bme680_values_float_t values;

static uint32_t duration_ms;
static bool waiting_for_result = false;
// static uint32_t period_ms = 5000;
static uint32_t elapsed_ms = 0;

/**
 * @brief Task to setup the BME680 sensor
 *
 * @param pvParameter Parameter which can be passed to the task.
 */
static void bme680_task_setup(void *pvParameter)
{
    esp_err_t err;

    memset(&sensor, 0, sizeof(bme680_t));

    err = bme680_init_desc(&sensor, I2C_ADDR, I2C_BUS, I2C_SDA_PIN, I2C_SCL_PIN);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bme680_init_desc failed: %d", err);
        return;
    }

    // Init the sensor
    err = bme680_init_sensor(&sensor);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bme680_init_sensor failed: %d", err);
        return;
    }

    // Changes the oversampling rates to 4x oversampling for temperature
    // and 2x oversampling for humidity. Pressure measurement is skipped.
    bme680_set_oversampling_rates(&sensor, BME680_OSR_8X, BME680_OSR_4X, BME680_OSR_2X);

    // Change the IIR filter size for temperature and pressure to 7.
    bme680_set_filter_size(&sensor, BME680_IIR_SIZE_7);

    // Change the heater profile 0 to 200 degree Celsius for 100 ms.
    bme680_set_heater_profile(&sensor, 0, 200, 100);
    bme680_use_heater_profile(&sensor, 0);

    // Set ambient temperature to 10 degree Celsius
    bme680_set_ambient_temperature(&sensor, 10);

    // As long as sensor configuration isn't changed, duration is constant
    bme680_get_measurement_duration(&sensor, &duration_ms);
}

/**
 * @brief Callback to read the measurement results
 *        It calls the user callbacks if they are provided in the configuration
 * @param pArg
 * @return * void
 */
static void bme680_task_read_cb(void *pArg)
{
    elapsed_ms += s_ctx.config->interval_ms;

    auto *ctx = (bme680_sensor_ctx_t *)pArg;
    if (!(ctx && ctx->config)) {
        return;
    }

    if (!waiting_for_result) {
        // Start a new measurement
        if (bme680_force_measurement(&sensor) == ESP_OK) {
            waiting_for_result = true;
            elapsed_ms = 0; // reset wait counter
        }
    } else {
        // Wait until duration has passed
        if (bme680_get_results_float(&sensor, &values) == ESP_OK) {
            ESP_LOGI(TAG, "BME680 Sensor: %.2f °C, %.2f %%, %.2f hPa, %.2f Ohm\n",
                     values.temperature, values.humidity,
                     values.pressure, values.gas_resistance);

            if (ctx->config->temperature.cb) {
                ctx->config->temperature.cb(ctx->config->temperature.endpoint_id, values.temperature, ctx->config->user_data);
            }
            if (ctx->config->humidity.cb) {
                ctx->config->humidity.cb(ctx->config->humidity.endpoint_id, values.humidity, ctx->config->user_data);
            }
            if (ctx->config->pressure.cb) {
                ctx->config->pressure.cb(ctx->config->pressure.endpoint_id, values.pressure, ctx->config->user_data);
            }
            if (ctx->config->gas_resistance.cb) {
                ctx->config->gas_resistance.cb(ctx->config->gas_resistance.endpoint_id, values.gas_resistance, ctx->config->user_data);
            }
        }
        waiting_for_result = false; // ready for next cycle
    }
}

esp_err_t bme680_task_sensor_init(bme680_sensor_config_t *pConfig)
{
    ESP_ERROR_CHECK(i2cdev_init());
    // First we call the setup task to initialize the sensor
    bme680_task_setup(NULL);

    esp_err_t err;

    if (pConfig == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    // We need at least one callback so that we can start notifying application layer
    if (pConfig->temperature.cb == NULL && pConfig->humidity.cb == NULL && pConfig->pressure.cb == NULL && pConfig->gas_resistance.cb == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_ctx.is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // Keep the pointer to config
    s_ctx.config = pConfig;

    // Create one-shot timer to read the results after measurement is done
    esp_timer_create_args_t read_args = {
        .callback = bme680_task_read_cb,
        .arg = &s_ctx,
        .name = "bme680_read",
    };

    err = esp_timer_create(&read_args, &s_ctx.timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Read: esp_timer_create failed, err:%d", err);
        return err;
    }

    err = esp_timer_start_periodic(s_ctx.timer, pConfig->interval_ms * 1000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_timer_start_periodic failed: %d", err);
        return err;
    }

    s_ctx.is_initialized = true;
    ESP_LOGI(TAG, "bme680 initialized successfully");

    return ESP_OK;
}