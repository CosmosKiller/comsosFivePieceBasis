#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_err.h>
#include <esp_log.h>
#include <cosmos_battery.h>

static const char *TAG = "cosmos_battery_adc";

#define NO_OF_SAMPLES 64 /*!< ADC multisample count */

typedef struct {
    adc_oneshot_unit_handle_t unit;
    adc_cali_handle_t cali;
    bool cali_enabled;
    adc_channel_t channel;
    int adc_gpio;
} cosmos_battery_adc_ctx_t;

static cosmos_battery_adc_ctx_t s_adc;

static esp_err_t adc_gpio_to_channel(int gpio, adc_unit_t *unit, adc_channel_t *channel)
{
#if CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32C5
    if (gpio < 0 || gpio > 6) {
        return ESP_ERR_INVALID_ARG;
    }
    *unit = ADC_UNIT_1;
    *channel = static_cast<adc_channel_t>(gpio);
    return ESP_OK;
#else
    (void)gpio;
    (void)unit;
    (void)channel;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

static esp_err_t adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_cali_handle_t *out_handle, bool *out_enabled)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t err = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = channel,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    err = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
    if (err == ESP_OK) {
        calibrated = true;
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        err = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (err == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    *out_enabled = calibrated;
    return calibrated ? ESP_OK : err;
}

esp_err_t cosmos_battery_adc_open(int adc_gpio)
{
    adc_unit_t unit = ADC_UNIT_1;
    adc_channel_t channel = ADC_CHANNEL_0;

    esp_err_t err = adc_gpio_to_channel(adc_gpio, &unit, &channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Unsupported battery sense GPIO %d", adc_gpio);
        return err;
    }

    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = unit,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    err = adc_oneshot_new_unit(&init_cfg, &s_adc.unit);
    if (err != ESP_OK) {
        return err;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    err = adc_oneshot_config_channel(s_adc.unit, channel, &chan_cfg);
    if (err != ESP_OK) {
        return err;
    }

    err = adc_calibration_init(unit, channel, &s_adc.cali, &s_adc.cali_enabled);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ADC calibration unavailable; raw counts will be used");
    }

    s_adc.channel = channel;
    s_adc.adc_gpio = adc_gpio;
    ESP_LOGI(TAG, "Battery ADC on GPIO%d (unit %d, channel %d)", adc_gpio, unit, channel);
    return ESP_OK;
}

esp_err_t cosmos_battery_adc_read_mv(int *voltage_mv)
{
    if (!s_adc.unit || voltage_mv == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    int raw_sum = 0;
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        int raw = 0;
        esp_err_t err = adc_oneshot_read(s_adc.unit, s_adc.channel, &raw);
        if (err != ESP_OK) {
            return err;
        }
        raw_sum += raw;
    }

    const int raw_avg = raw_sum / NO_OF_SAMPLES;
    int mv = 0;

    if (s_adc.cali_enabled) {
        esp_err_t err = adc_cali_raw_to_voltage(s_adc.cali, raw_avg, &mv);
        if (err != ESP_OK) {
            return err;
        }
    } else {
        mv = raw_avg;
    }

    *voltage_mv = mv;
    return ESP_OK;
}
