# I was planning on switching from XIOA ESP32-S3 to this board for SKU. what sounds intersting to me are two things:

- LoRA (To use this as a gateway)

- And the Hardware reference

Per-device GPIO, carrier-board guidance, and ECAD prompts. Update this file when pinouts or the BOM change; keep app `To-Do.MD` notes in sync until retired (see [POLISH_PLAN.md](POLISH_PLAN.md)).

Firmware is the source of truth for GPIO numbers — carrier boards must match the tables below.

---

## Cosmos carrier design rules

Shared defaults for **low-voltage, sensor-class, 2-layer** carriers across all SKUs (XIAO module + passives + sensor/actuator + battery sense).

### Electrical


| Rule              | Value / note                                                                                                                                            |
| ----------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Supply domain     | **1S Li-ion** (3.0–4.2 V) or regulated **3.3 V** to XIAO `3V3` / `VIN` per Seeed guidance; firmware assumes **1S** thresholds (empty 3.0 V, full 4.2 V) |
| Max board voltage | **≤ 12 V** at any net (low-voltage hobby/prototype class)                                                                                               |
| Logic             | **3.3 V** CMOS only on XIAO GPIO — no 5 V on module pins                                                                                                |
| Battery sense     | Resistive divider **2:1** (e.g. 100 kΩ / 100 kΩ) to ADC pin; `**divider_ratio = 2.0`** in `cosmos_battery`                                              |
| ADC filter        | **100 nF** ceramic from sense node (mid-divider tap) to **GND**, close to module pin                                                                    |
| Digital inputs    | Match firmware pull: contact sensor uses **pull-down** (reed open = high-Z, closed = tied high)                                                         |
| Factory reset     | **GPIO9** (`BOOT` on XIAO): long-press ≥ 5 s (`CONFIG_BUTTON_LONG_PRESS_TIME_MS=5000`); expose accessible tact switch                                   |
| Decoupling        | **100 nF** on each LED branch / noisy output near load; module relies on XIAO on-board decoupling                                                       |


### Power architecture (1S family)

Battery SKUs use a **single-cell (1S) Li-ion** pack (3.0–4.2 V). Do **not** put **2S** on XIAO battery pads. `cosmos_battery` thresholds assume 1S.

#### Product power roles (locked)


| SKU                 | Role                              | USB-C                                             | Battery             |
| ------------------- | --------------------------------- | ------------------------------------------------- | ------------------- |
| **1** Door sensor   | Fully portable                    | **Charge (+ flash) only**                         | Always runs from 1S |
| **2** Dual-mode btn | Fully portable                    | **Charge (+ flash) only**                         | Always runs from 1S |
| **3** Env sensor    | Mains / desk                      | **Only power source**                             | **None** (v1)       |
| **4** Bedside lamp  | USB primary; portable when needed | Power + charge; normal use plugged                | 1S for cordless use |
| **5** Door intercom | Fully portable                    | **Charge (+ flash) only**; may run while charging | Always runs from 1S |


#### USB-C port strategy (locked)


| SKU       | Product USB-C                       | Which connector             | Module USB-C                                               |
| --------- | ----------------------------------- | --------------------------- | ---------------------------------------------------------- |
| **1 / 2** | Yes — charge + flash                | **XIAO on-module** only     | **Is** the product port (edge access in enclosure)         |
| **3**     | Yes — desk power only (no cell)     | **Carrier receptacle (J1)** | Flash / bring-up only; do **not** dual-feed with J1        |
| **4**     | Yes — power + charge + LED 5 V      | **Carrier receptacle (J1)** | Flash / bring-up only; do **not** dual-feed charge with J1 |
| **5**     | Yes — charge (+ run while charging) | **Carrier receptacle (J1)** | Flash / bring-up only; do **not** dual-feed charge with J1 |


**Why carrier USB on 3 / 4 / 5?** Higher current (display / LEDs / camera), awkward module orientation, and a single controlled VBUS → system (and charger on 4/5) path. Product power/charge must go through **J1**, not a second live cable into the module jack.

#### Hard anti-leakage rules (all SKUs)

Never create a path that lets one rail back-feed another:


| Forbidden                                                                              | Why                                               |
| -------------------------------------------------------------------------------------- | ------------------------------------------------- |
| Tie **USB VBUS / XIAO `5V`** directly to **BAT+**                                      | Back-feeds the cell / charger; can fight USB host |
| Tie **boost VOUT (5 V)** to **USB VBUS** or **XIAO `5V`** without OR / load-switch     | Boost pushes into USB or USB fights boost         |
| Tie **boost VOUT** to **XIAO `3V3`** or GPIO rails                                     | Overvoltage / latch-up                            |
| Power peripherals from **XIAO `5V`** and expect them on battery                        | `5V` is dead on battery — only VBUS               |
| Carrier USB-C **and** module USB-C both wired to VBUS without a single controlled path | Dual feed / charge confusion                      |


**Allowed OR points only:**

1. **Charge path:** USB VBUS → **charger VIN only** → protect → **BAT+ / cell**.
2. **System load (battery SKUs):** Always from **BAT+ (after protect)** or a PMIC **SYS** pin — never from raw VBUS in parallel with BAT.
3. **SKU 4 LED 5 V only:** `USB_5V` **OR** `Boost_5V` → `LED_VDD` via **ideal diode / Schottky pair / load switch**; never a hard short between those sources.
4. **XIAO MCU:** Always **BAT pads** (battery SKUs) or **USB / `5V`→LDO** (SKU 3). Do not also hard-wire carrier 5 V into XIAO `5V` while using module USB for charge unless that net is the same controlled VBUS→charger input.

Prefer a **power-path charger** (USB → SYS for load, separate BAT charge) on SKU **4** and **5** if budget allows; classic TP4056 with load on BAT works for Beta but shares charge current with the load.

#### Recommended topology by SKU

**SKU 1 / 2 — portable, USB = charge only**

```text
USB-C (module edge OK) ──► XIAO onboard charger ──► BAT pads ──► 1S cell
                                                              │
                                                         XIAO 3V3 LDO
                                                              │
                                                    sensors / LEDs / piezo (3V3 or BAT+)
```

- No carrier boost. No use of XIAO `5V` for loads.
- Optional carrier USB-C only if it is **VBUS → same charger input** (not a second path to BAT).
- ADC divider: ≥100 k / 100 k (or MOSFET-gated) so sense is not a constant drain.

**SKU 3 — desk USB, no battery (display + BME680)**

```text
Carrier USB-C (J1) ──► VBUS 5V ──► XIAO `5V` ──► onboard LDO ──► 3V3 ──► MCU
                              │
                              ├──► BME680 VDD (3.3 V)
                              └──► ST7789 VDD (3.3 V) + BL driver from GPIO12
```

