# Build and Matter setup

## Environment

| Variable | Purpose |
|----------|---------|
| `IDF_PATH` | ESP-IDF installation |
| `ESP_MATTER_PATH` | esp-matter repository root (required by each app `CMakeLists.txt`) |
| `ESP_MATTER_DEVICE_PATH` | Device HAL under esp-matter (optional; set per target in project CMake) |
| `SDKCONFIG_DEFAULTS` | Semicolon-separated defaults files before `idf.py reconfigure` |

Standard workflow inside a project directory:

```bash
idf.py set-target esp32c6    # or esp32c5 when environmental app is retargeted
idf.py build
idf.py flash monitor
```

Artifacts land in `build/` (gitignored). Dependencies resolve to `managed_components/` (gitignored).

## ESP32-C6: Thread vs Thread + Wi-Fi

Applies to **`iotDualModeBtn`** (defaults files in that project).

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

## ESP32-C5

Environmental sensor hardware targets XIAO ESP32-C5; retarget and add `sdkconfig.defaults` variants when that app is brought in line (see [POLISH_PLAN.md](POLISH_PLAN.md)).

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
export SDKCONFIG_DEFAULTS="sdkconfig.defaults;..."   # if used
idf.py set-target esp32c6
idf.py build
```
