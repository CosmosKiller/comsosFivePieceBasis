#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <lib/support/CodeUtils.h>

#include <binary_sensor_task.h>
#include <evt_service_task.h>
#include <panic_alarm_task.h>

using namespace chip::app::Clusters;
using namespace esp_matter;

static const char *TAG = "binary_sensor_task";
extern uint16_t alarm_led_endpoint_id;

typedef struct {
    binary_sensor_config_t *config;
    bool is_initialized;
    bool is_started;
    QueueHandle_t level_queue;
    TaskHandle_t worker_task;
} binary_sensor_ctx_t;

static binary_sensor_ctx_t s_ctx;
bool is_armed = false;
bool is_panic = false;

typedef struct {
    bool level;
} binary_sensor_level_evt_t;

static void binary_sensor_handle_level(bool level)
{
    if (s_ctx.config == NULL || s_ctx.config->cb == NULL) {
        return;
    }

    s_ctx.config->cb(s_ctx.config->endpoint_id, level, s_ctx.config->user_data);

    evt_service_event_t evt;
    if (is_panic) {
        evt = {
            .source = EVT_SOURCE_PANIC,
            .type = EVT_TYPE_SUSTAINED,
            .value = 1,
        };
    } else if (!level && is_armed) {
        evt = {
            .source = EVT_SOURCE_PANIC,
            .type = EVT_TYPE_TRIGGERED,
            .value = 1,
        };
    } else {
        evt = {
            .source = EVT_SOURCE_SENSOR,
            .type = level ? EVT_TYPE_CLEARED : EVT_TYPE_TRIGGERED,
            .value = level ? 1 : 0,
        };
    }
    evt_service_post(&evt);
}

static void binary_sensor_worker(void *arg)
{
    (void)arg;
    binary_sensor_level_evt_t evt;

    while (xQueueReceive(s_ctx.level_queue, &evt, portMAX_DELAY) == pdTRUE) {
        binary_sensor_handle_level(evt.level);
    }
}

static void IRAM_ATTR binary_sensor_isr_handler(void *arg)
{
    (void)arg;

    if (s_ctx.level_queue == NULL) {
        return;
    }

    const bool level = gpio_get_level(SENSOR_PIN);
    const binary_sensor_level_evt_t evt = {.level = level};

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(s_ctx.level_queue, &evt, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void binary_sensor_gpio_init(void)
{
    gpio_reset_pin(SENSOR_PIN);
    gpio_set_intr_type(SENSOR_PIN, GPIO_INTR_ANYEDGE);
    gpio_set_direction(SENSOR_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SENSOR_PIN, GPIO_PULLDOWN_ONLY);
}

esp_err_t binary_sensor_attribute_update(binary_sensor_task_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val)
{
    (void)driver_handle;
    esp_err_t err = ESP_OK;

    if (endpoint_id == alarm_led_endpoint_id) {
        if (cluster_id == OnOff::Id) {
            if (attribute_id == OnOff::Attributes::OnOff::Id) {
                evt_service_event_t evt = {
                    .source = EVT_SOURCE_ALARM,
                    .type = (val->val.b) ? EVT_TYPE_TRIGGERED : EVT_TYPE_CLEARED,
                    .value = val->val.i,
                };
                evt_service_post(&evt);
            }
        }
    } else {
        ESP_LOGW(TAG, "Received attribute update for unknown endpoint: %d", endpoint_id);
    }

    return err;
}

esp_err_t binary_sensor_task_init(binary_sensor_config_t *pConfig)
{
    if (pConfig == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_ctx.is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    s_ctx.level_queue = xQueueCreate(8, sizeof(binary_sensor_level_evt_t));
    if (s_ctx.level_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ret = xTaskCreatePinnedToCore(
        binary_sensor_worker,
        "binary_sensor_worker",
        3072,
        NULL,
        5,
        &s_ctx.worker_task,
        0);
    if (ret != pdPASS) {
        vQueueDelete(s_ctx.level_queue);
        s_ctx.level_queue = NULL;
        return ESP_ERR_NO_MEM;
    }

    binary_sensor_gpio_init();

    s_ctx.config = pConfig;
    s_ctx.is_initialized = true;
    return ESP_OK;
}

esp_err_t binary_sensor_task_start(void)
{
    if (!s_ctx.is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_ctx.is_started) {
        return ESP_ERR_INVALID_STATE;
    }

    static bool isr_service_installed = false;
    if (!isr_service_installed) {
        gpio_install_isr_service(0);
        isr_service_installed = true;
    }

    esp_err_t err = gpio_isr_handler_add(SENSOR_PIN, binary_sensor_isr_handler, NULL);
    if (err != ESP_OK) {
        return err;
    }

    binary_sensor_handle_level(gpio_get_level(SENSOR_PIN));

    s_ctx.is_started = true;
    ESP_LOGI(TAG, "Binary sensor ISR started");
    return ESP_OK;
}
