#ifndef DOOR_INTERCOM_TASK_H_
#define DOOR_INTERCOM_TASK_H_

#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_matter.h>

#define DOORBELL_PIN GPIO_NUM_1 /*!< XIAO D0 — dedicated doorbell tact */
#define DOORLOCK_PIN GPIO_NUM_5 /*!< Reserved (Phase 2) — XIAO D4 */

using door_intercom_cb_t = void (*)(uint16_t endpoint_id, bool pressed, void *user_data);

typedef void *door_intercom_task_handle_t;

typedef struct {
    struct {
        door_intercom_cb_t cb = NULL;
        uint16_t endpoint_id;
    } doorbell;

    struct {
        door_intercom_cb_t cb = NULL;
        uint16_t endpoint_id;
    } doorlock;

    void *user_data = NULL;
} door_intercom_config_t;

/** Driver Update
 *
 * @brief This API should be called to update the driver for the attribute being updated.
 *        This is usually called from the common `app_attribute_update_cb()`.
 *
 * @param driver_handle Handle to the driver instance. This is usually passed as `priv_data` while creating the endpoint.
 * @param endpoint_id Endpoint ID of the attribute.
 * @param cluster_id Cluster ID of the attribute.
 * @param attribute_id Attribute ID of the attribute.
 * @param val Pointer to `esp_matter_attr_val_t`. Use appropriate elements as per the value type.
 *
 * @return error in case of failure.
 */
esp_err_t door_intercom_attribute_update(door_intercom_task_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val);

/**
 * @brief Initialize door intercom driver. This function should be called only once
 *
 * @param pConfig sensor configurations. This should last for the lifetime of the driver
 *               as driver layer do not make a copy of this object.
 *
 * @return esp_err_t - ESP_OK on success,
 *                     ESP_ERR_INVALID_ARG if pConfig is NULL
 *                     ESP_ERR_INVALID_STATE if driver is already initialized
 *                     appropriate error code otherwise
 */

esp_err_t door_intercom_task_init(door_intercom_config_t *pConfig);

#endif // DOOR_INTERCOM_TASK_H_