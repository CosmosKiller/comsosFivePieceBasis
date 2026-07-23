# Build and Matter setup

## Pinned toolchains (tested)

Record the combo you build with locally. CI uses the `espressif/esp-matter:latest` container (ESP-IDF + esp-matter preinstalled).

| Component | Version |
|-----------|---------|
| ESP-IDF | [v5.4.1](https://github.com/espressif/esp-idf/releases/tag/v5.4.1) |
| esp-matter | commit [`2cb668c`](https://github.com/espressif/esp-matter) (Jan 2026 local build) |

Clone and align locally:

```bash
git clone -b v5.4.1 --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf
git clone https://github.com/espressif/esp-matter.git ~/esp/esp-matter
cd ~/esp/esp-matter && git checkout 2cb668c && git submodule update --init --recursive
cd ~/esp/esp-matter && ./install.sh
. ~/esp/esp-idf/export.sh
export ESP_MATTER_PATH=~/esp/esp-matter
```

Update this table when you bump toolchains.

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

**Generated config:** `sdkconfig` / `sdkconfig.old` are **not** tracked — only `sdkconfig.defaults` (and `sdkconfig.defaults.*` variants). After clone: `idf.py set-target …` then `idf.py build`.

**Battery monitor defaults:** each app’s `sdkconfig.defaults` sets `CONFIG_COSMOS_BATTERY_*` (GPIO and sample interval) for that board. Override there for CI/reproducible builds, or use **Component config → Cosmos battery monitor** in `idf.py menuconfig` for local tuning.

**Component locks:** each app commits `dependencies.lock` (ESP-IDF Component Manager) for reproducible `managed_components/` resolution.

## Build all apps

From repo root (after sourcing ESP-IDF and setting `ESP_MATTER_PATH`):

```bash
./tools/scripts/build_all.sh
```

Fresh config from defaults only (matches CI):

```bash
FRESH_CONFIG=1 ./tools/scripts/build_all.sh
```

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
idf.py --preview set-target esp32c5
idf.py --preview build
```

ESP32-C5 is still a **preview** target in ESP-IDF 5.4 — append `--preview` to `idf.py` (CI does this automatically for the environmental sensor job).

`sdkconfig.defaults` already sets `CONFIG_IDF_TARGET_ESP32C5=y` and Wi-Fi Matter options. CMake resolves `ESP_MATTER_DEVICE_PATH` to `esp32c5_devkit_c` under esp-matter.

## Matter certificates

- [How to Secure Matter Certs](https://mattercoder.com/codelabs/how-to-secure-matter-certs/?index=..%2F..index#5) (Matter Coder codelab)

Manufacturing scripts for the binary sensor live under `iotBasicBinarySensor/mfg_tool_scripts/` (gitignored; local factory tooling only).

## OTA images and testing

With `CONFIG_CHIP_OTA_IMAGE_BUILD=y` and `CONFIG_DEVICE_SOFTWARE_VERSION_NUMBER` set in the app’s `sdkconfig.defaults`, `idf.py build` emits `<app-name>-ota.bin` under `build/`. Version numbers must come from `PROJECT_VER` / `PROJECT_VER_NUMBER` in the app `CMakeLists.txt`.

End-to-end local OTA testing (chip-tool + `chip-ota-provider-app`): [TESTING.md](TESTING.md).

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