- **No** charger IC, **no** JST, **no** battery divider on v1 (GPIO6 reserved for v2).
- Module USB-C = flash / bring-up only — do not dual-feed with J1.
- Place J1 by hand in Flux; size traces for display backlight peaks.

**SKU 4 — USB primary + portable boost**

```text
USB-C 5V ──► power-path / 1S charger + protect ──► BAT+ / cell
         │                                         │
         │                    ┌────────────────────┼──────────────┐
         │                    ▼                    ▼              ▼
         │               XIAO BAT            Boost VIN        sense divider
         │            (MCU → 3.3 V)               │
         │                                   Boost VOUT 5V
         │                                        │
         └──── ideal-diode / load-switch OR ──────┴──► WS2812 VDD only
```


| Rule               | Detail                                                                                                                               |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------------------ |
| Boost IC           | e.g. **MT3608** @ **5.00 V**, **≥ 1.5 A**                                                                                            |
| Boost VIN          | BAT+ after protect only                                                                                                              |
| Boost VOUT         | **LED rail only**                                                                                                                    |
| USB plugged        | LEDs from **carrier** USB 5 V via OR; **boost EN = off** (cell does not feed LEDs)                                                   |
| Battery / lamp Off | **boost EN = off** (GPIO or USB-detect)                                                                                              |
| MCU vs LEDs        | XIAO (or carrier SYS) switches MCU USB↔BAT; **carrier** switches LED rail — see [Who switches what](#who-switches-what-mcu-vs-loads) |
| Cell               | ≥3000 mAh, ~1–2 A peaks                                                                                                              |


**Why not 2S + buck?** Shared 1S line + firmware; revisit only if soak shows brownouts.

**SKU 5 — portable camera, USB = charge (run-while-charge OK)**

```text
USB-C ──► 1S charger + protect ──► BAT+ / cell ──► XIAO BAT (Sense 3V3 + camera)
                                      │
                                 sense divider (GPIO5)
                                 piezo/siren from 3V3 or BAT+ via NPN
```

- No 5 V boost required for v1 (camera / MCU on Sense 3.3 V rail).
- Do **not** hang camera or siren on XIAO `5V` (dead on battery).
- Prefer power-path charger so USB can run the system while charging without back-feeding the host.
- Size traces / cell for ≥1 A Wi‑Fi + camera peaks.

#### XIAO `5V` pin vs USB-C

On XIAO ESP32-C6 / S3 / C5, header `**5V**` = **USB VBUS**:


| Power source     | `5V` pin          | `3V3` pin                |
| ---------------- | ----------------- | ------------------------ |
| USB-C plugged in | ≈ **5 V**         | Regulated 3.3 V          |
| **Battery only** | **No usable 5 V** | Regulated 3.3 V from BAT |


Not a boost from the cell. Battery-only 5 V loads need a **carrier boost** (SKU 4). Feeding *into* `5V` needs a diode and a single charge path — prefer carrier USB-C → charger for products.

#### Who switches what (MCU vs loads)

The cell stays **electrically attached** whenever it is installed (and charges when USB is present). That is not the same as “battery is always the load supply.”


| Path                                       | Who switches USB ↔ battery                                     | Notes                                                                                                                                                                   |
| ------------------------------------------ | -------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **XIAO MCU → 3V3**                         | **On-module** (e.g. C6: SGM40567 charger + diode/FET → LDO)    | USB plugged: VBUS feeds LDO + charges cell. USB unplugged: BAT feeds LDO. Do not also hard-feed XIAO `5V` from a second uncontrolled source.                            |
| **SKU 4 LED 5 V**                          | **Carrier only** (ideal diode / load-switch OR + **boost EN**) | USB plugged: LEDs from **carrier USB VBUS**; **boost EN = off** → cell does **not** feed LEDs. Battery / lamp Off: boost EN off. XIAO does **not** switch the LED rail. |
| **SKU 1 / 2 / 5 loads**                    | Run from **3V3 or BAT+** after XIAO / carrier protect          | No 5 V boost. Use module USB (or carrier USB → charger only) for charge; do not hang loads on XIAO `5V`.                                                                |
| **SKU 3 loads**                            | Carrier **J1 VBUS** → XIAO `5V` / 3V3                          | No battery on v1; display + BME680 from 3V3; module USB flash-only.                                                                                                     |
| **SKU 4 / 5 with carrier power-path PMIC** | **Carrier SYS** feeds system; XIAO may see only BAT+/SYS       | Prefer this when camera / LED current is high. Avoid stacking a second charger into XIAO BAT while a carrier charger already owns the cell.                             |


**SKU 4 mental model when USB-C is plugged:** battery is charging (and still connected); **MCU** may be on USB via XIAO or via carrier SYS; **LEDs** must be on **USB VBUS via OR**, not on the boost/cell path.

#### Piezo buzzers (SKU 1 / 5)

| Prefer | Avoid |
|--------|--------|
| **Active** magnetic/piezo rated **3–5 V** (or explicit 3.3 V) | “5 V only” parts if the board has no 5 V rail; **passive** (PWM) unless a dedicated GPIO is added |

Drive with **NPN** (e.g. S8050) + ~1 kΩ base from GPIO: collector to buzzer ← **3V3** or **BAT+**. Do not source buzzer current from the GPIO pin. Flyback diode if the part is magnetic/inductive.

**SKU 1 — same MPN, two channels, no extra GPIO:** both buzzers are PUI **AI-1223-TWT-3V-2-R** (active magnetic, **2.3 kHz**, 2–4 V). Arm vs alarm are distinguished by **which channel and blink pattern**, not pitch.

| Sound | GPIO | LED | Designator | MPN |
|-------|------|-----|------------|-----|
| Arm / confirm | GPIO22 | Confirm (yellow) | **BZ2** | PUI **AI-1223-TWT-3V-2-R** |
| Alarm | GPIO23 | Alarm (red) | **BZ1** | PUI **AI-1223-TWT-3V-2-R** (same) |

Firmware stays on/off (same blink as the LED). Do **not** put both buzzers on one GPIO. Status LED (GPIO21) stays silent.

**SKU 5** keeps a single alarm/siren active piezo on GPIO4.

### RF / layout (Wi‑Fi SKUs: C6, C5)


| Rule             | Value / note                                                                                                                                          |
| ---------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| Antenna keep-out | **No copper, ground fill, or components** under the XIAO PCB antenna area (module end opposite USB)                                                   |
| Ground           | Solid **GND** pour on bottom layer; stitch vias near module ground pads                                                                               |
| USB              | If USB-C is broken out, follow Seeed/XIAO keep-out and differential routing guidelines (optional on carrier — programming can use edge USB on module) |


### PCB fabrication

Target fab: **JLCPCB standard 2-layer** (capability floor is 5 mil / 5 mil; we keep a **6 mil** margin).


| Parameter       | Default                                                          |
| --------------- | ---------------------------------------------------------------- |
| Layers          | **2** (no 4-layer; no blind/buried / via-in-pad)                 |
| Thickness       | **1.6 mm**                                                       |
| Copper          | **1 oz** (0.035 mm) both sides                                   |
| Min trace/space | **6 mil / 6 mil** (0.1524 mm)                                    |
| Default signal  | **10 mil** (GPIO / LED / control / ADC)                          |
| Power traces    | **VBAT ≥ 20 mil (0.5 mm)**; **3V3 / GND stubs ≥ 16 mil (0.4 mm)** |
| Via             | **0.3 mm drill / 0.6 mm pad** through-hole only                  |
| Copper to edge  | **≥ 0.3 mm**                                                     |
| Mask / silk     | Green mask, white silk; text ≥ **1.0 mm**, stroke ≥ **0.15 mm**  |
| Silkscreen      | Product name, `3V3`, `GND`, `BAT+`, revision                     |
| Test            | **BAT sense**, **3V3**, **GND** pads or test points for bring-up |

**SKU 1 Flux:** rulesets locked under [PCB Fabrication Rules](https://www.flux.ai/cosmoskiller/cosmos-iotdoorsensor~7o/files/pcb-fabrication-rules~o3).


### Design workflow (Flux / KiCad)

1. Place **XIAO footprint** first; lock antenna keep-out.
2. Route **power** (JST-PH → optional protection → XIAO `BAT`).
3. Route **battery divider + ADC** and **sensor input** before auto-router.
4. Place **LEDs / buzzer drivers** on designated GPIOs (do not reassign without firmware change).
5. Run DRC; export **Gerber + BOM + pick-and-place** for prototype order.

### ECAD / schematics (Phase 6 tracking)

Firmware GPIO + Flux prompts in this file are the **source of truth** until Gerbers land.


| SKU                        | Flux / schematic status             | Link                                                                             |
| -------------------------- | ----------------------------------- | -------------------------------------------------------------------------------- |
| iotDoorSensor (1)          | Placement done; JLCPCB 2L DRC locked; routing incomplete | [cosmos-iotDoorSensor](https://www.flux.ai/cosmoskiller/cosmos-iotdoorsensor~7o) |
| iotDualModeBtn (2)         | Prompt + BOM ready — layout next    | *Add project URL when shared*                                                    |
| iotEnvironmentalSensor (3) | Prompt + BOM ready (60×60 mm)       | *Add project URL when shared*                                                    |
| iotBedsideLamp (4)         | Prompt + BOM ready (Ø50 mm)         | *Add project URL when shared*                                                    |
| iotDoorIntercom (5)        | Prompt + BOM ready (outdoor 60×100) | *Add project URL when shared*                                                    |


When a Flux or KiCad project is public (or in a private hardware repo), paste the URL in the table above and optionally add a `hardware/` submodule or sibling repo note here.

---

## iotDoorSensor

**Firmware app:** `iotDoorSensor/`  
**Module:** [Seeed XIAO ESP32-C6](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)  
**Matter (test):** VID **65522** (`0xFFF2`), PID **32769** (`0x8001`)  
**Role:** Matter door/window contact sensor, status LEDs, optional panic/alarm outputs, battery reporting.  
**Flux:** [cosmos-iotDoorSensor](https://www.flux.ai/cosmoskiller/cosmos-iotdoorsensor~7o) — schematic + placement complete; routing not finished.

### Product decisions (locked for v1 carrier)

| Item | Choice |
|------|--------|
| Form | **30 × 90 mm** 2-layer PCB, **5 mm** corner radius |
| Module | Seeed XIAO ESP32-C6; USB-C on module = charge + flash only |
| Battery | **1S** pouch via **JST-PH 2.0** right-angle header (**U2** `S2B-PH-K-S(LF)(SN)`) → XIAO BAT+/BAT− |
| Buzzers | **Two channels, same MPN** — PUI **AI-1223-TWT-3V-2-R** (2.3 kHz) on GPIO22 (arm) and GPIO23 (alarm); distinguish by pattern |
| Reed | Coto **CT10-1530-G1** (SMD NO) |
| Factory reset | XUNPU **TS-1088-AR02016** tact to BOOT/GPIO9 |

### GPIO map (must match firmware)


| XIAO pin | ESP GPIO | Firmware                         | Function                               |
| -------- | -------- | -------------------------------- | -------------------------------------- |
| D9       | GPIO20   | `SENSOR_PIN`                     | Reed / contact input (pull-down in SW) |
| D3       | GPIO21   | `STATE_LED_PIN`                  | Status LED (event aggregator)          |
| D4       | GPIO22   | `CONFIRM_LED_PIN`                | Arm / confirm LED + **BZ2**            |
| D5       | GPIO23   | `ALARM_LED_PIN`                  | Alarm LED + **BZ1**                    |
| D0 / A0  | GPIO0    | `CONFIG_COSMOS_BATTERY_ADC_GPIO` | Battery voltage sense (ADC1)           |
| BOOT     | GPIO9    | `FACTORY_RESET_BUTTON_PIN`       | Factory reset (long-press)             |


Unused in current firmware (available for carrier features): D1, D2, D6–D8, D10.

**Sensor logic:** GPIO20 with **internal pull-down** — configure reed switch between **3.3 V** and **D9** (closed = high = triggered). Debouncing is software/GPIO ISR.

**Battery sense:** Tap divider at **D0/A0**; firmware scales by **2.0×** to infer cell voltage.

**Indicators:** Active-high LED drive from GPIO21–23. **BZ1** (alarm) and **BZ2** (arm) are both **AI-1223-TWT-3V-2-R** via NPN on GPIO23 / GPIO22 — see [Piezo buzzers](#piezo-buzzers-sku-1--5).

### Flux.ai project prompt

Copy into Flux when iterating the carrier (board size / JST already locked in the live project):

> **Status:** Flux schematic + placement done (Aug 2026); routing remaining. Project: https://www.flux.ai/cosmoskiller/cosmos-iotdoorsensor~7o

```text
Design / finish a 2-layer carrier PCB for the "Cosmos iotDoorSensor" — compact Wi-Fi Matter door/window contact sensor.

Form:
- Board outline 30 × 90 mm, 5 mm corner radius, 2 layers, 1.6 mm FR4, 1 oz.
- Seeed XIAO ESP32-C6 castellated; USB-C on the module accessible for charge + flash.
- Keep the antenna area at the module end clear: no copper or components under the on-module PCB antenna.

Power:
- U2: JST-PH 2.0 right-angle 2-pin S2B-PH-K-S(LF)(SN) for 1S Li-ion pouch (3.7 V nominal, 4.2 V max). Edge-accessible mating.
- Connect U2 to XIAO BAT+ / BAT− (not 3V3). Do not hang loads on XIAO 5V.
- Battery monitor: 100 kΩ + 100 kΩ divider BAT+ to GND; mid tap to XIAO D0 (GPIO0). 100 nF from tap to GND; 100 nF on VBAT.

Digital inputs:
- Reed CT10-1530-G1 from 3.3 V to XIAO D9 (GPIO20). SW pull-down; closed = HIGH.
- Factory-reset tact TS-1088-AR02016 from XIAO BOOT (GPIO9) to GND.

Digital outputs (3.3 V, active high):
- D3 GPIO21 → green LED (e.g. Würth 150060VS75000) + 330 Ω. No buzzer.
- D4 GPIO22 → yellow LED (e.g. LTST-C190KSKT) + 330 Ω, and BZ2 AI-1223-TWT-3V-2-R via S8050 + 1 kΩ base; BZ+ = 3V3. Optional DNP flyback.
- D5 GPIO23 → red LED (e.g. LTST-C190KRKT) + 330 Ω, and BZ1 AI-1223-TWT-3V-2-R (same MPN) via S8050 + 1 kΩ; flyback 1N4148 populated.

Layout:
- Placement complete in Flux; finish routing; solid GND pour; antenna keep-out; test pads BAT+/3V3/GND/ADC.
- Do not reassign GPIOs. JLCPCB-friendly 0603 passives.
```

### Bill of materials (prototype — matches Flux)


| Ref | Qty | Description | Notes |
|-----|-----|-------------|--------|
| U1 | 1 | Seeed XIAO ESP32-C6 (MPN 113991254) | Matter MCU module |
| U2 | 1 | JST **S2B-PH-K-S(LF)(SN)** | Right-angle PH 2.0, 1S pouch |
| BAT1 | 1 | 1S Li-ion pouch | Off-board; 3.7 V nominal |
| SW1 | 1 | Coto **CT10-1530-G1** | Reed NO, SMD |
| SW2 | 1 | XUNPU **TS-1088-AR02016** | Factory reset — GPIO9 |
| R1, R2 | 2 | 100 kΩ, 0603, 1% | Battery divider |
| R3–R5 | 3 | 330 Ω, 0603 | LED series |
| R6, R7 | 2 | 1 kΩ, 0603 | Q1 / Q2 base |
| C1, C2 | 2 | 100 nF, 0603 | VBAT bypass + ADC filter |
| LED1 | 1 | Würth **150060VS75000** | Green status — GPIO21 |
| LED2 | 1 | Lite-On **LTST-C190KSKT** | Yellow confirm — GPIO22 |
| LED3 | 1 | Lite-On **LTST-C190KRKT** | Red alarm — GPIO23 |
| Q1, Q2 | 2 | **S8050** SOT-23 | Alarm / arm buzzer low-side |
| BZ1, BZ2 | 2 | PUI **AI-1223-TWT-3V-2-R** | Same 2.3 kHz active; alarm / arm |
| D1 | 1 | **1N4148W** | Flyback on BZ1 (populated) |
| D2 | 0–1 | **1N4148W** | Flyback on BZ2 — **DNP / exclude BOM** in Flux |
| — | — | Enclosure, magnet | Mechanical |

**Bring-up checklist** — prototype validated 2026-07 (XIAO ESP32-C6 bench carrier; contact input exercised with a **latching toggle** in place of reed for Boolean State testing).

- [x] Divider ratio verified (100 kΩ / 100 kΩ → plausible cell % in HA; fine-tune divider/thresholds after MVP soak if needed)
- [x] Contact input toggles Matter Boolean State (endpoint 1) — latching switch stand-in for reed; replace with reed + magnet on production carrier
- [x] Long-press factory reset clears fabric (GPIO9)
- [x] Battery percent updates in Matter Power Source cluster (endpoint 3) and visible in Home Assistant
- [x] LEDs match `evt_service` / panic tasks on GPIO21–23
- [x] HA low-battery package — [`home-assistant/packages/cosmos_door_sensor.yaml`](../home-assistant/packages/cosmos_door_sensor.yaml) installed and notifying; fleet/OTA in [cosmos-ha-field](https://github.com/CosmosKiller/cosmos-ha-field)
- [ ] Flux carrier: finish routing → Gerbers → fab bring-up (reed + both buzzers)

### Firmware modules

Matter, contact sensor driver, event service, panic/alarm outputs, OTA via `cosmos_matter_ota`, battery via `cosmos_battery`.

---

## iotBedsideLamp (SKU 4)

**MVP board:** [ESP32-C6-DevKitC-1](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/hw-reference/esp32c6/user-guide-devkitm-1.html) (bench bring-up)  
**Target carrier:** circular PCB + [Seeed XIAO ESP32-C6](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)  
**Matter role:** Extended color light (On/Off, brightness, color).  
**PID (test):** `0x8004` — see [MANUFACTURING.md](MANUFACTURING.md).

### Product decisions (locked for v1 carrier)


| Item            | Choice                                                                                                                        |
| --------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| Form            | **Circular PCB**, LED ring onboard                                                                                            |
| LEDs            | **10× WS2812** (or SK6812), single data line                                                                                  |
| Ring / board OD | **≈ Ø50 mm** (10× 5050 LEDs need ~Ø45–55 mm; treat “5 mm” as typo for 50 mm — change OD in Flux if you want larger)           |
| Buttons         | **2 only** — user + factory reset (no third)                                                                                  |
| Battery         | **1S** pouch via **JST-PH 2.0**; size free; target **≥ 3 h at full brightness** → recommend **≥ 3000 mAh** (see power budget) |
| USB             | USB-C on carrier for power + charge                                                                                           |


### GPIO map (carrier — must match firmware / Kconfig)


| XIAO pin | ESP GPIO | Firmware / Kconfig                     | Function                                     |
| -------- | -------- | -------------------------------------- | -------------------------------------------- |
| D8       | GPIO19   | `CONFIG_BEDSIDE_LAMP_LED_GPIO`         | WS2812 data (RMT) — **10 LEDs**              |
| D9       | GPIO20   | `CONFIG_BEDSIDE_LAMP_USER_BUTTON_GPIO` | User tact (toggle / double / long → presets) |
| BOOT     | GPIO9    | `FACTORY_RESET_BUTTON_PIN`             | Factory reset tact (long ≥ 5 s)              |
| D0 / A0  | GPIO0    | `CONFIG_COSMOS_BATTERY_ADC_GPIO`       | Battery sense (divider mid-tap)              |


**MVP DevKit (today):** LED data **GPIO8**, user **GPIO20**, reset **GPIO9**. Battery ADC on **GPIO0** is enabled in firmware (floating / unused on DevKit until carrier divider is wired). Carrier needs LED GPIO19 + LED count 10 override.

Do **not** reuse the user button for factory reset.

### Power budget (full white, rough)


| Load                    | Estimate                                                 |
| ----------------------- | -------------------------------------------------------- |
| 10× WS2812 @ full white | ~500–600 mA @ 5 V                                        |
| ESP32-C6 + Wi‑Fi        | ~80–150 mA                                               |
| Total                   | ~0.7–0.8 A                                               |
| **3 h runtime**         | **≥ 2.4 Ah** usable → specify **1S 3000–3500 mAh** pouch |


**Rails:** USB-C 5 V → charge IC → **1S** cell (JST). Cell → XIAO `BAT` (MCU 3.3 V; on-module charger/LDO also switches USB↔BAT for the MCU when module USB is used). **Separate 5 V boost from BAT+** (MT3608-class, ≥1.5 A, set to 5.00 V) → WS2812 VDD only. When USB is present: LEDs from **carrier USB VBUS** via ideal-diode/OR; **boost EN = off** so the cell does not feed LEDs. MCU path ≠ LED path — see [Who switches what](#who-switches-what-mcu-vs-loads). Data from GPIO19 (3.3 V) via series resistor. Shared `**cosmos_battery`** divider **2:1** on GPIO0. See [Power architecture](#power-architecture-1s-family). **Not 2S.**

### MVP — DevKitC-1 bench setup


| Signal          | DevKit (now)      | Target carrier        |
| --------------- | ----------------- | --------------------- |
| Addressable LED | GPIO8 — 1× WS2812 | GPIO19 — **10×** ring |
| User button     | GPIO20            | Dedicated tact        |
| Factory reset   | GPIO9 (BOOT)      | Dedicated tact        |
| Power           | USB               | USB-C + 1S JST        |


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
- Architecture: **1S only** (not 2S). Prefer power-path charger if available; else TP4056 + DW01/FS8205 class. USB-C 5 V → charger + protection → JST-PH 2.0 pouch ≥ 3000 mAh (cell must support ~1–2 A peaks).
- XIAO BAT from the same BAT+ (or SYS) net. MCU 3.3 V is on-module. Cell stays attached while charging; that does **not** mean LEDs draw from the cell when USB is present.
- Battery sense: 100 kΩ + 100 kΩ divider BAT+ to GND; mid tap to XIAO D0/A0 (GPIO0); 100 nF at the ADC pin.
- **5 V LED rail (carrier-switched, not XIAO):** boost from BAT+ to 5.00 V (e.g. MT3608), **≥ 1.5 A**. Boost VIN = BAT+ after protect only; VOUT = WS2812 VDD only. Bulk cap on VOUT; local 100 nF at LEDs.
- **USB plugged:** LED_VDD from **carrier USB VBUS** via ideal diode / Schottky OR / load switch; **boost EN = forced off** so battery cannot feed LEDs. Do not hard-short USB 5 V to boost VOUT.
- **USB unplugged / portable:** boost EN on (or gated by lamp On); LEDs from boost only.
- Optional: same boost EN also off when lamp is Off (GPIO or USB-detect).
- Single charge path: do not stack carrier charger + XIAO onboard charger fighting the same cell (pick carrier charger for this SKU; use module USB for flash only, or isolate).
- Do **not** put 5 V on XIAO GPIO or 3V3. Do **not** power WS2812 from 3V3, raw BAT, or XIAO `5V` alone for battery mode.

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


| Ref        | Qty | Description                                                                 | Notes                                                      |
| ---------- | --- | --------------------------------------------------------------------------- | ---------------------------------------------------------- |
| U1         | 1   | [Seeed XIAO ESP32-C6](https://www.seeedstudio.com/XIAO-ESP32C6-p-5914.html) | Matter MCU                                                 |
| LED1–LED10 | 10  | WS2812B / SK6812, 5050                                                      | Ring, one data line                                        |
| R_LED      | 1   | 33–100 Ω, 0603                                                              | Series on DIN                                              |
| SW1        | 1   | Tact switch                                                                 | User — GPIO20                                              |
| SW2        | 1   | Tact switch                                                                 | Factory reset — GPIO9                                      |
| J1         | 1   | USB-C receptacle (power)                                                    | Charge + 5 V                                               |
| J2         | 1   | JST-PH 2.0, 2-pin                                                           | 1S pouch ≥ 3000 mAh                                        |
| U2         | 1   | 1S Li-ion charger + protection                                              | e.g. TP4056 + DW01 path                                    |
| U3         | 1   | 5 V boost ≥ 1.5 A (e.g. MT3608 set to 5.00 V)                               | VIN=BAT+, VOUT=LED 5 V only                                |
| D_OR       | 1–2 | Schottky / ideal-diode / load-switch                                        | **Required:** USB 5 V OR boost → LED_VDD; never hard-short |
| C_BST      | 1–2 | 10–47 µF                                                                    | Boost input/output bulk                                    |
| R1, R2     | 2   | 100 kΩ, 0603, 1%                                                            | Battery divider                                            |
| C1         | 1   | 100 nF, 0603                                                                | ADC filter                                                 |
| C_LED      | 10  | 100 nF, 0603                                                                | Local LED decoupling (optional but recommended)            |
| BAT1       | 1   | 1S Li-ion pouch ≥ 3000 mAh                                                  | Off-board, JST; size OK                                    |
| —          | —   | Diffuser / enclosure                                                        | Mechanical                                                 |


### Bring-up checklist (carrier)

- [ ] 10 LEDs light as one Matter extended-color light (firmware LED count = 10)
- [ ] User button: click / double / long preset; reset button does **not** toggle lamp
- [ ] Factory reset long-press clears fabric
- [ ] USB charges cell; with USB plugged, LEDs run from VBUS and boost EN is off (no cell→LED path)
- [ ] On battery only, boost supplies LEDs; ≥ 3 h full white soak
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


| Item        | Choice                                                                  |
| ----------- | ----------------------------------------------------------------------- |
| Form        | Compact handheld / wall puck (similar class to door sensor)             |
| Buttons     | **2** — large action tact + recessed factory-reset tact                 |
| Indicators  | **2× discrete LEDs** (firmware today — not a single RGB package)        |
| Power       | **1S** Li-ion pouch via **JST-PH 2.0** (`J1`); battery sense on carrier |
| Environment | Indoor (default); conformal coat optional                               |


> Early notes mentioned “RGB LED”; shipping firmware drives **two** GPIOs (`SINGLE_PRESS` / `MULTI_PRESS`). PCB matches firmware.

### GPIO map (must match firmware)


| XIAO pin | ESP GPIO | Firmware                         | Function / wiring                                             |
| -------- | -------- | -------------------------------- | ------------------------------------------------------------- |
| D9       | GPIO20   | `BUTTON_GPIO_PIN`                | Action tact to **GND** (iot_button pull-up; press = LOW)      |
| BOOT     | GPIO9    | `FACTORY_RESET_BUTTON_PIN`       | Reset tact to **GND**; long ≥ 5 s — **not** the action button |
| D3       | GPIO21   | `SINGLE_PRESS_LED_PIN`           | Active-high LED + 330 Ω (single-press feedback)               |
| D8       | GPIO19   | `MULTI_PRESS_LED_PIN`            | Active-high LED + 330 Ω (multi-press feedback)                |
| D0 / A0  | GPIO0    | `CONFIG_COSMOS_BATTERY_ADC_GPIO` | Battery divider mid-tap (2:1)                                 |


Unused on carrier for v1 (available): D1, D2, D4–D7, D10.

### Flux.ai project prompt

```text
Design a 2-layer carrier PCB for the "Cosmos iotDualModeBtn" — a compact Matter generic-switch remote / wall button (press, multi-press, long-press).

Core module:
- Seeed XIAO ESP32-C6 (castellated), USB accessible for flashing.
- Keep the on-module PCB antenna clear: no copper or components under the antenna end.

Power:
- **J1:** JST-PH 2.0, 2-pin for 1S Li-ion pouch (3.7 V nominal). No on-board cell holder.
- Optional reverse-polarity protection and 100 nF on VBAT.
- Connect J1 to XIAO **BAT pads** (sleepy device — minimize quiescent load). Module USB-C = charge + flash only; on-module path switches USB↔BAT for MCU. No loads on XIAO `5V`.
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


| Ref    | Qty | Description                                                                 | Notes                           |
| ------ | --- | --------------------------------------------------------------------------- | ------------------------------- |
| U1     | 1   | [Seeed XIAO ESP32-C6](https://www.seeedstudio.com/XIAO-ESP32C6-p-5914.html) | Matter MCU                      |
| SW1    | 1   | Tact switch (large / soft)                                                  | Action — GPIO20 to GND          |
| SW2    | 1   | Tact switch (recessed)                                                      | Factory reset — GPIO9 to GND    |
| D1     | 1   | Green LED, 0603                                                             | Single-press (`GPIO21`)         |
| D2     | 1   | Blue or yellow LED, 0603                                                    | Multi-press (`GPIO19`)          |
| R3, R4 | 2   | 330 Ω, 0603                                                                 | LED current limit               |
| J1     | 1   | JST-PH 2.0, 2-pin                                                           | 1S pouch                        |
| BAT1   | 1   | 1S Li-ion pouch                                                             | Off-board, JST; match enclosure |
| R1, R2 | 2   | 100 kΩ, 0603, 1%                                                            | Battery divider → GPIO0         |
| C1     | 1   | 100 nF, 0603, X7R                                                           | ADC filter                      |
| C2     | 1   | 100 nF, 0603                                                                | Optional VBAT decoupling        |
| —      | —   | Enclosure / wall plate                                                      | Mechanical                      |


### Bring-up checklist

- [ ] Single / double / multi press → Matter Switch events in HA (`event.*`)
- [ ] LEDs flash per press type (GPIO21 / GPIO19)
- [ ] Factory-reset long-press clears fabric (GPIO9 only)
- [ ] Battery % via `cosmos_battery` / Power Source in HA
- [ ] OTA image builds (`CHIP_OTA_IMAGE_BUILD`)

### Firmware modules

Matter generic switch, `iot_button_task`, `cosmos_battery` (GPIO0), OTA via `cosmos_matter_ota`, factory reset via `cosmos_matter_common`.

---

## iotEnvironmentalSensor (SKU 3)

**Board:** [Seeed XIAO ESP32-C5](https://wiki.seeedstudio.com/xiao_esp32c5_getting_started/)  
**Matter role:** Temperature / humidity / pressure (BME680); local UI later.  
**PID (test):** `0x8003` — see [MANUFACTURING.md](MANUFACTURING.md).  
**GPIO source of truth:** this file (firmware must match; old bench pins were temporary).

> **Note:** Target is **esp32c5** (`sdkconfig.defaults`, CMake); run `idf.py set-target esp32c5` locally to regenerate `sdkconfig`. C5 is still a preview IDF target.

### Product decisions (locked for v1 carrier)


| Item                 | Choice                                                                                        |
| -------------------- | --------------------------------------------------------------------------------------------- |
| Form                 | **60 × 60 mm** square PCB                                                                     |
| Environment          | Desk / indoor; enclosure + mounting holes + display orientation — **your choice in Flux**     |
| Power                | **Carrier USB-C (J1)** only — no battery, no JST, no charger on v1                            |
| Sensor               | **BME680 bare** on carrier (I2C)                                                              |
| Display              | **1.3" 240×240 ST7789**, SPI, **no CS** (tie module CS to GND), **no touch**; soldered module |
| Encoder              | **EC11** through-hole with integrated push                                                    |
| Factory reset        | **Separate tact** on BOOT (not the encoder push)                                              |
| RGB / addr LED       | **Not on v1** — optional upgrade later                                                        |
| Battery              | **v2 only** — reserve **GPIO6** (ADC_BAT); no divider / cell on v1                            |
| Firmware v1 bring-up | BME680 + Matter + OTA now; encoder / display / reset UI tasks when carrier arrives            |


### GPIO map (must match firmware)


| XIAO pin | ESP GPIO | Function / wiring                                                               |
| -------- | -------- | ------------------------------------------------------------------------------- |
| D0       | GPIO1    | Encoder **A**                                                                   |
| D1       | GPIO0    | Encoder **B**                                                                   |
| D2       | GPIO25   | Encoder **push** (to GND; SW pull-up)                                           |
| BOOT     | GPIO28   | Factory-reset tact to GND (`CONFIG_FACTORY_RESET_BUTTON_GPIO=28`)               |
| D4       | GPIO23   | BME680 **SDA**                                                                  |
| D5       | GPIO24   | BME680 **SCL**                                                                  |
| D8       | GPIO8    | ST7789 **SCK**                                                                  |
| D9       | GPIO9    | ST7789 **DC**                                                                   |
| D10      | GPIO10   | ST7789 **MOSI**                                                                 |
| D6       | GPIO11   | ST7789 **RST**                                                                  |
| D7       | GPIO12   | ST7789 **BL** (PWM / GPIO → NPN or FET + resistor; active HIGH = on)            |
| ADC_BAT  | GPIO6    | **Reserved v2** battery sense — DNP divider on v1                               |
| —        | GPIO26   | XIAO **ADC_CRL** (on-module bat-sense enable) — leave for v2; do not load on v1 |


**ST7789 CS:** hard-tie module **CS to GND** (no MCU CS pin).  
**Unused v1:** D3/GPIO7; RGB / WS2812 footprints omitted.

### Flux.ai project prompt

```text
Design a square 2-layer carrier PCB for "Cosmos iotEnvironmentalSensor" — a desk Matter environmental display (BME680 + ST7789 + EC11).

Form factor:
- Square PCB, 60 × 60 mm. Mounting holes, enclosure, and exact placement of display / encoder / USB are designer choice.
- Center or offset pocket for Seeed XIAO ESP32-C5 (castellated). Keep on-module / u.FL antenna clearances per Seeed (no copper under antenna region).

Core module:
- Seeed XIAO ESP32-C5.

Power (v1 — no battery):
- Carrier USB-C receptacle (J1) as the only product power input. Place J1 by hand.
- VBUS 5 V → XIAO `5V` pin (MCU onboard LDO → 3V3). Feed BME680 and ST7789 from regulated 3.3 V (XIAO 3V3 or a small carrier LDO if current budget needs it).
- No JST, no charger IC, no battery pouch on v1. Do not dual-feed module USB-C and J1 onto VBUS.
- Module USB-C = flash / bring-up only.
- Size power traces for ST7789 backlight peaks (~50–100 mA class) + Wi-Fi.

BME680 (bare):
- Place BME680 with recommended I2C pull-ups (4.7 kΩ to 3V3 on SDA/SCL), 100 nF local decoupling, and airflow / keep-out so the sensor is not heat-soaked by the MCU or backlight.
- SDA = XIAO D4 (GPIO23), SCL = XIAO D5 (GPIO24). Tie SDO for I2C address 0x76 (ADDR_0) unless your BOM uses 0x77.
- Do not reassign I2C pins.

Display (soldered module):
- 1.3" 240×240 ST7789 SPI module, no touch.
- Tie module CS to GND (no CS GPIO).
- Wiring: SCK=D8/GPIO8, MOSI=D10/GPIO10, DC=D9/GPIO9, RST=D6/GPIO11, BL=D7/GPIO12 via transistor/FET + series resistor (GPIO high = backlight on). VDD=3.3 V, common GND.
- Orientation and connector style are designer choice.

Encoder + reset:
- Through-hole EC11: A→D0/GPIO1, B→D1/GPIO0, push→D2/GPIO25 to GND (firmware pull-up). Debounce caps optional.
- Separate factory-reset tact: BOOT/GPIO28 to GND (long press ≥ 5 s). Do not combine with encoder push.

Reserved / DNP for v2:
- Leave silkscreen / pads note for future 1S JST + 100 k / 100 k divider into GPIO6 (ADC_BAT). Do not populate on v1.
- No RGB or addressable LED footprints on v1 (optional upgrade later).

Layout:
- 2 layers, 1.6 mm FR4, 1 oz, JLCPCB-friendly, 0603 passives.
- Solid GND pour; antenna keep-out on XIAO.
- Silkscreen: 5V, 3V3, GND, SDA, SCL, ENC A/B/SW, RESET, TFT DC/RST/BL, REV, product name.
- Test pads: 5V, 3V3, GND, SDA, SCL, GPIO6 (v2).

GPIO lock (do not reassign):
- Enc A=GPIO1, B=GPIO0, push=GPIO25, Reset=GPIO28, SDA=23, SCL=24, SCK=8, DC=9, MOSI=10, RST=11, BL=12. GPIO6 reserved v2 only.
```

### Bill of materials (prototype carrier)


| Ref         | Qty | Description                                                                       | Notes                              |
| ----------- | --- | --------------------------------------------------------------------------------- | ---------------------------------- |
| U1          | 1   | [Seeed XIAO ESP32-C5](https://wiki.seeedstudio.com/xiao_esp32c5_getting_started/) | Matter MCU                         |
| U2          | 1   | BME680 (bare)                                                                     | I2C; addr 0x76 typical             |
| R_I2C       | 2   | 4.7 kΩ, 0603                                                                      | SDA / SCL pull-up to 3V3           |
| C_BME       | 1–2 | 100 nF, 0603                                                                      | Local BME680 decoupling            |
| DISP1       | 1   | ST7789 1.3" 240×240 SPI module                                                    | No touch; CS→GND                   |
| R_BL / Q_BL | 1   | BL series R + NPN/FET                                                             | Drive from GPIO12                  |
| SW_ENC      | 1   | EC11 rotary encoder w/ push                                                       | Through-hole                       |
| SW_RST      | 1   | Tact switch                                                                       | Factory reset — GPIO28             |
| J1          | 1   | USB-C receptacle                                                                  | Product power only (place in Flux) |
| C_USB       | 1–2 | 10 µF + 100 nF                                                                    | VBUS bulk / HF                     |
| —           | —   | Enclosure / stand                                                                 | Mechanical — deferred              |


**DNP v1 / v2 reserve:** JST-PH, battery divider R1/R2, BAT1 pouch, RGB / WS2812.

### Bring-up checklist (carrier)

- [ ] USB-C (J1) powers MCU + BME680; Matter temp/humidity/pressure update in HA
- [ ] ST7789 lights (BL) and accepts SPI once UI firmware lands
- [ ] EC11 A/B/push and separate reset tact wired to locked GPIOs
- [ ] Module USB used for flash without fighting J1
- [ ] No battery / no Power Source required for v1 field units
- [ ] OTA image builds (`CHIP_OTA_IMAGE_BUILD`)

### Firmware modules

Matter temp/humidity/pressure, `bme680_task` (I2C GPIO23/24), OTA via `cosmos_matter_ota`, factory reset on GPIO28. **Display / encoder / LVGL / custom QR / `cosmos_battery` — later** (battery reserved GPIO6 for v2 carrier).

---

## iotDoorIntercom (SKU 5)

**Board:** [Seeed XIAO ESP32-S3 Sense](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/) (OV2640 camera on Sense expansion)  
**Matter role:** Generic Switch doorbell + PIR + tamper + OnOff stream gate + OnOff siren clear.  
**PID (test):** `0x8005` — see [MANUFACTURING.md](MANUFACTURING.md).  
**Stream:** HTTPS MJPEG `GET https://<device-ip>/stream` (port 443, Beta self-signed cert). Not Matter Camera / WebRTC yet.

### Product decisions (locked for v1 carrier)


| Item           | Choice                                                                                                                                                                          |
| -------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Board size     | **≈ 60 × 100 mm** (doorbell / wall-mount)                                                                                                                                       |
| Environment    | **Outdoor** enclosure (gasketed camera window, conformal coat PCB)                                                                                                              |
| Power          | **USB-C** + **JST-PH 2.0** 1S pouch; battery sense on carrier                                                                                                                   |
| Front controls | Doorbell tact + **AM312** mini PIR (3.3 V) on carrier                                                                                                                           |
| Tamper         | **Leaf spring → chassis GND** when seated; **two gold pads on PCB back** closed by **pogo pins** in the housing (series or parallel path — both must make contact when mounted) |
| Siren          | Piezo + LED on **GPIO4**, NPN drive (same idea as door-sensor alarm)                                                                                                            |
| Door lock      | **None** for v1 (GPIO5 used for battery ADC)                                                                                                                                    |
| Camera         | Keep Sense camera FPC / expansion; do not reassign DVP pins                                                                                                                     |


### GPIO map (must match firmware)


| XIAO pin  | ESP GPIO | Firmware                         | Function / wiring                                                               |
| --------- | -------- | -------------------------------- | ------------------------------------------------------------------------------- |
| D0        | GPIO1    | `DOORBELL_PIN`                   | Tact to **3.3 V** (SW pull-down); press = HIGH                                  |
| D1        | GPIO2    | `PIR_PIN`                        | AM312 **OUT** (active HIGH); module VCC=3.3 V, GND                              |
| D2        | GPIO3    | `TAMPER_PIN`                     | SW pull-up; **LOW when seated** (path to GND via leaf spring + back pads/pogos) |
| D3        | GPIO4    | `ALARM_LED_PIN`                  | NPN → red LED + piezo (active HIGH blink)                                       |
| D4        | GPIO5    | `CONFIG_COSMOS_BATTERY_ADC_GPIO` | Mid-tap of 100 k / 100 k divider (`cosmos_battery` enabled)                     |
| BOOT      | GPIO0    | `FACTORY_RESET_BUTTON_PIN`       | Tact to **GND**; long ≥ 5 s (not the doorbell)                                  |
| —         | GPIO21   | `LED_PIN`                        | On-module user LED — stream status (no carrier LED required)                    |
| Sense DVP | (fixed)  | `cam_task.h`                     | Camera — do not steal these GPIOs                                               |


**Unused for v1 (do not load door-lock features):** former door-lock plan on GPIO5 — superseded by battery ADC.

### Tamper electromechanical detail

GPIO: internal pull-up on D2; **LOW = seated (NC to GND)**, **HIGH = open/tampered**. Matter Boolean State uses contact-sensor convention (**true = closed/seated**); HA inverts so tamper is **off when grounded**, **on when open**.

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
- USB-C 5 V input on the carrier → 1S charger + protection (prefer power-path / SYS; else TP4056 + DW01/FS8205 class or better outdoor-rated).
- 1S Li-ion pouch on JST-PH 2.0 (2-pin). Feed XIAO **BAT pads** from BAT+ (or SYS). Cell stays attached while charging; MCU 3V3 is on-module (Sense). Do **not** power camera/siren from XIAO `5V`.
- Single charge path — do not stack carrier charger and Sense onboard charger on the same cell without isolation.
- Battery monitor: 100 kΩ + 100 kΩ divider BAT+ to GND; mid tap to XIAO D4 (GPIO5). 100 nF at ADC pin.
- Camera + Wi-Fi are power-hungry — size traces and charger for ≥ 1 A peaks.
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
- XIAO D3 (GPIO4) → 1 kΩ → NPN base (S8050) → drive in parallel: (a) red LED + 330 Ω; (b) **3–5 V active piezo** (prefer not “5 V only”) with collector feed from **3V3 or BAT+**, flyback diode as needed. GPIO high = siren/LED on (firmware blinks).
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


| Ref        | Qty | Description                                                                             | Notes                          |
| ---------- | --- | --------------------------------------------------------------------------------------- | ------------------------------ |
| U1         | 1   | [Seeed XIAO ESP32-S3 Sense](https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html) | Camera + Wi‑Fi MCU             |
| SW1        | 1   | Doorbell tact (large / weatherized)                                                     | To 3.3 V / GPIO1               |
| SW2        | 1   | Tact switch, recessed                                                                   | Factory reset GPIO0            |
| U2         | 1   | AM312 (or 3.3 V mini PIR)                                                               | OUT → GPIO2                    |
| SW3        | 1   | Leaf spring / chassis contact                                                           | Tamper to GND when seated      |
| PAD1, PAD2 | 2   | Gold / pogo landing pads (back)                                                         | Case pogo short when closed    |
| —          | 2   | Pogo pins (in enclosure)                                                                | Mechanical; not on PCB BOM     |
| Q1         | 1   | NPN SOT-23 (S8050)                                                                      | Siren / LED drive              |
| R_B        | 1   | 1 kΩ, 0603                                                                              | NPN base                       |
| D_ALM      | 1   | Red LED, 0603                                                                           | Alarm visual                   |
| R_LED      | 1   | 330 Ω, 0603                                                                             | LED limit                      |
| BZ1        | 1   | 3–5 V active piezo (not 5 V-only)                                                       | Siren; NPN from 3V3/BAT+       |
| J1         | 1   | USB-C receptacle                                                                        | Power + charge                 |
| J2         | 1   | JST-PH 2.0, 2-pin                                                                       | 1S pouch                       |
| U3         | 1   | 1S charger + protection                                                                 | TP4056-class or better         |
| R1, R2     | 2   | 100 kΩ, 0603, 1%                                                                        | Battery divider → GPIO5        |
| C1         | 1   | 100 nF, 0603                                                                            | ADC filter                     |
| C2         | 1   | 100 nF–10 µF                                                                            | VIN / BAT decoupling as needed |
| BAT1       | 1   | 1S Li-ion pouch                                                                         | Size for outdoor runtime; JST  |
| —          | —   | Outdoor enclosure, gaskets, camera window, Fresnel for PIR                              | Mechanical                     |
| —          | —   | Conformal coat                                                                          | After electrical bring-up      |


### Bring-up checklist

- [ ] Doorbell → Matter `event.*` + stream gate / HA notify
- [ ] AM312 motion → occupancy + auto stream
- [ ] Open case / lift board → tamper binary_sensor on + GPIO4 siren latches; remount does **not** silence; HA siren OnOff Off stops siren
- [ ] Back pogo pads + leaf spring both exercise tamper path
- [ ] USB-C charges pouch; divider on GPIO5 reads plausible % in Matter / HA
- [ ] HTTPS MJPEG through camera window; Wi‑Fi RSSI acceptable in metal/plastic enclosure
- [ ] Factory reset on BOOT only (not doorbell)
- [ ] HA package + Lovelace — `[home-assistant/](../home-assistant/)`

### Firmware modules

Matter (stream OnOff, PIR, doorbell `generic_switch`, tamper `contact_sensor`, siren OnOff, Power Source), `cam_task`, `http_stream_task`, `evt_service_task`, `door_intercom_task`, `security_module_task`, `panic_alarm_task`, `cosmos_battery` (GPIO5), OTA via `cosmos_matter_ota`, factory reset via `cosmos_matter_common`.

**HTTPS /stream (Beta):** certs in `iotDoorIntercom/main/certs/` (regen `tools/certs/gen_door_intercom_https.sh`). HA MJPEG: UI integration, verify SSL off on LAN.

> **Target:** `esp32s3` — `idf.py set-target esp32s3`. Octal PSRAM required for camera framebuffers.

*Matter Camera + WebRTC deferred (after toolchain bump).*