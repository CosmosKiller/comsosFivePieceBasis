# Hardware reference

Per-device GPIO, carrier-board guidance, and ECAD prompts. Update this file when pinouts or the BOM change; keep app `To-Do.MD` notes in sync until retired (see [POLISH_PLAN.md](POLISH_PLAN.md)).

Firmware is the source of truth for GPIO numbers — carrier boards must match the tables below.

---

## Cosmos carrier design rules

Shared defaults for **low-voltage, sensor-class, 2-layer** carriers across all SKUs (XIAO module + passives + sensor/actuator + battery sense).

### Electrical

| Rule | Value / note |
|------|----------------|
| Supply domain | **1S Li-ion** (3.0–4.2 V) or regulated **3.3 V** to XIAO `3V3` / `VIN` per Seeed guidance; firmware assumes **1S** thresholds (empty 3.0 V, full 4.2 V) |
| Max board voltage | **≤ 12 V** at any net (low-voltage hobby/prototype class) |
| Logic | **3.3 V** CMOS only on XIAO GPIO — no 5 V on module pins |
| Battery sense | Resistive divider **2:1** (e.g. 100 kΩ / 100 kΩ) to ADC pin; **`divider_ratio = 2.0`** in `cosmos_battery` |
| ADC filter | **100 nF** ceramic from sense node (mid-divider tap) to **GND**, close to module pin |
| Digital inputs | Match firmware pull: contact sensor uses **pull-down** (reed open = high-Z, closed = tied high) |
| Factory reset | **GPIO9** (`BOOT` on XIAO): long-press ≥ 5 s (`CONFIG_BUTTON_LONG_PRESS_TIME_MS=5000`); expose accessible tact switch |
| Decoupling | **100 nF** on each LED branch / noisy output near load; module relies on XIAO on-board decoupling |

### RF / layout (Wi‑Fi SKUs: C6, C5)

| Rule | Value / note |
|------|----------------|
| Antenna keep-out | **No copper, ground fill, or components** under the XIAO PCB antenna area (module end opposite USB) |
| Ground | Solid **GND** pour on bottom layer; stitch vias near module ground pads |
| USB | If USB-C is broken out, follow Seeed/XIAO keep-out and differential routing guidelines (optional on carrier — programming can use edge USB on module) |

### PCB fabrication

| Parameter | Default |
|-----------|---------|
| Layers | **2** |
| Thickness | **1.6 mm** |
| Copper | **1 oz** |
| Min trace/space | **6 mil / 6 mil** (JLCPCB 2-layer capability) |
| Via | **0.3 mm drill / 0.6 mm pad** (or fab default) |
| Silkscreen | Product name, `3V3`, `GND`, `BAT+`, revision |
| Test | **BAT sense**, **3V3**, **GND** pads or test points for bring-up |

### Design workflow (Flux / KiCad)

1. Place **XIAO footprint** first; lock antenna keep-out.
2. Route **power** (cell → holder → optional protection → `VIN` or `3V3`).
3. Route **battery divider + ADC** and **sensor input** before auto-router.
4. Place **LEDs / buzzer drivers** on designated GPIOs (do not reassign without firmware change).
5. Run DRC; export **Gerber + BOM + pick-and-place** for prototype order.

### ECAD / schematics (Phase 6 tracking)

Firmware GPIO + Flux prompts in this file are the **source of truth** until Gerbers land.

| SKU | Flux / schematic status | Link |
|-----|-------------------------|------|
| iotDoorSensor (1) | Carrier in progress (Flux.ai) | *Add project URL when shared* |
| iotDualModeBtn (2) | Prompt + BOM ready — layout next | *Add project URL when shared* |
| iotEnvironmentalSensor (3) | Deferred (C5 + display) | — |
| iotBedsideLamp (4) | Prompt + BOM ready (Ø50 mm) | *Add project URL when shared* |
| iotDoorIntercom (5) | Prompt + BOM ready (outdoor 60×100) | *Add project URL when shared* |

When a Flux or KiCad project is public (or in a private hardware repo), paste the URL in the table above and optionally add a `hardware/` submodule or sibling repo note here.

---

## iotDoorSensor

**Firmware app:** `iotDoorSensor/`  
**Module:** [Seeed XIAO ESP32-C6](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)  
**Matter (test):** VID **65522** (`0xFFF2`), PID **32769** (`0x8001`)  
**Role:** Matter door/window contact sensor, status LEDs, optional panic/alarm outputs, battery reporting.

### GPIO map (must match firmware)

