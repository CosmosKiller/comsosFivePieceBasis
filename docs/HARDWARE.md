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

---

## iotBasicBinarySensor

**Firmware app:** `iotBasicBinarySensor/`  
**Module:** [Seeed XIAO ESP32-C6](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)  
**Matter (test):** VID **65521** (`0xFFF1`), PID **32768** (`0x8000`)  
**Role:** Matter contact / leak-style binary sensor, status LEDs, optional panic/alarm outputs, battery reporting.

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

```text
Design a 2-layer carrier PCB for the "Cosmos iotBasicBinarySensor" — a compact Wi-Fi Matter door/window contact sensor.

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

**Bring-up checklist**

- [ ] Divider ratio verified (100 kΩ / 100 kΩ → ~half cell voltage at A0)
- [ ] Reed open/closed toggles Matter Boolean State (endpoint 1)
- [ ] Long-press factory reset clears fabric (GPIO9)
- [ ] Battery percent updates in Matter Power Source cluster (endpoint 3)
- [ ] LEDs match `evt_service` / panic tasks on GPIO21–23

### Firmware modules

Matter, binary sensor driver, event service, panic/alarm outputs, OTA via `cosmos_matter_ota`, battery via `cosmos_battery`.

---

## iotDualModeBtn

**Board:** [Seeed XIAO ESP32-C6](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)

Follow [Cosmos carrier design rules](#cosmos-carrier-design-rules) (2-layer, 1S battery sense on **A0 / GPIO0**, factory reset on **GPIO9**).

| Signal | Role |
|--------|------|
| Digital in ×2 | Action button, reset button |
| Digital out ×3 | RGB LED |
| Analog in ×1 | Battery monitoring — **A0 / GPIO0** (ADC1_CH0) |

**Modules:** Matter, button driver — battery via `cosmos_battery`; OTA via `cosmos_matter_ota`.

*Flux prompt and BOM — TBD when this SKU enters hardware layout.*

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
