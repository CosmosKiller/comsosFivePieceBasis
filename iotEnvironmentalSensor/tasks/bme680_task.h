/**
 * @file bme680_task.h
 * @brief BME680 I2C driver with periodic reads and Matter measurement callbacks.
 */

#ifndef MAIN_BME680_TASK_H_
#define MAIN_BME680_TASK_H_

#include <bme680.h>
#include <esp_err.h>

#define I2C_BUS     I2C_NUM_0         /*!< I2C peripheral */
#define I2C_SCL_PIN GPIO_NUM_22       /*!< I2C clock GPIO */
#define I2C_SDA_PIN GPIO_NUM_21       /*!< I2C data GPIO */
#define I2C_FREQ    I2C_FREQ_100K     /*!< I2C bus frequency */
#define I2C_ADDR    BME680_I2C_ADDR_0 /*!< BME680 I2C address */

using bme680_sensor_cb_t = void (*)(uint16_t endpoint_id, float value, void *user_data);

/**
 * @brief Per-cluster callbacks and polling interval for the BME680 driver.
 */
typedef struct {
    struct {
        bme680_sensor_cb_t cb; /*!< Temperature reading callback */
        uint16_t endpoint_id;  /*!< Temperature measurement endpoint */
    } temperature;

    struct {
        bme680_sensor_cb_t cb; /*!< Relative humidity callback */
        uint16_t endpoint_id;  /*!< Humidity measurement endpoint */
    } humidity;

    struct {
        bme680_sensor_cb_t cb; /*!< Pressure callback */
        uint16_t endpoint_id;  /*!< Pressure measurement endpoint */
    } pressure;

    struct {
        bme680_sensor_cb_t cb; /*!< Gas resistance callback (optional) */
        uint16_t endpoint_id;  /*!< TVOC / gas endpoint when enabled */
    } gas_resistance;

    void *user_data;             /*!< Passed to each callback */
    uint32_t interval_ms = 5000; /*!< Polling interval in milliseconds */
} bme680_sensor_config_t;

/**
 * @brief Initialize I2C, BME680, and the periodic measurement task (call once).
 *
 * At least one of temperature, humidity, or pressure callbacks must be set.
 *
 * @param pConfig Configuration; must remain valid for the driver lifetime.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t bme680_task_sensor_init(bme680_sensor_config_t *pConfig);

#endif /* MAIN_BME680_TASK_H_ */
