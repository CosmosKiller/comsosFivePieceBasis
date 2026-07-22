# Contributing

Thanks for helping with the Cosmos Matter firmware monorepo. Each `iot*` directory is a standalone ESP-IDF project.

## Before you start

1. Install [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) and [esp-matter](https://github.com/espressif/esp-matter).
2. Read [BUILD.md](BUILD.md) for environment variables and build variants.
3. Read [HARDWARE.md](HARDWARE.md) for board and GPIO assignments.
4. Read **[CODE_STYLE.md](CODE_STYLE.md)** for formatting, naming, comments, and Doxygen — required before changing project code.

## Building

```bash
cd iotDualModeBtn   # or another app folder
export ESP_MATTER_PATH=/path/to/esp-matter
. $IDF_PATH/export.sh
idf.py build
```

## Code conventions

- **Formatting:** run clang-format using the repo [`.clang-format`](../.clang-format) (see [CODE_STYLE.md](CODE_STYLE.md)).
- **C++17** for new application code in `main/` (see each `main/CMakeLists.txt`).
- **C** is fine for existing modules (e.g. GPIO ISR paths, event service) or when interfacing with C-only ESP-IDF APIs; prefer C++ for new tasks unless there is a strong reason not to.
- **Naming / docs:** `snake_case` functions, `*_t` / `*_e` types, Doxygen on public `tasks/*.h` APIs — details in [CODE_STYLE.md](CODE_STYLE.md).
- **Task layout:** public API in `tasks/*.h`, implementation in `main/*.{cpp,c}` (see [REPO_LAYOUT.md](REPO_LAYOUT.md)).
- **Adding a task:** create `tasks/my_task.h`, implement `main/my_task.cpp`, add both to `main/CMakeLists.txt` `SRCS` and keep `INCLUDE_DIRS "." "../tasks"`.
- **Shared Matter glue:** factory reset and standard device-layer events live in `components/cosmos_matter_common` (`factory_reset_task`, `cosmos_matter_handle_device_event`). That component’s `idf_component.yml` pulls `espressif/button` for the reset button — apps do not need their own `main/idf_component.yml` for it. App `matter_task.cpp` files only forward events and implement `app_attribute_update_cb`.
- **Battery monitoring:** ADC sampling and Matter Power Source publishing live in `components/cosmos_battery`. Each app adds a Power Source endpoint and calls `cosmos_battery_init()` / `cosmos_battery_start()` from `main.cpp`.
- **Matter thread safety:** do not call Matter attribute APIs directly from GPIO/button callbacks or ISRs. Schedule work on the CHIP system layer, for example:

```cpp
chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, value]() {
    // attribute::update(...) or cluster accessors here
});
```

- **Logging:** use `ESP_LOGx` with a file-level `static const char *TAG`.
- **Headers:** add a one-line `@brief` on new source files; document non-obvious public functions in `tasks/*.h` with Doxygen (`@param`, `/*!< */` on struct members).

## Personal notes

Per-app `To-Do.MD` files are **gitignored** on purpose (local checklists only). Put durable hardware or module status in `docs/HARDWARE.md` or issues when it should be shared.

## Pull requests

- Keep changes scoped to one app or shared `components/` when possible.
- Format touched C/C++ files with clang-format before opening a PR.
- Confirm the affected project still builds with `idf.py build`.
- Do not commit `build/`, `managed_components/`, or generated `sdkconfig` unless the repo policy changes (see [POLISH_PLAN.md](POLISH_PLAN.md)).
