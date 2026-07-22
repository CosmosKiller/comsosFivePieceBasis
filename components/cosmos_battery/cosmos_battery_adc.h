#pragma once

#include <esp_err.h>

esp_err_t cosmos_battery_adc_open(int adc_gpio);
esp_err_t cosmos_battery_adc_read_mv(int *voltage_mv);
