# Hardware reference

Per-device GPIO and module status. Update this file when pinouts change; keep `To-Do.MD` in sync until those files are retired (see [POLISH_PLAN.md](POLISH_PLAN.md)).

## iotBasicBinarySensor

**Board:** [Seeed XIAO ESP32-C6](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)

| Signal | Role |
|--------|------|
| Digital in ×2 | Reed switch, reset button |
| Digital out ×3 | R/G/B LEDs (with buzzer on R/G channels per design) |
| Analog in ×1 | Battery monitoring |

**Modules:** Matter, sensor driver, OTA — battery management open.

## iotDualModeBtn

**Board:** [Seeed XIAO ESP32-C6](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)

| Signal | Role |
|--------|------|
| Digital in ×2 | Action button, reset button |
| Digital out ×3 | RGB LED |
| Analog in ×1 | Battery monitoring |

**Modules:** Matter, button driver — battery management and OTA open.

## iotEnvironmentalSensor

**Board (target):** [Seeed XIAO ESP32-C5](https://wiki.seeedstudio.com/xiao_esp32c5_getting_started/)

| Signal | Role |
|--------|------|
| D0/GPIO1, D1/GPIO0, D2/GPIO25, BOOT/GPIO28 | Rotary encoder, reset |
| GPIO23 / GPIO24 | I2C SDA / SCL (BME680) |
| SPI (GPIO8–12, etc.) | ST7789 display (planned) |
| A6/GPIO6 | Battery monitoring |
| Optional | RGB LED outputs |

**Modules:** Matter, BME680, OTA done — display, battery, custom QR open.

> **Note:** `iotEnvironmentalSensor/sdkconfig` may still list `esp32` until the project is retargeted to C5; treat this table as the hardware intent.