| XIAO pin | ESP GPIO | Firmware | Function |
|----------|----------|----------|----------|
| D9 | GPIO20 | `SENSOR_PIN` | Reed / contact input (pull-down in SW) |
| D3 | GPIO21 | `STATE_LED_PIN` | Status LED (event aggregator) |
| D4 | GPIO22 | `CONFIRM_LED_PIN` | Arm / confirm indicator |
| D5 | GPIO23 | `ALARM_LED_PIN` | Alarm / panic indicator |
| D0 / A0 | GPIO0 | `CONFIG_COSMOS_BATTERY_ADC_GPIO` | Battery voltage sense (ADC1) |
| BOOT | GPIO9 | `FACTORY_RESET_BUTTON_PIN` | Factory reset (long-press) |

Unused in current firmware (available for carrier features): D1, D2, D6–D8, D10.

**Sensor logic:** GPIO20 with **internal pull-down** — configure reed switch between **3.3 V** and **D9** (closed = high = triggered). Debouncing is software/GPIO ISR.

**Battery sense:** Tap divider at **D0/A0**; firmware scales by **2.0×** to infer cell voltage.

**Indicators:** Active-high LED drive from GPIO21–23. Hardware may wire **piezo buzzer** in parallel with red/green LED channels (see BOM) — same GPIO drives both; use series resistor + transistor if current exceeds GPIO limit.

### Flux.ai project prompt

Copy into Flux when starting the carrier board (adjust board size, connector part numbers, and cell holder to taste):

> **Status:** Carrier PCB in progress in Flux.ai (Jul 2026) for user-test builds; prompt below matches validated bench prototype GPIO map.

```text
Design a 2-layer carrier PCB for the "Cosmos iotDoorSensor" — a compact Wi-Fi Matter door/window contact sensor.

Core module:
- Seeed XIAO ESP32-C6 (castellated module), mounted on the edge with USB accessible for flashing.
- Keep the antenna area at the module end clear: no copper or components under the on-module PCB antenna.

Power:
- Single-cell Li-ion (1S, 3.7 V nominal, 4.2 V max) with a through-hole or SMD battery holder (e.g. AA/14500 or 102050 pouch with JST-PH 2.0).
- Optional: reverse-polarity protection (Schottky or P-FET) and 100 nF on VBAT.
- Connect battery positive to XIAO VIN (or 3V3 path per Seeed recommendations for battery-powered use).
- Battery monitor: 100 kΩ + 100 kΩ divider from BAT+ to GND; mid tap to XIAO D0 (A0 / GPIO0). 100 nF from tap to GND at the module pin.

Digital inputs:
- Reed switch (normally-open magnetic contact) from 3.3 V to XIAO D9 (GPIO20). Software uses pull-down — open = low, closed = high.
- Tactile push button from XIAO BOOT (GPIO9) to GND for Matter factory reset (long press 5 s). Use a separate user-accessible button, not only the tiny module boot switch.

Digital outputs (3.3 V, active high):
- D3 GPIO21 → green LED + 330 Ω series resistor to GND.
- D4 GPIO22 → yellow or blue "confirm" LED + 330 Ω.
- D5 GPIO23 → red "alarm" LED + 330 Ω.
- Optional: 3–5 V piezo buzzer on red and/or green channel via NPN transistor (e.g. S8050), base resistor ~1 kΩ, flyback diode across buzzer if inductive load.

Layout:
- 2 layers, 1.6 mm FR4, 1 oz copper.
- Rough board size 45–55 mm × 25–35 mm (wall-mount friendly); 3× M2 mounting holes.
- Label silkscreen: BAT+, GND, 3V3, D9 SENSE, revision.
- Solid ground pour on bottom; do not place ground fill under XIAO Wi-Fi antenna.
- Include test pads for BAT+, 3V3, GND, and ADC sense.

Do not assign or reroute GPIOs differently from the table above. Target low-cost JLCPCB assembly; prefer 0603 passives.
```

### Bill of materials (prototype)

