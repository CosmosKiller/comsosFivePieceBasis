# Cosmos Five-Piece Basis

Monorepo of ESP-IDF firmware applications for a small Matter device family. Each subdirectory is an independent project (own `sdkconfig`, partition table, and flash image).


| Project                                             | Matter role                                      | Target board (see [docs/HARDWARE.md](docs/HARDWARE.md)) |
| --------------------------------------------------- | ------------------------------------------------ | ------------------------------------------------------- |
| [iotBasicBinarySensor](iotBasicBinarySensor/)     | Boolean state / alarm-style binary sensor        | Seeed XIAO ESP32-C6                                     |
| [iotDualModeBtn](iotDualModeBtn/)                | Switch / button (press, multi-press, long-press) | Seeed XIAO ESP32-C6                                     |
| [iotEnvironmentalSensor](iotEnvironmentalSensor/)| Environmental sensing (BME680)                   | Seeed XIAO ESP32-C5 (planned)                           |


The repo name reflects a **five-device product line**; three firmware apps exist today. Additional SKUs are planned.

## Prerequisites

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) (projects currently built against **ESP32-C6** for binary sensor and dual-mode button; confirm `CONFIG_IDF_TARGET` in each app’s `sdkconfig`)
- [esp-matter](https://github.com/espressif/esp-matter) — set before building:

```bash
export ESP_MATTER_PATH=/path/to/esp-matter
# Optional: override device HAL path (defaults per IDF_TARGET in each project's CMakeLists.txt)
export ESP_MATTER_DEVICE_PATH=$ESP_MATTER_PATH/device_hal/device/esp32c6_devkit_c
export IDF_PATH=/path/to/esp-idf
. $IDF_PATH/export.sh
```

- Matter development credentials / factory data as required by your commissioning flow ([docs/BUILD.md](docs/BUILD.md#matter-certificates))

## Quick build

From any project directory:

```bash
cd iotDualModeBtn   # or iotBasicBinarySensor / iotEnvironmentalSensor
idf.py set-target esp32c6   # if not already configured
idf.py build flash monitor
```

Thread-only vs Thread+Wi-Fi on **ESP32-C6** (dual-mode button only today):

```bash
cd iotDualModeBtn
export SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.c6_thread"
idf.py reconfigure && idf.py build
```

```bash
export SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.c6_wifi_thread"
idf.py reconfigure && idf.py build
```

More detail: [docs/BUILD.md](docs/BUILD.md) (toolchain pins, build all apps, CI).

Build all firmware apps locally:

```bash
export ESP_MATTER_PATH=/path/to/esp-matter
. $IDF_PATH/export.sh
./tools/scripts/build_all.sh
```

## Repository layout

```
cosmosFivePieceBasis/
├── README.md                 # You are here
├── LICENSE
├── docs/                     # Shared documentation
│   ├── BUILD.md
│   ├── HARDWARE.md
│   ├── REPO_LAYOUT.md        # Current vs target structure
│   └── POLISH_PLAN.md        # Roadmap to tighten the repo
├── iotBasicBinarySensor/     # Firmware app (ESP-IDF project root)
├── iotDualModeBtn/
└── iotEnvironmentalSensor/
```

Long-term target layout (shared Matter glue, CI): [docs/REPO_LAYOUT.md](docs/REPO_LAYOUT.md).

Contributing (build, PR expectations): [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md).  
Code style (clang-format, naming, Doxygen): [docs/CODE_STYLE.md](docs/CODE_STYLE.md).

## Code organization (per app)

Each firmware app follows the same idea:

- `**main/**` — `app_main`, Matter endpoint setup, and task implementations (`.cpp` / `.c`)
- `**tasks/**` — Public headers for those tasks (`*.h`)
- `**main/CMakeLists.txt**` — Registers sources and `INCLUDE_DIRS` for `main` + `../tasks`

Matter lifecycle callbacks live in **`components/cosmos_matter_common`** (`cosmos_matter_handle_device_event`, `factory_reset_task`); each app’s `matter_task.cpp` forwards events and implements device-specific `app_attribute_update_cb`.

## Status

Feature checklists and GPIO notes live in each project’s `To-Do.MD` (local working notes; see [docs/POLISH_PLAN.md](docs/POLISH_PLAN.md) for moving this into tracked docs).

## License

MIT — see [LICENSE](LICENSE).