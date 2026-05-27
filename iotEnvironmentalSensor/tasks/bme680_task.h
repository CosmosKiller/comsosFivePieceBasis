#ifndef MAIN_BME680_TASK_H_
#define MAIN_BME680_TASK_H_

#include <bme680.h>
#include <esp_err.h>

// I2C interface defintions for ESP32 and ESP8266
#define I2C_BUS     I2C_NUM_0
#define I2C_SCL_PIN GPIO_NUM_22
#define I2C_SDA_PIN GPIO_NUM_21
#define I2C_FREQ    I2C_FREQ_100K
#define I2C_ADDR    BME680_I2C_ADDR_0

using bme680_sensor_cb_t = void (*)(uint16_t endpoint_id, float value, void *user_data);

/**
 * @brief Configuration structure for the BME680 sensor
 *
 */
typedef struct {
    struct {
        bme680_sensor_cb_t cb = NULL; /*!< This callback functon will be called periodically to report the temperature.*/
        uint16_t endpoint_id;         /*!< Endpoint_id associated with temperature sensor */
    } temperature;

    struct {
        bme680_sensor_cb_t cb = NULL; /*!< This callback functon will be called periodically to report the humidity.*/
        uint16_t endpoint_id;         /*!< Endpoint_id associated with humidity sensor */
    } humidity;

    struct {
        bme680_sensor_cb_t cb = NULL; /*!< This callback functon will be called periodically to report the pressure.*/
        uint16_t endpoint_id;         /*!< Endpoint_id associated with pressure sensor */
    } pressure;

    struct {
        bme680_sensor_cb_t cb = NULL; /*!< This callback functon will be called periodically to report the gas resistance.*/
        uint16_t endpoint_id;         /*!< Endpoint_id associated with gas resistance sensor */
    } gas_resistance;

    void *user_data = NULL; /*!< User data*/

    uint32_t interval_ms = 5000; /*!< Polling interval in milliseconds, defaults to 5000 ms */
} bme680_sensor_config_t;

/**
 * @brief Initialize sensor driver. This function should be called only once
 *        When initializing, at least one callback should be provided, else it
 *        returns ESP_ERR_INVALID_ARG.
 *
 * @param pConfig sensor configurations. This should last for the lifetime of the driver
 *                as driver layer do not make a copy of this object.
 *
 * @return esp_err_t - ESP_OK on success,
 *                     ESP_ERR_INVALID_ARG if config is NULL
 *                     ESP_ERR_INVALID_STATE if driver is already initialized
 *                     appropriate error code otherwise
 */
esp_err_t bme680_task_sensor_init(bme680_sensor_config_t *pConfig);

#endif /* MAIN_BME680_TASK_H_ */