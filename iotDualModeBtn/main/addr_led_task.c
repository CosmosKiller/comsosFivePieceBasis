#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include <stdio.h>

// Configuration
#define LED_STRIP_BLINK_GPIO  8                     // GPIO 8 is the built-in addressable LED on most C6 development boards
#define LED_STRIP_LED_NUMBERS 1                     // Change this to match your actual LED count
#define LED_STRIP_CHIP        LED_STRIP_CHIP_WS2812 // LED type

static const char *TAG = "led_example";
static led_strip_handle_t led_strip;

void configure_led(void)
{
    ESP_LOGI(TAG, "Initializing LED strip peripheral...");

    // LED strip general configuration
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_BLINK_GPIO,
        .max_leds = LED_STRIP_LED_NUMBERS,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // WS2812 uses GRB format
        .led_model = LED_STRIP_CHIP,
        .flags.invert_out = false,
    };

    // RMT backend specific configuration
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz resolution (standard for timing)
        .flags.with_dma = false,
    };

    // Create the LED strip object using the RMT backend
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    // Clear the strip (turn all off initially)
    led_strip_clear(led_strip);
}

void app_main(void)
{
    configure_led();

    while (1) {
        // Set to Red
        ESP_LOGI(TAG, "Setting LED to Red");
        led_strip_set_pixel(led_strip, 0, 255, 0, 0); // arguments: handle, index, Red, Green, Blue
        led_strip_refresh(led_strip);                 // Push the data to the physical LED
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Set to Green
        ESP_LOGI(TAG, "Setting LED to Green");
        led_strip_set_pixel(led_strip, 0, 0, 255, 0);
        led_strip_refresh(led_strip);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Set to Blue
        ESP_LOGI(TAG, "Setting LED to Blue");
        led_strip_set_pixel(led_strip, 0, 0, 0, 255);
        led_strip_refresh(led_strip);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
