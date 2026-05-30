#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <string.h>

#include <panic_alarm_task.h>

static const char *TAG = "panic_alarm";
bool is_initialized = false;
bool buzzer_ready = false;

TaskHandle_t panic_handle = NULL;

/**
 * @brief Initialize panic alarm buzzer GPIO
 */
static void panic_alarm_buzzer_init(void)
{
    // Buzzer config
    gpio_config_t buzzer_conf = {
        .pin_bit_mask = (1ULL << ALARM_BUZZER_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&buzzer_conf);
    gpio_set_level(ALARM_BUZZER_PIN, 0);
    buzzer_ready = true;
}

/**
 * @brief Panic alarm task function
 *
 * @param pParameters
 */
static void panic_alarm_task_handler(void *pParameters)
{
    ESP_LOGW(TAG, "Panic alarm warning! Buzzing alarm");

    for (int j = 1000; j >= 250; j -= 250) {
        for (int i = 0; i < 5; i++) {
            gpio_set_level(ALARM_BUZZER_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(j)); // Ensure buzzer state is updated
            gpio_set_level(ALARM_BUZZER_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(j));
        }
    }

    ESP_LOGE(TAG, "Panic alarm triggered! Buzzing alarm...");

    while (1) {
        gpio_set_level(ALARM_BUZZER_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(250)); // Ensure buzzer state is updated
        gpio_set_level(ALARM_BUZZER_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

esp_err_t panic_alarm_task_init(void)
{
    if (is_initialized && panic_handle != NULL) {
        ESP_LOGW(TAG, "Panic alarm task already initialized");
        return ESP_OK;
    }

    // Initialize GPIO for buzzer if not already done
    if (!buzzer_ready) {
        panic_alarm_buzzer_init();
    }

    BaseType_t ret = xTaskCreate(
        panic_alarm_task_handler,
        "panic_alarm_task_handler",
        PANIC_ALARM_STACK_SIZE,
        NULL,
        PANIC_ALARM_TASK_PRIORITY,
        &panic_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create panic alarm task");
        return ESP_ERR_NO_MEM;
    }

    is_initialized = true;
    return ESP_OK;
}

esp_err_t panic_alarm_task_deinit(void)
{
    if (!is_initialized || panic_handle == NULL) {
        ESP_LOGW(TAG, "Panic alarm task not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    vTaskDelete(panic_handle); // Delete the current task (panic alarm task)

    gpio_set_level(ALARM_BUZZER_PIN, 0); // Ensure buzzer is turned off

    is_initialized = false;
    return ESP_OK;
}