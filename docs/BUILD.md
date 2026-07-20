# Build and Matter setup

## Environment

| Variable | Purpose |
|----------|---------|
| `IDF_PATH` | ESP-IDF installation |
| `ESP_MATTER_PATH` | esp-matter repository root (required by each app `CMakeLists.txt`) |
| `ESP_MATTER_DEVICE_PATH` | Device HAL under esp-matter (optional; set per target in project CMake) |
| `SDKCONFIG_DEFAULTS` | Semicolon-separated defaults files before `idf.py reconfigure` |

Each app ships **`sdkconfig.defaults`** in its project folder (loaded automatically on build). Shared baseline options live in [sdkconfig.defaults.matter-base](sdkconfig.defaults.matter-base) and are merged into those files.

## Per-app targets

| App | Board | `idf.py set-target` |
|-----|-------|------------------------|
| `iotBasicBinarySensor` | XIAO ESP32-C6 | `esp32c6` |
| `iotDualModeBtn` | XIAO ESP32-C6 | `esp32c6` |
| `iotEnvironmentalSensor` | XIAO ESP32-C5 | `esp32c5` |

Standard workflow inside a project directory:

```bash
cd iotBasicBinarySensor   # or another app
export ESP_MATTER_PATH=/path/to/esp-matter
. $IDF_PATH/export.sh
idf.py set-target esp32c6    # esp32c5 for iotEnvironmentalSensor
idf.py build
idf.py flash monitor
```

Artifacts land in `build/` (gitignored). Dependencies resolve to `managed_components/` (gitignored).

## ESP32-C6: Thread vs Thread + Wi-Fi

Applies to **`iotDualModeBtn`** (extra defaults files in that project).

**Thread only**

```bash
cd iotDualModeBtn
export SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.c6_thread"
idf.py reconfigure
idf.py build
```

**Thread + Wi-Fi**

```bash
export SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.c6_wifi_thread"
idf.py reconfigure
idf.py build
```

## ESP32-C5 (environmental sensor)

Retarget from the legacy `esp32` config when setting up a fresh tree:

```bash
cd iotEnvironmentalSensor
rm -f sdkconfig sdkconfig.old
idf.py set-target esp32c5
idf.py build
```

`sdkconfig.defaults` already sets `CONFIG_IDF_TARGET_ESP32C5=y` and Wi-Fi Matter options. CMake resolves `ESP_MATTER_DEVICE_PATH` to `esp32c5_devkit_c` under esp-matter.

## Matter certificates

- [How to Secure Matter Certs](https://mattercoder.com/codelabs/how-to-secure-matter-certs/?index=..%2F..index#5) (Matter Coder codelab)

Manufacturing scripts for the binary sensor live under `iotBasicBinarySensor/mfg_tool_scripts/` (gitignored; local factory tooling only).

## Clean rebuild

```bash
idf.py fullclean
idf.py build
```

If Kconfig defaults changed:

```bash
rm -f sdkconfig sdkconfig.old
export SDKCONFIG_DEFAULTS="sdkconfig.defaults;..."   # if using dual-mode Thread variants
idf.py set-target esp32c6   # or esp32c5
idf.py build
```