| Ref | Qty | Description | Notes |
|-----|-----|-------------|--------|
| U1 | 1 | [Seeed XIAO ESP32-C6](https://www.seeedstudio.com/XIAO-ESP32C6-p-5914.html) | Matter + Wi-Fi MCU module |
| SW1 | 1 | Reed switch, normally open (magnetic contact) | Door/window sense; e.g. GPS-14 or similar |
| SW2 | 1 | Tact switch, through-hole or SMD | Factory reset on **GPIO9** / BOOT |
| BAT1 | 1 | 1S Li-ion cell + holder or JST-PH 2-pin pouch | Match product enclosure; 3.7 V nominal |
| R1, R2 | 2 | 100 kΩ, 0603, 1% | Battery voltage divider |
| R3–R5 | 3 | 330 Ω, 0603 | LED current limit (~3 mA at 3.3 V) |
| C1 | 1 | 100 nF, 0603, X7R | ADC filter at D0 |
| C2 | 1 | 100 nF, 0603, X7R | Optional VBAT / VIN decoupling |
| D1 | 1 | Green LED, 0603 | Status (`STATE_LED`) |
| D2 | 1 | Blue or yellow LED, 0603 | Confirm (`CONFIRM_LED`) |
| D3 | 1 | Red LED, 0603 | Alarm (`ALARM_LED`) |
| Q1, Q2 | 0–2 | NPN SOT-23 (e.g. S8050) | Only if buzzers need more current than GPIO |
| BZ1, BZ2 | 0–2 | 3–5 V active buzzer, SMD or wired | Optional; parallel with D1/D2 per product industrial design |
| R6, R7 | 0–2 | 1 kΩ, 0603 | NPN base resistors if buzzers used |
| — | — | M2 standoffs / screws | Enclosure-dependent |
| — | — | Enclosure, magnet (for reed) | Mechanical; not on PCB BOM |

**Bring-up checklist** — prototype validated 2026-07 (XIAO ESP32-C6 bench carrier; contact input exercised with a **latching toggle** in place of reed for Boolean State testing).

- [x] Divider ratio verified (100 kΩ / 100 kΩ → plausible cell % in HA; fine-tune divider/thresholds after MVP soak if needed)
- [x] Contact input toggles Matter Boolean State (endpoint 1) — latching switch stand-in for reed; replace with reed + magnet on production carrier
- [x] Long-press factory reset clears fabric (GPIO9)
- [x] Battery percent updates in Matter Power Source cluster (endpoint 3) and visible in Home Assistant
- [x] LEDs match `evt_service` / panic tasks on GPIO21–23
- [x] HA low-battery package — [`home-assistant/packages/cosmos_door_sensor.yaml`](../home-assistant/packages/cosmos_door_sensor.yaml) installed and notifying; fleet/OTA in [cosmos-ha-field](https://github.com/CosmosKiller/cosmos-ha-field)

### Firmware modules

Matter, contact sensor driver, event service, panic/alarm outputs, OTA via `cosmos_matter_ota`, battery via `cosmos_battery`.

---

## iotBedsideLamp (SKU 4)

**MVP board:** [ESP32-C6-DevKitC-1](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/hw-reference/esp32c6/user-guide-devkitm-1.html) (bench bring-up)  
**Target carrier:** circular PCB + [Seeed XIAO ESP32-C6](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)  
**Matter role:** Extended color light (On/Off, brightness, color).  
**PID (test):** `0x8004` — see [MANUFACTURING.md](MANUFACTURING.md).

### Product decisions (locked for v1 carrier)

| Item | Choice |
|------|--------|
| Form | **Circular PCB**, LED ring onboard |
| LEDs | **10× WS2812** (or SK6812), single data line |
| Ring / board OD | **≈ Ø50 mm** (10× 5050 LEDs need ~Ø45–55 mm; treat “5 mm” as typo for 50 mm — change OD in Flux if you want larger) |
| Buttons | **2 only** — user + factory reset (no third) |
| Battery | **1S** pouch via **JST-PH 2.0**; size free; target **≥ 3 h at full brightness** → recommend **≥ 3000 mAh** (see power budget) |
| USB | USB-C on carrier for power + charge |

### GPIO map (carrier — must match firmware / Kconfig)

| XIAO pin | ESP GPIO | Firmware / Kconfig | Function |
|----------|----------|--------------------|----------|
| D8 | GPIO19 | `CONFIG_BEDSIDE_LAMP_LED_GPIO` | WS2812 data (RMT) — **10 LEDs** |
| D9 | GPIO20 | `CONFIG_BEDSIDE_LAMP_USER_BUTTON_GPIO` | User tact (toggle / double / long → presets) |
| BOOT | GPIO9 | `FACTORY_RESET_BUTTON_PIN` | Factory reset tact (long ≥ 5 s) |
| D0 / A0 | GPIO0 | `CONFIG_COSMOS_BATTERY_ADC_GPIO` | Battery sense (divider mid-tap) |

**MVP DevKit (today):** LED data **GPIO8**, user **GPIO20**, reset **GPIO9**. Battery ADC on **GPIO0** is enabled in firmware (floating / unused on DevKit until carrier divider is wired). Carrier needs LED GPIO19 + LED count 10 override.

Do **not** reuse the user button for factory reset.

### Power budget (full white, rough)

| Load | Estimate |
|------|----------|
| 10× WS2812 @ full white | ~500–600 mA @ 5 V |
| ESP32-C6 + Wi‑Fi | ~80–150 mA |
| Total | ~0.7–0.8 A |
| **3 h runtime** | **≥ 2.4 Ah** usable → specify **1S 3000–3500 mAh** pouch |

**Rails:** USB-C 5 V → charge IC → 1S cell (JST). Cell → XIAO `BAT`/`3V3` path per Seeed. **5 V boost** (or USB 5 V) for WS2812 VDD; data from GPIO19 (3.3 V levels usually OK into WS2812 at 5 V VDD). Shared **`cosmos_battery`** divider **2:1** on GPIO0.

### MVP — DevKitC-1 bench setup

| Signal | DevKit (now) | Target carrier |
|--------|--------------|----------------|
| Addressable LED | GPIO8 — 1× WS2812 | GPIO19 — **10×** ring |
| User button | GPIO20 | Dedicated tact |
| Factory reset | GPIO9 (BOOT) | Dedicated tact |
| Power | USB | USB-C + 1S JST |

Build with `ESP_MATTER_DEVICE_PATH=$ESP_MATTER_PATH/device_hal/device/esp32c6_devkit_c`.

### Flux.ai project prompt

```text
Design a circular 2-layer carrier PCB for the "Cosmos iotBedsideLamp" — a portable Matter RGB bedside lamp.

Form factor:
- Circular PCB, outer diameter approximately 50–55 mm (fits a 10-LED ring of 5050 WS2812 with ~1–2 mm edge clearance).
- Center or offset pocket for Seeed XIAO ESP32-C6 (castellated), USB-C accessible from the edge or a cutout.
- Keep XIAO PCB antenna clear: no copper/components under the antenna end.

Core module:
- Seeed XIAO ESP32-C6.

LEDs:
- 10× WS2812B (or SK6812) arranged in a ring near the board perimeter, equal angular spacing.
- Single data daisy-chain: DIN of LED1 from XIAO D8 (GPIO19) via 33–100 Ω series resistor close to first LED; optional 100 nF per LED on VDD locally.
- LED VDD = 5 V rail (from USB when plugged, or from a 5 V boost when on battery). Common GND with MCU.
- Leave a silk ring / keep-out for a diffuser dome above the LEDs.

Power:
- USB-C receptacle on the carrier (5 V). Charge a 1S Li-ion pouch (JST-PH 2.0, 2-pin) with a protected charger (e.g. TP4056 + DW01/FS8205 or integrated module). Target cell ≥ 3000 mAh for ≥ 3 h full-brightness runtime.
- Battery sense: 100 kΩ + 100 kΩ divider from BAT+ to GND; mid tap to XIAO D0/A0 (GPIO0); 100 nF at the ADC pin.
- Provide a 5 V boost from BAT for the LED ring when USB is unplugged (e.g. MT3608 or similar, ≥ 1.5 A capability). When USB is present, LEDs may run from USB 5 V.
- Do not put 5 V on XIAO GPIO pins.

Buttons (exactly two):
- User tact: XIAO D9 (GPIO20) to GND (firmware pull-up / button library). Accessible on top or side of enclosure.
- Factory-reset tact: XIAO BOOT (GPIO9) to GND, long-press ≥ 5 s. Separate from user button; recess or side placement to avoid accidents.

Layout:
- 2 layers, 1.6 mm FR4, 1 oz copper.
- Solid GND pour; stitch vias; antenna keep-out on XIAO.
- Silkscreen: BAT+, GND, 5V, LED DIN, REV, product name.
- Test pads: BAT+, 5V, 3V3, GND, ADC sense.
- Prefer JLCPCB assembly; 0603 passives; 5050 LEDs on top.

Do not reassign GPIOs from: LED=GPIO19, user=GPIO20, reset=GPIO9, battery ADC=GPIO0.
```

### Bill of materials (prototype carrier)

| Ref | Qty | Description | Notes |
|-----|-----|-------------|--------|
| U1 | 1 | [Seeed XIAO ESP32-C6](https://www.seeedstudio.com/XIAO-ESP32C6-p-5914.html) | Matter MCU |
| LED1–LED10 | 10 | WS2812B / SK6812, 5050 | Ring, one data line |
| R_LED | 1 | 33–100 Ω, 0603 | Series on DIN |
| SW1 | 1 | Tact switch | User — GPIO20 |
| SW2 | 1 | Tact switch | Factory reset — GPIO9 |
| J1 | 1 | USB-C receptacle (power) | Charge + 5 V |
| J2 | 1 | JST-PH 2.0, 2-pin | 1S pouch ≥ 3000 mAh |
| U2 | 1 | 1S Li-ion charger + protection | e.g. TP4056 + DW01 path |
| U3 | 1 | 5 V boost ≥ 1.5 A | LED rail from battery |
| R1, R2 | 2 | 100 kΩ, 0603, 1% | Battery divider |
| C1 | 1 | 100 nF, 0603 | ADC filter |
| C_LED | 10 | 100 nF, 0603 | Local LED decoupling (optional but recommended) |
| BAT1 | 1 | 1S Li-ion pouch ≥ 3000 mAh | Off-board, JST; size OK |
| — | — | Diffuser / enclosure | Mechanical |

### Bring-up checklist (carrier)

- [ ] 10 LEDs light as one Matter extended-color light (firmware LED count = 10)
- [ ] User button: click / double / long preset; reset button does **not** toggle lamp
- [ ] Factory reset long-press clears fabric
- [ ] USB charges cell; boost supplies LEDs on battery; ≥ 3 h full white soak
- [ ] Battery % in Matter / HA via `cosmos_battery`

### Firmware modules

Matter extended color light + Power Source, `lamp_task`, `led_effects_task`, `user_button_task`, `cosmos_battery` (GPIO0), OTA via `cosmos_matter_ota`, factory reset via `cosmos_matter_common`.

---

## iotDualModeBtn (SKU 2)

**Firmware app:** `iotDualModeBtn/`  
**Module:** [Seeed XIAO ESP32-C6](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)  
**Matter (test):** VID **65522** (`0xFFF2`), PID **32770** (`0x8002`)  
**Role:** Matter generic switch (press / multi-press / long-press events); sleepy Wi‑Fi/Thread capable; battery reporting.

Follow [Cosmos carrier design rules](#cosmos-carrier-design-rules).

### Product decisions (locked for v1 carrier)

| Item | Choice |
|------|--------|
| Form | Compact handheld / wall puck (similar class to door sensor) |
| Buttons | **2** — large action tact + recessed factory-reset tact |
| Indicators | **2× discrete LEDs** (firmware today — not a single RGB package) |
| Power | **1S** Li-ion via holder or **JST-PH 2.0**; battery sense on carrier |
| Environment | Indoor (default); conformal coat optional |

> Early notes mentioned “RGB LED”; shipping firmware drives **two** GPIOs (`SINGLE_PRESS` / `MULTI_PRESS`). PCB matches firmware.

### GPIO map (must match firmware)

| XIAO pin | ESP GPIO | Firmware | Function / wiring |
|----------|----------|----------|-------------------|
| D9 | GPIO20 | `BUTTON_GPIO_PIN` | Action tact to **GND** (iot_button pull-up; press = LOW) |
| BOOT | GPIO9 | `FACTORY_RESET_BUTTON_PIN` | Reset tact to **GND**; long ≥ 5 s — **not** the action button |
| D3 | GPIO21 | `SINGLE_PRESS_LED_PIN` | Active-high LED + 330 Ω (single-press feedback) |
| D8 | GPIO19 | `MULTI_PRESS_LED_PIN` | Active-high LED + 330 Ω (multi-press feedback) |
| D0 / A0 | GPIO0 | `CONFIG_COSMOS_BATTERY_ADC_GPIO` | Battery divider mid-tap (2:1) |

Unused on carrier for v1 (available): D1, D2, D4–D7, D10.

### Flux.ai project prompt

```text
Design a 2-layer carrier PCB for the "Cosmos iotDualModeBtn" — a compact Matter generic-switch remote / wall button (press, multi-press, long-press).

Core module:
- Seeed XIAO ESP32-C6 (castellated), USB accessible for flashing.
- Keep the on-module PCB antenna clear: no copper or components under the antenna end.

Power:
- Single-cell Li-ion (1S, 3.7 V nominal) with through-hole/SMD holder or JST-PH 2.0 pouch connector.
- Optional reverse-polarity protection and 100 nF on VBAT.
- Connect battery to XIAO VIN / BAT path per Seeed battery guidance (sleepy device — minimize quiescent load).
- Battery monitor: 100 kΩ + 100 kΩ divider from BAT+ to GND; mid tap to XIAO D0 (A0 / GPIO0). 100 nF from tap to GND at the module pin.

Buttons (exactly two — do not combine):
- Primary action tact: between XIAO D9 (GPIO20) and GND. Large, easy to press. Firmware uses pull-up.
- Factory-reset tact: between XIAO BOOT (GPIO9) and GND. Recessed or side-mounted; long press ≥ 5 s. Separate from the action button.

LEDs (two discrete, active high):
- D3 GPIO21 → LED (e.g. green) + 330 Ω to GND — single-press indicator.
- D8 GPIO19 → LED (e.g. blue or yellow) + 330 Ω to GND — multi-press indicator.
- Optional: 100 nF near each LED. No RGB package required for v1.

Layout:
- 2 layers, 1.6 mm FR4, 1 oz copper.
- Rough board size 40–55 mm × 25–35 mm (pocket / wall-mount friendly); 2–3× M2 mounting holes.
- Solid GND pour on bottom; antenna keep-out on XIAO.
- Silkscreen: BAT+, GND, 3V3, ACTION, RESET, REV, product name.
- Test pads: BAT+, 3V3, GND, ADC sense, GPIO20.
- Prefer JLCPCB assembly; 0603 passives.

GPIO lock (do not reassign):
- Action GPIO20, Reset GPIO9, LED single GPIO21, LED multi GPIO19, Battery ADC GPIO0.
```

### Bill of materials (prototype)

| Ref | Qty | Description | Notes |
|-----|-----|-------------|--------|
| U1 | 1 | [Seeed XIAO ESP32-C6](https://www.seeedstudio.com/XIAO-ESP32C6-p-5914.html) | Matter MCU |
| SW1 | 1 | Tact switch (large / soft) | Action — GPIO20 to GND |
| SW2 | 1 | Tact switch (recessed) | Factory reset — GPIO9 to GND |
| D1 | 1 | Green LED, 0603 | Single-press (`GPIO21`) |
| D2 | 1 | Blue or yellow LED, 0603 | Multi-press (`GPIO19`) |
| R3, R4 | 2 | 330 Ω, 0603 | LED current limit |
| BAT1 | 1 | 1S Li-ion + holder or JST-PH 2-pin pouch | Match enclosure |
| R1, R2 | 2 | 100 kΩ, 0603, 1% | Battery divider → GPIO0 |
| C1 | 1 | 100 nF, 0603, X7R | ADC filter |
| C2 | 1 | 100 nF, 0603 | Optional VBAT decoupling |
| — | — | Enclosure / wall plate | Mechanical |

### Bring-up checklist

- [ ] Single / double / multi press → Matter Switch events in HA (`event.*`)
- [ ] LEDs flash per press type (GPIO21 / GPIO19)
- [ ] Factory-reset long-press clears fabric (GPIO9 only)
- [ ] Battery % via `cosmos_battery` / Power Source in HA
- [ ] OTA image builds (`CHIP_OTA_IMAGE_BUILD`)

### Firmware modules

Matter generic switch, `iot_button_task`, `cosmos_battery` (GPIO0), OTA via `cosmos_matter_ota`, factory reset via `cosmos_matter_common`.

---

## iotEnvironmentalSensor

**Board (target):** [Seeed XIAO ESP32-C5](https://wiki.seeedstudio.com/xiao_esp32c5_getting_started/)

Follow [Cosmos carrier design rules](#cosmos-carrier-design-rules); battery sense on **A6 / GPIO6** (not A0).

| Signal | Role |
|--------|------|
| D0/GPIO1, D1/GPIO0, D2/GPIO25, BOOT/GPIO28 | Rotary encoder, reset |
| GPIO23 / GPIO24 | I2C SDA / SCL (BME680) |
| SPI (GPIO8–12, etc.) | ST7789 display (planned) |
| A6/GPIO6 | Battery monitoring (ADC1_CH6) |
| Optional | RGB LED outputs |

**Modules:** Matter, BME680, OTA via `cosmos_matter_ota` — battery via `cosmos_battery`; display, custom QR open.

> **Note:** Target is **esp32c5** (`sdkconfig.defaults`, CMake); run `idf.py set-target esp32c5` locally to regenerate `sdkconfig`.

*Flux prompt and BOM — TBD (higher complexity: I2C sensor + display + encoder).*

---

## iotDoorIntercom (SKU 5)

**Board:** [Seeed XIAO ESP32-S3 Sense](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/) (OV2640 camera on Sense expansion)  
**Matter role:** Generic Switch doorbell + PIR + tamper + OnOff stream gate + OnOff siren clear.  
**PID (test):** `0x8005` — see [MANUFACTURING.md](MANUFACTURING.md).  
**Stream:** HTTPS MJPEG `GET https://<device-ip>/stream` (port 443, Beta self-signed cert). Not Matter Camera / WebRTC yet.

### Product decisions (locked for v1 carrier)

| Item | Choice |
|------|--------|
| Board size | **≈ 60 × 100 mm** (doorbell / wall-mount) |
| Environment | **Outdoor** enclosure (gasketed camera window, conformal coat PCB) |
| Power | **USB-C** + **JST-PH 2.0** 1S pouch; battery sense on carrier |
| Front controls | Doorbell tact + **AM312** mini PIR (3.3 V) on carrier |
| Tamper | **Leaf spring → chassis GND** when seated; **two gold pads on PCB back** closed by **pogo pins** in the housing (series or parallel path — both must make contact when mounted) |
| Siren | Piezo + LED on **GPIO4**, NPN drive (same idea as door-sensor alarm) |
| Door lock | **None** for v1 (GPIO5 used for battery ADC) |
| Camera | Keep Sense camera FPC / expansion; do not reassign DVP pins |

### GPIO map (must match firmware)

| XIAO pin | ESP GPIO | Firmware | Function / wiring |
|----------|----------|----------|-------------------|
| D0 | GPIO1 | `DOORBELL_PIN` | Tact to **3.3 V** (SW pull-down); press = HIGH |
| D1 | GPIO2 | `PIR_PIN` | AM312 **OUT** (active HIGH); module VCC=3.3 V, GND |
| D2 | GPIO3 | `TAMPER_PIN` | SW pull-up; **LOW when seated** (path to GND via leaf spring + back pads/pogos) |
| D3 | GPIO4 | `ALARM_LED_PIN` | NPN → red LED + piezo (active HIGH blink) |
| D4 | GPIO5 | `CONFIG_COSMOS_BATTERY_ADC_GPIO` | Mid-tap of 100 k / 100 k divider (`cosmos_battery` enabled) |
| BOOT | GPIO0 | `FACTORY_RESET_BUTTON_PIN` | Tact to **GND**; long ≥ 5 s (not the doorbell) |
| — | GPIO21 | `LED_PIN` | On-module user LED — stream status (no carrier LED required) |
| Sense DVP | (fixed) | `cam_task.h` | Camera — do not steal these GPIOs |

**Unused for v1 (do not load door-lock features):** former door-lock plan on GPIO5 — superseded by battery ADC.

### Tamper electromechanical detail

```text
Seated (OK):   TAMPER_PIN ── leaf spring ── chassis GND
                 also: back pad A ── pogo ── case jumper ── pogo ── back pad B
                 (implement as one series path to GND so either leaf or pogo pair can be primary;
                  recommended: leaf spring is the chassis NC; pogo pads are a second NC in series
                  so opening the case OR lifting the board trips tamper)

Open (alarm):  path broken → internal pull-up → HIGH → latched panic_alarm + Matter contact open
```

Firmware **does not** clear the siren on remount — HA turns Off the Matter siren OnOff.

### Flux.ai project prompt

```text
Design a 2-layer outdoor doorbell / door-station carrier PCB for "Cosmos iotDoorIntercom".

Form factor:
- Board outline approximately 60 mm × 100 mm, vertical wall-mount (doorbell aspect).
- 3–4× M2 or M3 mounting holes aligned with a gasketed outdoor enclosure.
- Camera opening / keep-out matching Seeed XIAO ESP32-S3 Sense camera module (OV2640) so the lens looks through a clear / IR-capable window.
- USB-C accessible from bottom or side without opening the weather seal if possible (or under a sealed plug).

Core module:
- Seeed XIAO ESP32-S3 Sense (castellated + camera expansion). Mount so the Sense camera faces outward.
- Keep Wi-Fi / antenna clearances per Seeed (no copper under antenna region). Prefer external u.FL antenna option only if enclosure blocks the PCB antenna — default: module antenna with plastic RF window.

Power (USB-C + pouch):
- USB-C 5 V input on the carrier.
- 1S Li-ion pouch on JST-PH 2.0 (2-pin). Include charger + protection (TP4056 + DW01/FS8205 class or better outdoor-rated module).
- Feed XIAO battery / 5 V / 3V3 per Seeed battery + USB guidance. Camera + Wi-Fi are power-hungry — size traces and charger for ≥ 1 A peaks.
- Battery monitor: 100 kΩ + 100 kΩ divider BAT+ to GND; mid tap to XIAO D4 (GPIO5). 100 nF at ADC pin.
- Conformal-coating friendly: no flux traps; prefer taller connectors only where needed.

Front / user I/O:
- Large doorbell tact (or off-board tact on short wires to pads): between 3.3 V and XIAO D0 (GPIO1). Firmware pull-down.
- AM312 (or equivalent 3.3 V mini PIR) with OUT to XIAO D1 (GPIO2), VCC=3.3 V, GND. Place PIR behind a Fresnel window at the top of the faceplate; keep LED/siren optical isolation from PIR.
- Factory-reset tact to XIAO BOOT (GPIO0) to GND, recessed, separate from doorbell.

Tamper (anti-theft / case open):
- Leaf spring or spring finger that contacts chassis / backplate GND when the unit is screwed to the wall.
- On the PCB BACK: two exposed gold pads (or pogo landing pads), spaced for pogo pins in the enclosure. When the housing is closed and mounted, pogos short those pads into the tamper-to-GND path.
- Net TAMPER to XIAO D2 (GPIO3). Firmware internal pull-up: seated = LOW, open = HIGH.

Siren (panic alarm):
- XIAO D3 (GPIO4) → 1 kΩ → NPN base (S8050) → drive in parallel: (a) red LED + 330 Ω to 3.3 V or from collector topology active-high as convenient; (b) 3–5 V active piezo with diode. Match door-sensor style: GPIO high turns siren/LED on (blinked in firmware).
- Place piezo so it vents through a grille; keep water away (IP membrane or rear chamber).

Status:
- Rely on XIAO GPIO21 user LED for stream status; optional extra status LED not required.

Outdoor / reliability:
- Design for IP54+ enclosure (gaskets, camera window seal, drain path). Document conformal coat after bring-up.
- Silkscreen: BAT+, GND, 3V3, DOORBELL, PIR, TAMPER, SIREN, ADC, REV.
- Test pads: BAT+, 3V3, GND, GPIO1/2/3/4/5.
- 2 layers, 1.6 mm FR4, 1 oz, JLCPCB-friendly, 0603 passives.

GPIO lock (do not reassign):
- Doorbell GPIO1, PIR GPIO2, Tamper GPIO3, Siren GPIO4, Battery ADC GPIO5, Reset GPIO0.
- Do not use GPIO5 for door lock. Do not steal camera DVP pins.
```

### Bill of materials (prototype carrier)

| Ref | Qty | Description | Notes |
|-----|-----|-------------|--------|
| U1 | 1 | [Seeed XIAO ESP32-S3 Sense](https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html) | Camera + Wi‑Fi MCU |
| SW1 | 1 | Doorbell tact (large / weatherized) | To 3.3 V / GPIO1 |
| SW2 | 1 | Tact switch, recessed | Factory reset GPIO0 |
| U2 | 1 | AM312 (or 3.3 V mini PIR) | OUT → GPIO2 |
| SW3 | 1 | Leaf spring / chassis contact | Tamper to GND when seated |
| PAD1, PAD2 | 2 | Gold / pogo landing pads (back) | Case pogo short when closed |
| — | 2 | Pogo pins (in enclosure) | Mechanical; not on PCB BOM |
| Q1 | 1 | NPN SOT-23 (S8050) | Siren / LED drive |
| R_B | 1 | 1 kΩ, 0603 | NPN base |
| D_ALM | 1 | Red LED, 0603 | Alarm visual |
| R_LED | 1 | 330 Ω, 0603 | LED limit |
| BZ1 | 1 | 3–5 V active piezo | Siren; flyback diode if needed |
| J1 | 1 | USB-C receptacle | Power + charge |
| J2 | 1 | JST-PH 2.0, 2-pin | 1S pouch |
| U3 | 1 | 1S charger + protection | TP4056-class or better |
| R1, R2 | 2 | 100 kΩ, 0603, 1% | Battery divider → GPIO5 |
| C1 | 1 | 100 nF, 0603 | ADC filter |
| C2 | 1 | 100 nF–10 µF | VIN / BAT decoupling as needed |
| BAT1 | 1 | 1S Li-ion pouch | Size for outdoor runtime; JST |
| — | — | Outdoor enclosure, gaskets, camera window, Fresnel for PIR | Mechanical |
| — | — | Conformal coat | After electrical bring-up |

### Bring-up checklist

- [ ] Doorbell → Matter `event.*` + stream gate / HA notify
- [ ] AM312 motion → occupancy + auto stream
- [ ] Open case / lift board → tamper binary_sensor on + GPIO4 siren latches; remount does **not** silence; HA siren OnOff Off stops siren
- [ ] Back pogo pads + leaf spring both exercise tamper path
- [ ] USB-C charges pouch; divider on GPIO5 reads plausible % in Matter / HA
- [ ] HTTPS MJPEG through camera window; Wi‑Fi RSSI acceptable in metal/plastic enclosure
- [ ] Factory reset on BOOT only (not doorbell)
- [ ] HA package + Lovelace — [`home-assistant/`](../home-assistant/)

### Firmware modules

Matter (stream OnOff, PIR, doorbell `generic_switch`, tamper `contact_sensor`, siren OnOff, Power Source), `cam_task`, `http_stream_task`, `evt_service_task`, `door_intercom_task`, `security_module_task`, `panic_alarm_task`, `cosmos_battery` (GPIO5), OTA via `cosmos_matter_ota`, factory reset via `cosmos_matter_common`.

**HTTPS /stream (Beta):** certs in `iotDoorIntercom/main/certs/` (regen `tools/certs/gen_door_intercom_https.sh`). HA MJPEG: UI integration, verify SSL off on LAN.

> **Target:** `esp32s3` — `idf.py set-target esp32s3`. Octal PSRAM required for camera framebuffers.

*Matter Camera + WebRTC deferred (after toolchain bump).*
