# Manufacturing — factory data, QR codes, and beta bundles

Per-device Matter credentials live in a dedicated `**fctry**` flash partition (`0x3E0000`, 24 KiB). Application firmware is **identical** for every unit of a SKU; only the factory partition (and optional `esp_secure_cert`) differs per board.

Scripts: `[tools/mfg/](../tools/mfg/)`  
Pi / HA side of beta kits: [cosmos-ha-field](https://github.com/CosmosKiller/cosmos-ha-field)

---

## Prerequisites

```bash
python3 -m pip install esp-matter-mfg-tool

# Your esp-matter clone (not the README placeholder)
export ESP_MATTER_PATH=$HOME/esp/esp-matter
export ESP_MATTER_DEVICE_PATH=$ESP_MATTER_PATH/device_hal/device/esp32c6_devkit_c
export IDF_PATH=/path/to/esp-idf
. $IDF_PATH/export.sh
```

`MATTER_SDK_PATH` defaults to `$ESP_MATTER_PATH/connectedhomeip/connectedhomeip` if unset.

---

## SKU map (Cosmos test vendor)


| SKU                  | Firmware app             | Matter role                   | PID      | IDF target | Mfg script                    |
| -------------------- | ------------------------ | ----------------------------- | -------- | ---------- | ----------------------------- |
| Door / contact sensor | `iotDoorSensor`          | Contact / Boolean State       | `0x8001` | esp32c6    | `gen_door_sensor.sh`          |
| Dual-mode button     | `iotDualModeBtn`         | Switch (press / multi / long) | `0x8002` | esp32c6    | `gen_dual_mode_btn.sh`        |
| Environmental sensor | `iotEnvironmentalSensor` | BME680 environmental          | `0x8003` | esp32c5    | `gen_environmental_sensor.sh` |
| Bedside lamp         | `iotBedsideLamp`         | Extended color light          | `0x8004` | esp32c6    | `gen_bedside_lamp.sh`         |
| Door intercom        | `iotDoorIntercom`        | Doorbell + PIR + MJPEG        | `0x8005` | esp32s3    | `gen_door_intercom.sh`        |


**Vendor ID:** `0xFFF2` (Espressif **Chip-Test-** credentials — closed beta / lab only).

**Serial / label codes:** `DOOR`, `BTN`, `ENV`, `LAMP`, `INTR` (e.g. `BETA007-INTR-001`).

---

## Beta bundle workflow

One beta home kit = **1× Raspberry Pi** (Home Assistant) + **1+ ESP devices** (each with a unique QR).

```text
┌─────────────────────────────────────────────────────────────┐
│  Your bench (once per firmware release)                      │
│  1. idf.py build  →  common .bin per SKU                    │
│  2. gen_*.sh N    →  N unique fctry + QR sets per SKU       │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│  Per physical unit                                             │
│  3. flash_unit.sh  →  app + esp_secure_cert + fctry          │
│  4. Print QR from summary CSV onto device label              │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│  Per beta home                                                 │
│  5. Flash/configure Pi (cosmos-ha-field)                       │
│  6. Tester: HA → Add Matter device → scan each device QR     │
└─────────────────────────────────────────────────────────────┘
```

The **RPi is not provisioned by `esp-matter-mfg-tool`**. It runs HA as the Matter commissioner. Device QRs are commissioned into HA on that Pi.

For **N beta homes × 3 SKUs**, generate **3 batches of N** factory sets (not N×3 manual one-offs):

```bash
./tools/mfg/gen_door_sensor.sh 10
./tools/mfg/gen_dual_mode_btn.sh 10
./tools/mfg/gen_environmental_sensor.sh 10
```

Optional bundle prefix on a single unit:

```bash
BUNDLE_ID=BETA007 ./tools/mfg/gen_door_sensor.sh
```

---

## Step 1 — Build firmware (once per release)

### Door / contact sensor (C6)

```bash
cd iotDoorSensor
idf.py set-target esp32c6
idf.py build
```

### Dual-mode button (C6, Wi‑Fi + Thread — beta default)

Beta bundles use **Wi‑Fi + Thread**, not Thread-only:

```bash
cd iotDualModeBtn
export SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.c6_wifi_thread"
idf.py set-target esp32c6
idf.py build
```

Thread-only remains available via `sdkconfig.defaults.c6_thread` for low-power experiments; do not ship that variant in beta kits without relabeling.

### Environmental sensor (C5, preview target)

```bash
cd iotEnvironmentalSensor
idf.py --preview set-target esp32c5
idf.py --preview build
```

---

## Step 2 — Generate factory data + QR codes

From repo root:

```bash
# One unit (default)
./tools/mfg/gen_door_sensor.sh

# Ten unique units for ten beta homes
./tools/mfg/gen_dual_mode_btn.sh 10

# Custom output directory
./tools/mfg/gen_environmental_sensor.sh 5 out/mfg/batch-2026-07
```

Output lands under `out/mfg/<sku>/fff2_<pid>/`:


| Artifact                             | Purpose                                                                |
| ------------------------------------ | ---------------------------------------------------------------------- |
| `summary-*.csv`                      | **QR** (`qrcode` column), manual code, passcode, discriminator, serial |
| `*-partition.bin` (or `*_fctry.bin`) | Factory NVS partition → flash at `0x3E0000`                            |
| `*_esp_secure_cert.bin`              | DAC in secure cert partition → flash at `0xD000`                       |
| Per-unit UUID subfolder              | One directory per generated device                                     |


Turn QR text into a printable image: [CHIP QR Code generator](https://project-chip.github.io/connectedhomeip/qrcode.html).

---

## Step 3 — Flash each unit

**One board = one SKU = one flash.** Each physical device gets the firmware for its product type plus the factory data generated for *that* unit. Do not run all three `flash_unit.sh` commands on the same board — the factory partition and QR are SKU-specific and unit-specific.

Pick the **UUID folder** for this board (the directory that contains `*-partition.bin` and `*_esp_secure_cert.bin`):

```bash
export ESPPORT=/dev/ttyACM0

# Custom batch outdir (all SKUs under one folder — your beta-batch-24-07-2026 layout):
UNIT_BS=out/mfg/beta-batch-24-07-2026/fff2_8001/<uuid>
./tools/mfg/flash_unit.sh iotDoorSensor "${UNIT_BS}"

UNIT_BTN=out/mfg/beta-batch-24-07-2026/fff2_8002/<uuid>
./tools/mfg/flash_unit.sh iotDualModeBtn "${UNIT_BTN}"

UNIT_ENV=out/mfg/beta-batch-24-07-2026/fff2_8003/<uuid>
./tools/mfg/flash_unit.sh iotEnvironmentalSensor "${UNIT_ENV}"
```

Default output (if you omit the custom outdir) uses `out/mfg/<sku>/fff2_<pid>/<uuid>/` — same **one** UUID level, e.g. `out/mfg/door_sensor/fff2_8001/<uuid>/`.

The UUID appears twice in filenames (`<uuid>-partition.bin`) but there is **only one** UUID directory — not `<uuid>/<uuid>/`.

For a **beta kit** (1× of each SKU), you flash **three separate boards** — one command per board, each with its own `UNIT_`* path from that SKU’s manufacturing run.

**Critical:** the printed QR must come from the **same** summary row as the `fctry` bin flashed onto **that** board.

Manual flash (equivalent):

```bash
esptool.py --port $ESPPORT write_flash \
  0x0      build/bootloader/bootloader.bin \
  0xC000   build/partition_table/partition-table.bin \
  0xD000   <unit>_esp_secure_cert.bin \
  0x20000  build/<app>.bin \
  0x3E0000 <unit>-partition.bin
```

Partition table: `[partitions.csv](../iotDoorSensor/partitions.csv)` (same layout in all three apps).

---

## Step 4 — Ship beta kit


| Item             | Action                                                                            |
| ---------------- | --------------------------------------------------------------------------------- |
| Raspberry Pi     | HA OS + [cosmos-ha-field](https://github.com/CosmosKiller/cosmos-ha-field) deploy |
| Each ESP device  | Powered, labeled with its QR                                                      |
| HA packages      | Copy `home-assistant/packages/*.yaml` to Pi `/config/packages/`                   |
| Quick-start card | “Power Pi → open HA → Add Matter device → scan each QR”                           |


Keep a **shipment manifest**: bundle ID → serial → SKU → QR → (optional) MAC after first boot.

---

## Test credentials vs production


|               | Beta (today)                       | Production                       |
| ------------- | ---------------------------------- | -------------------------------- |
| VID           | `0xFFF2` (Chip-Test)               | CSA-assigned VID                 |
| DAC / PAI     | Auto-generated or Chip-Test-* PEMs | Unique production DAC per device |
| CD            | Chip-Test-CD-FFF2-*.der            | CSA CD per PID                   |
| Commissioners | HA, chip-tool (test-mode tolerant) | Apple Home, Google Home, etc.    |


**Note:** SKUs `0x8003`–`0x8005` use Chip-Test-CD-FFF2-**8002** + NoPID PAI until CSA CDs exist — fine for closed beta; replace before retail.

---

## Troubleshooting


| Symptom                          | Likely cause                                                                 |
| -------------------------------- | ---------------------------------------------------------------------------- |
| Commissioning fails / wrong PIN  | QR label does not match `fctry` on that board                                |
| PID mismatch error from mfg tool | PAI cert PID must match `-p` (8001 uses PAI-8001; 8002/8003 use PAI-NoPID)   |
| Button not on Wi‑Fi              | Flashed Thread-only build — rebuild with `sdkconfig.defaults.c6_wifi_thread` |
| Env sensor build fails           | Append `--preview` for ESP32-C5                                              |


---

## Related docs

- [BUILD.md](BUILD.md) — toolchain, OTA images, Matter cert codelab link
- [HARDWARE.md](HARDWARE.md) — carrier GPIO / BOM per SKU
- [POLISH_PLAN.md](POLISH_PLAN.md) — roadmap

