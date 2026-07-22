/**
 * @file cosmos_battery.h
 * @brief Shared battery ADC sampling and optional Matter publishing.
 */

#ifndef COSMOS_BATTERY_H_
#define COSMOS_BATTERY_H_

#include <esp_err.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Matter BatPercentRemaining uses 0.5% steps (200 = 100%). */
#define COSMOS_BATTERY_PERCENT_UNKNOWN 0

typedef struct {
    int adc_gpio;         /*!< GPIO on the battery sense divider */
    float divider_ratio;  /*!< Multiply ADC volts to obtain cell voltage */
    float voltage_full_v; /*!< Full-charge cell voltage */
    float voltage_empty_v;/*!< Cutoff / empty cell voltage */
    uint32_t interval_ms; /*!< Sample period */
    uint16_t endpoint_id; /*!< Matter Power Source endpoint id (0 to skip Matter updates) */
} cosmos_battery_config_t;

/**
 * @brief Fill config with Kconfig defaults (GPIO, interval) and typical Li-ion thresholds.
 *
 * @param config Configuration struct to initialize.
 */
void cosmos_battery_config_set_defaults(cosmos_battery_config_t *config);

/**
 * @brief Initialize ADC calibration and store runtime configuration.
 *
 * @param config Configuration; must remain valid for the driver lifetime.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t cosmos_battery_init(const cosmos_battery_config_t *config);

/**
 * @brief Start periodic battery sampling (esp_timer).
 *
 * @return ESP_OK on success, or an error code.
 */
esp_err_t cosmos_battery_start(void);

#ifdef __cplusplus
}
#endif

#endif /* COSMOS_BATTERY_H_ */
