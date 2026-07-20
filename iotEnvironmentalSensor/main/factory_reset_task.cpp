
// Include ESP-IDF libraries
#include <esp_log.h>
#include <stdio.h>

#include <esp_matter.h>

#include <button_gpio.h>
#include <iot_button.h>

#include <factory_reset_task.h>

static const char *TAG = "factory_reset_task";

/**
 * @brief Factory reset button callback
 *
 * This function is called when the factory reset button is long-pressed.
 * It triggers a factory reset of the device, which clears all Matter credentials and reboots the chip automatically.
 *
 * @param pArg Pointer to the button event data
 *
 */
static void factory_reset_button_cb(void *arg, void *data)
{
    ESP_LOGW(TAG, "!! Factory Reset Triggered via Button !!");
    ESP_LOGW(TAG, "Decommissioning device and erasing all Matter fabrics...");

    // This clears all Matter credentials and reboots the chip automatically
    esp_matter::factory_reset();
}

void factory_reset_task(void)
{
    // Create a button instance for the factory reset button
    // Initialize button
    button_handle_t handle = NULL;
    const button_config_t btn_cfg = {0};

    const button_gpio_config_t btn_gpio_cfg = {
        .gpio_num = FACTORY_RESET_BUTTON_PIN,
        .active_level = 0,
    };

    // Initialize the factory reset button
    if (iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create factory reset button device");
        return;
    }

    // Register the long press callback for the factory reset button
    iot_button_register_cb(handle, BUTTON_LONG_PRESS_START, NULL, factory_reset_button_cb, NULL);

    ESP_LOGI(TAG, "Factory reset task initialized. Long press the button to trigger a factory reset.");
}