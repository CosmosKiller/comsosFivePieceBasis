#ifndef SECURITY_MODULE_H_
#define SECURITY_MODULE_H_

#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_matter.h>

#define PIR_PIN         GPIO_NUM_2 /*!< XIAO D1 — PIR input */
#define TAMPER_PIN      GPIO_NUM_3 /*!< XIAO D2 — mount/tamper contact (NC to GND when seated) */
#define TRIGGER_TIME_MS 5000

using security_module_cb_t = void (*)(uint16_t endpoint_id, bool active, void *user_data);

typedef struct {
    struct {
        security_module_cb_t cb = NULL;
        uint16_t endpoint_id;
    } pir_sensor;

    struct {
        security_module_cb_t cb = NULL;
        uint16_t endpoint_id;
    } tamper;

    void *user_data = NULL;
} security_module_config_t;

/**
 * @brief Initialize PIR + tamper contact GPIOs. Call once.
 *
 * @param pConfig Sensor config; must outlive the driver.
 */
esp_err_t security_module_task_init(security_module_config_t *pConfig);

/**
 * @brief Read tamper GPIO (true = open / unit removed). Safe anytime after GPIO init.
 */
bool security_module_tamper_is_open(void);

#endif // SECURITY_MODULE_H_
