#include <esp_log.h>

#include <esp_matter_ota.h>

#include <cosmos_matter_ota.h>

static const char *TAG = "cosmos_matter_ota";

#if CONFIG_ENABLE_ENCRYPTED_OTA
extern const char cosmos_ota_decryption_key_start[] asm("_binary_esp_image_encryption_key_pem_start");
extern const char cosmos_ota_decryption_key_end[] asm("_binary_esp_image_encryption_key_pem_end");
#endif

esp_err_t cosmos_matter_ota_configure(void)
{
#if CONFIG_ENABLE_OTA_REQUESTOR
    ESP_LOGI(TAG, "Matter OTA requestor enabled");

#if CONFIG_ENABLE_ENCRYPTED_OTA
    const char *key = cosmos_ota_decryption_key_start;
    const uint16_t key_len = cosmos_ota_decryption_key_end - cosmos_ota_decryption_key_start;
    esp_err_t err = esp_matter_ota_requestor_encrypted_init(key, key_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Encrypted OTA init failed: %d", err);
        return err;
    }
    ESP_LOGI(TAG, "Encrypted OTA decryption key loaded");
#endif

    return ESP_OK;
#else
    ESP_LOGW(TAG, "Matter OTA requestor disabled (CONFIG_ENABLE_OTA_REQUESTOR=n)");
    return ESP_ERR_NOT_SUPPORTED;
#endif
}
