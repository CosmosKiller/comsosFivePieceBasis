#include <esp_err.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <inttypes.h>
#include <sdkconfig.h>

#include <cosmos_battery.h>
#include <cosmos_battery_matter.h>

#include "cosmos_battery_adc.h"

#include <algorithm>
#include <cmath>

static const char *TAG = "cosmos_battery";

typedef struct {
    cosmos_battery_config_t config;
    esp_timer_handle_t timer;
    bool initialized;
    bool started;
} cosmos_battery_ctx_t;

static cosmos_battery_ctx_t s_ctx;

static uint8_t voltage_to_matter_percent(float cell_v, float empty_v, float full_v)
{
    if (full_v <= empty_v) {
        return COSMOS_BATTERY_PERCENT_UNKNOWN;
    }

    const float ratio = (cell_v - empty_v) / (full_v - empty_v);
    const float clamped = std::max(0.0f, std::min(1.0f, ratio));
    const uint8_t percent = static_cast<uint8_t>(std::lround(clamped * 100.0f));
    if (percent == 0) {
        return 1;
    }
    return static_cast<uint8_t>(percent * 2);
}

static void cosmos_battery_sample_once(void)
{
    int adc_mv = 0;
    esp_err_t err = cosmos_battery_adc_read_mv(&adc_mv);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ADC read failed: %d", err);
        return;
    }

    const float cell_v = (adc_mv / 1000.0f) * s_ctx.config.divider_ratio;
    const uint32_t cell_mv = static_cast<uint32_t>(std::lround(cell_v * 1000.0f));
    const uint8_t matter_percent = voltage_to_matter_percent(cell_v, s_ctx.config.voltage_empty_v, s_ctx.config.voltage_full_v);

    ESP_LOGI(TAG, "Battery: %.2f V (%" PRIu32 " mV), Matter percent=%u", cell_v, cell_mv, matter_percent);

    if (s_ctx.config.endpoint_id != 0) {
        cosmos_battery_matter_update(s_ctx.config.endpoint_id, cell_mv, matter_percent);
    }
}

static void cosmos_battery_timer_cb(void *arg)
{
    (void)arg;
    cosmos_battery_sample_once();
}

void cosmos_battery_config_set_defaults(cosmos_battery_config_t *config)
{
    if (config == NULL) {
        return;
    }

    config->adc_gpio = CONFIG_COSMOS_BATTERY_ADC_GPIO;
    config->divider_ratio = 2.0f;
    config->voltage_full_v = 4.2f;
    config->voltage_empty_v = 3.0f;
    config->interval_ms = CONFIG_COSMOS_BATTERY_SAMPLE_INTERVAL_MS;
    config->endpoint_id = 0;
}

esp_err_t cosmos_battery_init(const cosmos_battery_config_t *config)
{
#if !CONFIG_COSMOS_BATTERY_ENABLE
    ESP_LOGI(TAG, "Battery monitor disabled in Kconfig");
    return ESP_OK;
#else
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = cosmos_battery_adc_open(config->adc_gpio);
    if (err != ESP_OK) {
        return err;
    }

    s_ctx.config = *config;
    s_ctx.initialized = true;
    return ESP_OK;
#endif
}

esp_err_t cosmos_battery_start(void)
{
#if !CONFIG_COSMOS_BATTERY_ENABLE
    return ESP_OK;
#else
    if (!s_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_ctx.started) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_timer_create_args_t timer_args = {
        .callback = cosmos_battery_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "cosmos_battery",
        .skip_unhandled_events = true,
    };

    esp_err_t err = esp_timer_create(&timer_args, &s_ctx.timer);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_timer_start_periodic(s_ctx.timer, s_ctx.config.interval_ms * 1000ULL);
    if (err != ESP_OK) {
        return err;
    }

    cosmos_battery_sample_once();
    s_ctx.started = true;
    ESP_LOGI(TAG, "Battery monitor started (interval %lu ms)", static_cast<unsigned long>(s_ctx.config.interval_ms));
    return ESP_OK;
#endif
}
