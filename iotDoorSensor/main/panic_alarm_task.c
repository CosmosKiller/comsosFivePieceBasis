#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <string.h>

#include <panic_alarm_task.h>

static const char *TAG = "panic_alarm";

static bool s_gpio_ready = false;
static TaskHandle_t s_arm_task = NULL;
static TaskHandle_t s_siren_task = NULL;

extern bool is_armed;

static void panic_alarm_gpio_init(void)
{
    gpio_config_t alarm_led_conf = {
        .pin_bit_mask = (1ULL << ALARM_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&alarm_led_conf);
    gpio_set_level(ALARM_LED_PIN, 0);

    gpio_config_t confirm_led_conf = {
        .pin_bit_mask = (1ULL << CONFIRM_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&confirm_led_conf);
    gpio_set_level(CONFIRM_LED_PIN, 0);

    s_gpio_ready = true;
}

static void panic_alarm_task_arming(void *pParameters)
{
    (void)pParameters;
    ESP_LOGW(TAG, "Arming alarm! one minute to exit the premises.");

    for (int i = 0; i < 58; i++) {
        gpio_set_level(ALARM_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(ALARM_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    for (int i = 0; i < 4; i++) {
        gpio_set_level(CONFIRM_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(250));
        gpio_set_level(CONFIRM_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(250));
    }

    ESP_LOGW(TAG, "Alarm armed! Panic will trigger if door/window opens.");
    is_armed = true;

    while (1) {
        gpio_set_level(ALARM_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(ALARM_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(599000));
    }
}

static void panic_alarm_task_siren_active(void *pParameters)
{
    (void)pParameters;
    ESP_LOGW(TAG, "Siren warning — full alarm in a few seconds.");

    for (int j = 1000; j >= 250; j -= 250) {
        for (int i = 0; i < 5; i++) {
            gpio_set_level(ALARM_LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(j));
            gpio_set_level(ALARM_LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(j));
        }
    }

    ESP_LOGE(TAG, "Siren active (latched until Matter siren Off)");

    while (1) {
        gpio_set_level(ALARM_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(250));
        gpio_set_level(ALARM_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

esp_err_t panic_alarm_task_start_arming(void)
{
    if (s_arm_task != NULL) {
        return ESP_OK;
    }

    if (!s_gpio_ready) {
        panic_alarm_gpio_init();
    }

    BaseType_t ret = xTaskCreate(panic_alarm_task_arming, "panic_alarm_arming", PANIC_ALARM_STACK_SIZE, NULL,
                                 PANIC_ALARM_TASK_PRIORITY, &s_arm_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to start arming task");
        s_arm_task = NULL;
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t panic_alarm_task_disarm(void)
{
    if (s_arm_task != NULL) {
        vTaskDelete(s_arm_task);
        s_arm_task = NULL;
    }

    is_armed = false;
    gpio_set_level(CONFIRM_LED_PIN, 0);
    ESP_LOGI(TAG, "Disarmed (siren unchanged if active)");
    return ESP_OK;
}

esp_err_t panic_alarm_task_start_siren(void)
{
    if (s_siren_task != NULL) {
        return ESP_OK;
    }

    if (!s_gpio_ready) {
        panic_alarm_gpio_init();
    }

    BaseType_t ret = xTaskCreate(panic_alarm_task_siren_active, "panic_alarm_siren", PANIC_ALARM_STACK_SIZE, NULL,
                                 PANIC_ALARM_TASK_PRIORITY, &s_siren_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to start siren task");
        s_siren_task = NULL;
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t panic_alarm_task_stop_siren(void)
{
    if (s_siren_task != NULL) {
        vTaskDelete(s_siren_task);
        s_siren_task = NULL;
    }

    gpio_set_level(ALARM_LED_PIN, 0);
    ESP_LOGI(TAG, "Siren stopped");
    return ESP_OK;
}

bool panic_alarm_task_siren_is_active(void)
{
    return s_siren_task != NULL;
}