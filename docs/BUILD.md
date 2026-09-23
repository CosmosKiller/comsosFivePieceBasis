# Build and Matter setup

## Pinned toolchains (tested)

Record the combo you build with locally. CI uses the `espressif/esp-matter:latest` container (ESP-IDF + esp-matter preinstalled).

| Component | Version |
|-----------|---------|
| ESP-IDF | [v5.5.5](https://github.com/espressif/esp-idf/releases/tag/v5.5.5) |
| esp-matter | commit [`ff9f07ec`](https://github.com/espressif/esp-matter/commit/ff9f07ec) (main @ full upgrade 2026-09-20) |

Clone and align locally:

```bash
git clone -b v5.5.5 --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf
# or: cd ~/esp/esp-idf && git fetch --tags && git checkout v5.5.5 && git submodule update --init --recursive && ./install.sh
git clone --recursive https://github.com/espressif/esp-matter.git ~/esp/esp-matter
cd ~/esp/esp-matter && git checkout main && git pull --ff-only && git submodule update --init --recursive
. ~/esp/esp-idf/export.sh
cd ~/esp/esp-matter && ./install.sh
export ESP_MATTER_PATH=~/esp/esp-matter
```

Update this table when you bump toolchains. **SKU 5 Matter 1.5 camera (ESP32-P4)** requires this IDF 5.5.x line.

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
| `iotDoorSensor` | XIAO ESP32-C6 | `esp32c6` |
| `iotDualModeBtn` | XIAO ESP32-C6 | `esp32c6` |
| `iotEnvironmentalSensor` | Waveshare ESP32-C5-Touch-LCD-2.8 (target) | `esp32c5` |
| `iotBedsideLamp` | XIAO / DevKit ESP32-C6 | `esp32c6` |
| `iotDoorIntercom` | Waveshare ESP32-P4-WIFI6 (P4 + C6) | `esp32p4` + `esp32c6` (Matter camera split) |
| `iotSecurityCamera` | XIAO ESP32-S3 Sense (HTTPS MJPEG) | `esp32s3` |

Standard workflow inside a project directory:

```bash
cd iotDoorSensor   # or another app
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
idf.py set-target esp32c5
idf.py build
```

`sdkconfig.defaults` already sets `CONFIG_IDF_TARGET_ESP32C5=y` and Wi-Fi Matter options. CMake resolves `ESP_MATTER_DEVICE_PATH` to `esp32c5_devkit_c` under esp-matter. Explicitly:

```bash
export ESP_MATTER_DEVICE_PATH=$ESP_MATTER_PATH/device_hal/device/esp32c5_devkit_c
```

## SKU 5 — Waveshare ESP32-P4-WIFI6 (Matter 1.5 camera)

**Product path (locked):** esp-matter **split mode** camera — same P4+C6 / SDIO layout as Espressif’s Function EV Board class.

| Image | SoC | Role | Source |
|-------|-----|------|--------|
| `matter_camera` | ESP32-C6 | Matter + WebRTC **signaling** + security endpoints | `$ESP_MATTER_PATH/examples/camera/split_mode` + monorepo `iotDoorIntercom/c6/` |
| `media_adapter` | ESP32-P4 | Capture / H.264 / WebRTC **media** + GPIO I/O | `$KVS_SDK_PATH/examples/streaming_only` + monorepo `iotDoorIntercom/p4/` |

**CosmOS glue (in-repo):** `iotDoorIntercom/{c6,p4,shared}` — Matter security endpoints on C6; doorbell/PIR/tamper/siren tasks on P4 via `BRIDGE_EVT_SECURITY_IO` / `BRIDGE_CMD_SET_SIREN`; Matter mic/speaker volume+mute via `BRIDGE_CMD_SET_AUDIO` → ES8311 (`esp_codec_dev_set_in_gain` / `set_out_vol` / mute). Upstream camera/WebRTC stay path-deps (layout A+C).

```bash
export COSMOS_FIVE_PIECE_PATH=/path/to/cosmosFivePieceBasis
./iotDoorIntercom/scripts/build_c6.sh
./iotDoorIntercom/scripts/build_p4.sh
```

**Ownership:** Matter / OTA requestor / factory-reset **policy** on **C6**; P4 **senses/actuates** (GPIO22 reset button, battery ADC later) and reports over the Hosted bridge.

**Extra dependency** (not in esp-matter alone):

```bash
git clone --recursive https://github.com/espressif/esp-port-for-amazon-kvs-sdk.git ~/esp/esp-port-for-amazon-kvs-sdk
export KVS_SDK_PATH=~/esp/esp-port-for-amazon-kvs-sdk
```

Pinned to **IDF v5.5.5** (required by `esp_lvgl_port` DPI callbacks used by KVS `streaming_only`). Waveshare bring-up uses Function EV v1.6 defaults + UART0 console / OV5647 / audio overlay (`sdkconfig.defaults.waveshare_p4_wifi6.esp32p4` under `$KVS_SDK_PATH/examples/streaming_only`).

**Bring-up order (kit on hand):**

1. C6 signaling (`split_mode`, `idf.py set-target esp32c6`) — Waveshare **C6 UART pads** + USB–TTL; short **IO9→GND** only while downloading, then **remove the strap** before normal boot (C6 must leave `waiting for download` or P4 Hosted/SDIO will never come up).

   CosmOS enables Matter **Audio + Speaker** features (`HasSpeaker()=true` in `split_mode` `camera-device.h`) so the AVSM cluster advertises two-way talk. Rebuild C6 after pulling that change:

   ```bash
   export COSMOS_FIVE_PIECE_PATH=/path/to/cosmosFivePieceBasis
   export ESP_MATTER_PATH=~/esp/esp-matter
   . ~/esp/esp-idf/export.sh
   ./iotDoorIntercom/scripts/build_c6.sh
   idf.py -C $ESP_MATTER_PATH/examples/camera/split_mode -p /dev/ttyACM0 flash   # Arduino TTL = C6
   ```

2. P4 media (`streaming_only`, `idf.py set-target esp32p4`) — board **USB-C** (QinHeng CH343, UART0). Overlay chain used on this kit:

   ```bash
   export COSMOS_FIVE_PIECE_PATH=/path/to/cosmosFivePieceBasis
   export KVS_SDK_PATH=~/esp/esp-port-for-amazon-kvs-sdk
   . ~/esp/esp-idf/export.sh
   ./iotDoorIntercom/scripts/build_p4.sh
   # or explicitly:
   cd $KVS_SDK_PATH/examples/streaming_only
   idf.py -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.defaults.esp32p4;sdkconfig.defaults.p4_function_ev_board_v16.esp32p4;sdkconfig.defaults.waveshare_p4_wifi6.esp32p4' set-target esp32p4
   idf.py -p /dev/ttyACM1 build flash   # QinHeng = P4; Arduino TTL is usually the other ACM*
   ```

   Waveshare P4 samples are often **rev v1.x** (`CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y`). SDIO Hosted pins match Function EV (CLK18/CMD19/D0–D3=14–17, C6 reset GPIO54).

   **Audio (2-way):** Waveshare onboard **ES8311** @ I2C `0x18` + **NS4150B** PA matches Function EV BSP pins (I2C 7/8, I2S 9–13, PA GPIO53). Keep `CONFIG_BSP_SELECT_ESP32_P4_FUNCTION_EV_BOARD=y` and leave `CONFIG_MEDIA_STREAM_ENABLE_VIDEO_PLAYER` **off** (GPIO27 doorbell vs LCD_RST). After flash, confirm P4 boot logs:

   - `Audio capture IF ready (media_stream_get_audio_capture_if)`
   - `Audio player IF ready (media_stream_get_audio_player_if)`
   - `P4 audio levels ready (SET_AUDIO←C6, defaults mic/spk=254)`

   Matter **MicrophoneVolumeLevel** / **SpeakerVolumeLevel** boot defaults are **254** (range Min=1 Max=254). Mic maps to ES8311 PGA with **ceil-to-6 dB** steps (max **42 dB**) so mid/high Matter levels are not stuck a step quiet. C6 pushes `BRIDGE_CMD_SET_AUDIO` on attribute change and at camera Init; P4 stores levels and applies on ES8311 open (first Live View) or immediately if already open. Look for:

   - C6: `SET_AUDIO → P4 mic=254 mute=0 spk=254 mute=0`
   - P4: `SET_AUDIO recv…` / `Applied mic levels…` / `Applied speaker levels…`

   On first Live View session (codec open):

   - `ESP32P4 microphone codec initialized (ES8311 path)`
   - `OPUS audio player initialized`

   Failures: `Failed to initialize microphone/speaker codec` → I2C/ES8311 probe; `No speaker codec on this board` → wrong BSP selected. Plug an **8 Ω** speaker on the MX1.25 header; PA is GPIO53 high (unmuted). Retest Live View: phone should hear the door mic; talkback should be audible on the speaker.

   **Wi-Fi ownership (product path):** keep `CONFIG_STREAMING_WIFI_CREDENTIALS_FROM_KCONFIG` **off** so the P4 does **not** store SSID/password — C6 `matter_camera` owns Wi-Fi via Matter commissioning; P4 still calls `esp_wifi_connect()` (no `set_config`) and waits for `IP_EVENT_STA_GOT_IP` over ESP-Hosted. Do **not** push AP credentials from the P4 onto a Matter C6 (Hosted returns `ESP_ERR_WIFI_SSID`); commission the camera from Home Assistant / a Matter commissioner instead. Lab-only Hosted STA bring-up is for a non-Matter Hosted slave image — never commit secrets (use gitignored local `sdkconfig` only).

   **Live View = noise / stuck:** confirm `got ip:` first. Kit cam = **Waveshare RPi Camera (B) / OV5647**. Use **RAW8 800×800** on this board — `VIDIOC_S_FMT` otherwise picks **1920×1080** first (still enabled formats), which overflows the ISP and yields garbage H.264 that looks like noise. Function EV defaults alone prefer SC2336.
3. Power-cycle both chips with **IO9 free** (P4 Hosted resets the C6 on host boot — wait for Matter Wi-Fi to return); confirm P4 log shows ESP-Hosted slave ready + `got ip:`, then retry Live View. Wire doorbell / PIR / tamper / siren on the locked P4 GPIOs in [HARDWARE.md](HARDWARE.md#iotdoorintercom-sku-5) — CosmOS P4 I/O SoT is `iotDoorIntercom/p4/` + `shared/include/` (tact→3V3 + PD on GPIO27).

**SKU 5 product firmware:** Waveshare **ESP32-P4-WIFI6** only — dual image (`esp32p4` media/I/O + `esp32c6` Matter camera). Dual-image **CI later**; use `iotDoorIntercom/scripts/build_*.sh` locally. **SKU 6** field firmware: `iotSecurityCamera` as `esp32s3` (HTTPS MJPEG `/stream`, still JPEG `/capture`, stream-gate OnOff, ESPectre CSI occupancy). `/capture` uses the same gate as `/stream`. The image is size-optimized and keeps the clusters those endpoints need (root commissioning, OnOff + Identify + Groups + Scenes, occupancy, power source, OTA requestor) plus the OV3660 driver. Wi-Fi Enterprise is off. This esp-matter pin still links the Binding and OTA Provider cluster implementations; no endpoint is created for them.

**SKU 6 CSI:** `francescopace/espectre` is pinned in `iotSecurityCamera/main/idf_component.yml` from the staging registry (no stable production release yet). Matter keeps the Wi-Fi station. The SDK starts after the station has an IPv4 address and publishes `OccupancySensing` with feature `kRFSensing`. Optional ESPectre groups (Direct HTTP, MQTT, provisioning) stay off so they do not share the HTTPS server. The public SDK is **GPL-3.0-only**: a distributed image that links it is a GPLv3 combined work. Cosmos-authored files stay MIT. A bench flash that is not given to anyone else does not trigger that. Confirm on hardware that occupancy updates while `https://<device-ip>/stream` is on.

Reference: [esp-matter camera](https://github.com/espressif/esp-matter/tree/main/examples/camera), [Waveshare ESP32-P4-WIFI6](https://docs.waveshare.com/ESP32-P4-WIFI6).

## Matter certificates

- [How to Secure Matter Certs](https://mattercoder.com/codelabs/how-to-secure-matter-certs/?index=..%2F..index#5) (Matter Coder codelab)
- Factory partition, QR codes, beta bundle flashing: [MANUFACTURING.md](MANUFACTURING.md) and [`tools/mfg/`](../tools/mfg/)

## OTA images (mandatory for all SKUs)

Every firmware app in this monorepo must be Matter OTA-ready:

| Requirement | Where |
|-------------|--------|
| Dual OTA slots (`ota_0` / `ota_1`) | App `partitions.csv` |
| `CONFIG_ENABLE_OTA_REQUESTOR=y` | App `sdkconfig.defaults` (also in [matter-base](sdkconfig.defaults.matter-base)) |
| `CONFIG_CHIP_OTA_IMAGE_BUILD=y` | App `sdkconfig.defaults` |
| `CONFIG_DEVICE_SOFTWARE_VERSION_NUMBER` | App `sdkconfig.defaults` — must match `PROJECT_VER_NUMBER` |
| `PROJECT_VER` / `PROJECT_VER_NUMBER` | App `CMakeLists.txt` — `PROJECT_VER` is **`MAJOR.MINOR.PATCH`** |
| `cosmos_matter_ota_configure()` | After `esp_matter::start()` in `main.cpp` |

With those set, `idf.py build` emits `<app-name>-ota.bin` under `build/`.

**Version strategy:** flash at software version number *N*, OTA at *N+1* (monotonic `PROJECT_VER_NUMBER`). Human string bumps (`0.1.0` → `0.1.1` / `0.2.0` / `1.0.0`): [RELEASING.md](RELEASING.md). Field: [cosmos-ha-field](https://github.com/CosmosKiller/cosmos-ha-field).

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
