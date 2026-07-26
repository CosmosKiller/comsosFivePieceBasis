#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <panic_alarm_task.h>

static const char *TAG = "panic_alarm";

static bool s_gpio_ready = false;
static bool s_running = false;
static TaskHandle_t s_task = NULL;

static void panic_alarm_gpio_init(void)
{
    gpio_config_t conf = {
        .pin_bit_mask = (1ULL << ALARM_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&conf);
    gpio_set_level(ALARM_LED_PIN, 0);
    s_gpio_ready = true;
}

static void panic_alarm_task_active(void *pParameters)
{
    (void)pParameters;
    ESP_LOGW(TAG, "Panic alarm warning — full alarm in a few seconds");

    for (int j = 1000; j >= 250; j -= 250) {
        for (int i = 0; i < 5; i++) {
            gpio_set_level(ALARM_LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(j));
            gpio_set_level(ALARM_LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(j));
        }
    }

    ESP_LOGE(TAG, "Panic alarm active (latched until HA clears alarm OnOff)");
    while (1) {
        gpio_set_level(ALARM_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(250));
        gpio_set_level(ALARM_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

esp_err_t panic_alarm_task_init(void)
{
    if (s_running && s_task != NULL) {
        return ESP_OK;
    }

    if (!s_gpio_ready) {
        panic_alarm_gpio_init();
    }

    BaseType_t ret = xTaskCreate(panic_alarm_task_active, "panic_alarm_task", PANIC_ALARM_STACK_SIZE, NULL,
                                 PANIC_ALARM_TASK_PRIORITY, &s_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create panic alarm task");
        s_task = NULL;
        return ESP_ERR_NO_MEM;
    }

    s_running = true;
    return ESP_OK;
}

esp_err_t panic_alarm_task_deinit(void)
{
    if (!s_running || s_task == NULL) {
        if (s_gpio_ready) {
            gpio_set_level(ALARM_LED_PIN, 0);
        }
        return ESP_OK;
    }

    vTaskDelete(s_task);
    s_task = NULL;
    s_running = false;
    gpio_set_level(ALARM_LED_PIN, 0);
    ESP_LOGI(TAG, "Panic alarm stopped");
    return ESP_OK;
}

bool panic_alarm_task_is_active(void)
{
    return s_running;
}
