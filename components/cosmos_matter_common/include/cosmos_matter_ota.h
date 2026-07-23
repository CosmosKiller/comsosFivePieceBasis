/**
 * @file cosmos_matter_ota.h
 * @brief Shared Matter OTA requestor configuration (post-start).
 */

#ifndef COSMOS_MATTER_OTA_H_
#define COSMOS_MATTER_OTA_H_

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Apply post-start OTA configuration (encrypted image support, etc.).
 *
 * Call after esp_matter::start(). OTA cluster registration and requestor startup
 * are handled internally by esp-matter when CONFIG_ENABLE_OTA_REQUESTOR is set.
 *
 * @return ESP_OK on success, ESP_ERR_NOT_SUPPORTED when OTA is disabled in Kconfig.
 */
esp_err_t cosmos_matter_ota_configure(void);

#ifdef __cplusplus
}
#endif

#endif /* COSMOS_MATTER_OTA_H_ */
