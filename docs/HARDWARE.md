# Hardware — cosmosFivePieceBasis

Per-device GPIO, carrier-board guidance, and ECAD prompts. Update this file when pinouts or the BOM change; keep app `To-Do.MD` notes in sync until retired (see [POLISH_PLAN.md](POLISH_PLAN.md)). Platform choices for SKU **3 / 5** are recorded in [ARCHITECTURE.md](ARCHITECTURE.md).

Firmware is the source of truth for GPIO numbers — carrier boards must match the tables below.

---

## Cosmos carrier design rules

Shared defaults for **low-voltage, sensor-class, 2-layer** carriers. SKU **1 / 2 / 4** use Seeed **XIAO** modules on the carrier. SKU **3 / 5** use a **Waveshare main board** as the product compute; Flux builds an **expansion carrier** only (see [ARCHITECTURE.md](ARCHITECTURE.md)).

### Electrical


| Rule              | Value / note                                                                                                                                                                                                                                                       |
| ----------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Supply domain     | **1S Li-ion** (3.0–4.2 V) or regulated **3.3 V** to XIAO `3V3` / `VIN` per Seeed guidance; firmware assumes **1S** thresholds (empty 3.0 V, full 4.2 V)                                                                                                            |
| Max board voltage | **≤ 12 V** at any net (low-voltage hobby/prototype class)                                                                                                                                                                                                          |
| Logic             | **3.3 V** CMOS only on XIAO GPIO — no 5 V on module pins                                                                                                                                                                                                           |
| Battery sense     | Resistive divider **2:1** (e.g. 100 kΩ / 100 kΩ) to ADC pin; `**divider_ratio = 2.0`** in `cosmos_battery`                                                                                                                                                         |
| ADC filter        | **100 nF** ceramic from sense node (mid-divider tap) to **GND**, close to module pin                                                                                                                                                                               |
| Digital inputs    | **Mandatory external** pull-up or pull-down on the carrier — **do not rely on MCU internal pulls alone** (noise, EMI, fab variance, floating open). Default **10 kΩ**, 0603. Firmware may keep the matching internal pull as a secondary/belt-and-suspenders only. |
| Pull polarity     | Switch/reed to **3V3** → **10 kΩ to GND** (idle LOW). Switch/tact/tamper to **GND** → **10 kΩ to 3V3** (idle HIGH). Place the resistor at the module pin.                                                                                                          |
| Factory reset     | Dedicated tact to **GND** + **external 10 kΩ pull-up to 3V3**; long-press ≥ 5 s (`CONFIG_BUTTON_LONG_PRESS_TIME_MS=5000`)                                                                                                                                          |
| Decoupling        | **100 nF** on each LED branch / noisy output near load; module relies on XIAO on-board decoupling                                                                                                                                                                  |




### Power architecture (1S family)

Battery SKUs use a **single-cell (1S) Li-ion** pack (3.0–4.2 V). Do **not** put **2S** on XIAO battery pads. `cosmos_battery` thresholds assume 1S.

#### Product power roles (locked)


| SKU                 | Role                              | USB-C                                             | Battery             |
| ------------------- | --------------------------------- | ------------------------------------------------- | ------------------- |
| **1** Door sensor   | Fully portable                    | **Charge (+ flash) only**                         | Always runs from 1S |
| **2** Dual-mode btn | Fully portable                    | **Charge (+ flash) only**                         | Always runs from 1S |
| **3** Env sensor    | Desk / portable display           | Waveshare **USB-C** (charge + run)                | **ETA6098** + MX1.25 1S (Waveshare option) |
| **4** Bedside lamp  | USB primary; portable when needed | Power + charge; normal use plugged                | 1S for cordless use |
| **5** Door intercom | Fully portable                    | Waveshare / carrier **USB-C** charge (+ run OK)   | Always runs from 1S |




#### USB-C port strategy (locked)


| SKU       | Product USB-C                       | Which connector             | Module / main-board USB                                                                 |
| --------- | ----------------------------------- | --------------------------- | --------------------------------------------------------------------------------------- |
| **1 / 2** | Yes — charge + flash                | **XIAO on-module** only     | **Is** the product port (edge access in enclosure)                                      |
| **3**     | Yes — charge + run (+ desk OK)      | **Waveshare USB-C**         | Product port on C5-Touch-LCD-2.8; carrier should **not** dual-feed VBUS                 |
| **4**     | Yes — power + charge + LED 5 V      | **Carrier receptacle (J1)** | Flash / bring-up only; do **not** dual-feed charge with J1                              |
| **5**     | Yes — charge (+ run while charging) | **P4-WIFI6 USB-C** and/or carrier J1 | Prefer single controlled charge path; no dual-feed into BAT without isolation |


**Why carrier USB on 4?** Higher LED current and a single controlled VBUS → system/charge path on the lamp carrier. **SKU 3 / 5** use Waveshare onboard USB-C as the product port; Flux carriers expand I/O and must not dual-feed VBUS/BAT.

#### Hard anti-leakage rules (all SKUs)

Never create a path that lets one rail back-feed another:


| Forbidden                                                                              | Why                                               |
| -------------------------------------------------------------------------------------- | ------------------------------------------------- |
| Tie **USB VBUS / XIAO** `5V` directly to **BAT+**                                      | Back-feeds the cell / charger; can fight USB host |
| Tie **boost VOUT (5 V)** to **USB VBUS** or **XIAO** `5V` without OR / load-switch     | Boost pushes into USB or USB fights boost         |
| Tie **boost VOUT** to **XIAO** `3V3` or GPIO rails                                     | Overvoltage / latch-up                            |
| Power peripherals from **XIAO** `5V` and expect them on battery                        | `5V` is dead on battery — only VBUS               |
| Carrier USB-C **and** module USB-C both wired to VBUS without a single controlled path | Dual feed / charge confusion                      |


**Allowed OR points only:**

1. **Charge path:** USB VBUS → **charger VIN only** → protect → **BAT+ / cell**.
2. **System load (battery SKUs):** Always from **BAT+ (after protect)** or a PMIC **SYS** pin — never from raw VBUS in parallel with BAT.
3. **SKU 4 LED 5 V only:** `USB_5V` **OR** `Boost_5V` → `LED_VDD` via **ideal diode / Schottky pair / load switch**; never a hard short between those sources.
4. **XIAO MCU:** Always **BAT pads** (battery SKUs) or **USB /** `5V`**→LDO** (SKU 3). Do not also hard-wire carrier 5 V into XIAO `5V` while using module USB for charge unless that net is the same controlled VBUS→charger input.

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

**SKU 3 — Waveshare C5-Touch-LCD-2.8 (ETA6098 + optional 1S)**

```text
Waveshare USB-C ──► ETA6098 ──► MX1.25 1S cell
                         │
                    board 3V3 rails ──► ESP32-C5, ST7789, SHTC3, mic/spk, headers
                                              │
                         Flux carrier (I2C / SH1.0) ──► Sensirion **SGP41** (pressure deferred)
```

- Prefer Waveshare **battery option** (or board + MX1.25 cell). Do **not** add a second charger on the carrier without isolation.
- Carrier draws 3V3/GND (+ I2C) from main-board headers; no parallel USB-C into the same BAT net.

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

**SKU 5 — Waveshare ESP32-P4-WIFI6 + outdoor carrier**

```text
P4-WIFI6 USB-C ──► board power / flash
                         │
              ESP32-P4 (MIPI-CSI cam, H.264, audio) + ESP32-C6 (Wi-Fi 6 / Matter)
                         │
              Flux carrier headers ──► doorbell / PIR / tamper / siren / 1S battery path
```

- Product compute is the **P4-WIFI6** kit; carrier expands security I/O and battery as needed.
- Prefer power-path / single charge path for 1S; size for >=1 A Wi-Fi + camera peaks.
- Do **not** dual-feed USB into BAT without isolation. Camera stays on P4 MIPI-CSI (not S3 DVP).


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
| **SKU 1 / 2 loads**                        | Run from **3V3 or BAT+** after XIAO / protect                  | No 5 V boost. Module USB for charge; do not hang loads on XIAO `5V`.                                                                            |
| **SKU 3 loads**                            | Waveshare **ETA6098** / USB-C → board 3V3                      | Main board owns power; Flux carrier takes 3V3/GND only (**SGP41**). No second charger.                                                              |
| **SKU 5 loads**                            | P4-WIFI6 USB-C and/or carrier 1S path                          | Camera on P4; carrier I/O from 3V3/BAT+. Single charge path — no dual-feed.                                                                     |
| **SKU 4 with carrier power-path PMIC**     | **Carrier SYS** feeds system; XIAO may see only BAT+/SYS       | Prefer when LED current is high. Avoid stacking a second charger into XIAO BAT while a carrier charger already owns the cell.                   |


**SKU 4 mental model when USB-C is plugged:** battery is charging (and still connected); **MCU** may be on USB via XIAO or via carrier SYS; **LEDs** must be on **USB VBUS via OR**, not on the boost/cell path.

#### Piezo buzzers (SKU 1 / 5)


| Prefer                                                        | Avoid                                                                                             |
| ------------------------------------------------------------- | ------------------------------------------------------------------------------------------------- |
| **Active** magnetic/piezo rated **3–5 V** (or explicit 3.3 V) | “5 V only” parts if the board has no 5 V rail; **passive** (PWM) unless a dedicated GPIO is added |


Drive with **NPN** (e.g. S8050) + ~1 kΩ base from GPIO: collector to buzzer ← **3V3** or **BAT+**. Do not source buzzer current from the GPIO pin. **Flyback diode mandatory** on each magnetic/active channel (SKU 1: **D1** on BZ1, **D2** on BZ2 — same **1N4148W**, both stuffed for first fab).

**SKU 1 — same MPN, two channels, no extra GPIO:** both buzzers are PUI **AI-1223-TWT-3V-2-R** (active magnetic, **2.3 kHz**, 2–4 V). Arm vs alarm are distinguished by **which channel and blink pattern**, not pitch. **BZ1, BZ2, D1, and D2 are all mandatory** for the first manufacturing batch.


| Sound         | GPIO   | LED              | Designator | MPN                               |
| ------------- | ------ | ---------------- | ---------- | --------------------------------- |
| Arm / confirm | GPIO22 | Confirm (yellow) | **BZ2**    | PUI **AI-1223-TWT-3V-2-R**        |
| Alarm         | GPIO23 | Alarm (red)      | **BZ1**    | PUI **AI-1223-TWT-3V-2-R** (same) |


Firmware stays on/off (same blink as the LED). Do **not** put both buzzers on one GPIO. Status LED (GPIO21) stays silent.

**SKU 5** keeps a single alarm/siren active piezo on **GPIO21** (P4 carrier — see SKU 5 GPIO map). Do **not** use GPIO39–48 (ESP32-P4 SDMMC/TF bank; conflicts with `streaming_only`). **SKU 6** uses the same active-high NPN drive on XIAO **D0 / GPIO1**.

### RF / layout (Wi‑Fi SKUs: C6, C5)


| Rule             | Value / note                                                                                                                                          |
| ---------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| Antenna keep-out | **No copper, ground fill, or components** under the XIAO PCB antenna area (module end opposite USB)                                                   |
| Ground           | Solid **GND** pour on bottom layer; stitch vias near module ground pads                                                                               |
| USB              | If USB-C is broken out, follow Seeed/XIAO keep-out and differential routing guidelines (optional on carrier — programming can use edge USB on module) |




### PCB fabrication

Target fab: **JLCPCB standard 2-layer** (capability floor is 5 mil / 5 mil; we keep a **6 mil** margin).


| Parameter       | Default                                                           |
| --------------- | ----------------------------------------------------------------- |
| Layers          | **2** (no 4-layer; no blind/buried / via-in-pad)                  |
| Thickness       | **1.6 mm**                                                        |
| Copper          | **1 oz** (0.035 mm) both sides                                    |
| Min trace/space | **6 mil / 6 mil** (0.1524 mm)                                     |
| Default signal  | **10 mil** (GPIO / LED / control / ADC)                           |
| Power traces    | **VBAT ≥ 20 mil (0.5 mm)**; **3V3 / GND stubs ≥ 16 mil (0.4 mm)** |
| Via             | **0.3 mm drill / 0.6 mm pad** through-hole only                   |
| Copper to edge  | **≥ 0.3 mm**                                                      |
| Mask / silk     | Green mask, white silk; text ≥ **1.0 mm**, stroke ≥ **0.15 mm**   |
| Silkscreen      | Product name, `3V3`, `GND`, `BAT+`, revision                      |
| Test            | **BAT sense**, **3V3**, **GND** pads or test points for bring-up  |


**SKU 1 Flux:** rulesets locked under [PCB Fabrication Rules](https://www.flux.ai/cosmoskiller/cosmos-iotdoorsensor~7o/files/pcb-fabrication-rules~o3).

### Design workflow (Flux / KiCad)

1. Place **XIAO footprint** first; lock antenna keep-out.
2. Route **power** (JST-PH → optional protection → XIAO `BAT`).
3. Route **battery divider + ADC** and **sensor input** before auto-router.
4. Place **LEDs / buzzer drivers** on designated GPIOs (do not reassign without firmware change).
5. Run DRC; export **Gerber + BOM + pick-and-place** for prototype order.



### ECAD / schematics (Phase 6 tracking)

Firmware GPIO + Flux prompts in this file are the **source of truth** until Gerbers land. SKU 1 Gerber/BOM/CPL exported Sep 2026.


| SKU                        | Flux / schematic status                                                                              | Link                                                                             |
| -------------------------- | ---------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------- |
| iotDoorSensor (1)          | **Fab-ready** — DRC clean; BOM + MPNs synced in Flux; Gerber/BOM/CPL exported; order / bring-up next | [cosmos-iotDoorSensor](https://www.flux.ai/cosmoskiller/cosmos-iotdoorsensor~7o) |
| iotDualModeBtn (2)         | Prompt + BOM ready — layout next                                                                     | *Add project URL when shared*                                                    |
| iotEnvironmentalSensor (3) | **Platform agreed:** Waveshare C5-Touch-LCD-2.8 + Flux carrier (**SGP41**); see ARCHITECTURE.md           | *Add carrier Flux URL when shared*                                               |
| iotBedsideLamp (4)         | Prompt + BOM ready (Ø50 mm)                                                                          | *Add project URL when shared*                                                    |
| iotDoorIntercom (5)        | **Platform agreed:** Waveshare ESP32-P4-WIFI6 + Matter 1.5 cam + Flux outdoor carrier                | *Add carrier Flux URL when shared*                                               |
| iotSecurityCamera (6)      | **Locked:** XIAO ESP32-S3 Sense HTTPS MJPEG; CSI presence (espectre) later                           | *Add carrier Flux URL when shared*                                               |


When a Flux or KiCad project is public (or in a private hardware repo), paste the URL in the table above and optionally add a `hardware/` submodule or sibling repo note here.

---



## iotDoorSensor

**Firmware app:** `iotDoorSensor/`  
**Module:** [Seeed XIAO ESP32-C6](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)  
**Matter (test):** VID **65522** (`0xFFF2`), PID **32769** (`0x8001`)  
**Role:** Matter door/window contact sensor, status LEDs, optional panic/alarm outputs, battery reporting.  
**Flux:** [cosmos-iotDoorSensor](https://www.flux.ai/cosmoskiller/cosmos-iotdoorsensor~7o) — **fab-ready** (schematic + layout + DRC clean; BOM/MPNs synced; Gerber + BOM + CPL exported).

### Product decisions (locked for v1 carrier)


| Item          | Choice                                                                                                                       |
| ------------- | ---------------------------------------------------------------------------------------------------------------------------- |
| Form          | **30 × 90 mm** 2-layer PCB, **5 mm** corner radius                                                                           |
| Module        | Seeed XIAO ESP32-C6; USB-C on module = charge + flash only                                                                   |
| Battery       | **1S** pouch via **JST-PH 2.0** right-angle header (**U2** `S2B-PH-K-S(LF)(SN)`) → XIAO BAT+/BAT−                            |
| Buzzers       | **Two channels, same MPN** — PUI **AI-1223-TWT-3V-2-R** on GPIO22 (BZ2/arm) and GPIO23 (BZ1/alarm); **D1+D2 1N4148W flybacks both stuffed** |
| Reed          | Coto **CT10-1530-G1** (SMD NO)                                                                                               |
| Factory reset | XUNPU **TS-1088-AR02016** tact to BOOT/GPIO9                                                                                 |




### GPIO map (must match firmware)


| XIAO pin | ESP GPIO | Firmware                         | Function                                           |
| -------- | -------- | -------------------------------- | -------------------------------------------------- |
| D9       | GPIO20   | `SENSOR_PIN`                     | Reed to **3V3** + **10 kΩ to GND** (closed = HIGH) |
| D3       | GPIO21   | `STATE_LED_PIN`                  | Status LED (event aggregator)                      |
| D4       | GPIO22   | `CONFIRM_LED_PIN`                | Arm / confirm LED + **BZ2**                        |
| D5       | GPIO23   | `ALARM_LED_PIN`                  | Alarm LED + **BZ1**                                |
| D0 / A0  | GPIO0    | `CONFIG_COSMOS_BATTERY_ADC_GPIO` | Battery voltage sense (ADC1)                       |
| BOOT     | GPIO9    | `FACTORY_RESET_BUTTON_PIN`       | Tact to **GND** + **10 kΩ to 3V3**                 |


Unused in current firmware (available for carrier features): D1, D2, D6–D8, D10.

**Sensor logic:** Reed between **3.3 V** and **D9**; **external 10 kΩ pull-down to GND** at D9 (open = LOW, closed = HIGH). Firmware may also enable an internal pull-down as secondary only. Debouncing is software/GPIO ISR.

**Factory reset:** Tact **BOOT → GND** with **external 10 kΩ pull-up to 3V3** at BOOT.

**Battery sense:** Tap divider at **D0/A0**; firmware scales by **2.0×** to infer cell voltage.

**Indicators:** Active-high LED drive from GPIO21–23. **BZ1** (alarm) and **BZ2** (arm) are both **AI-1223-TWT-3V-2-R** via NPN on GPIO23 / GPIO22 — see [Piezo buzzers](#piezo-buzzers-sku-1--5).

### Flux.ai project prompt

Copy into Flux when iterating the carrier (board size / JST already locked in the live project):

> **Status:** Fab-ready (Sep 2026) — DRC clean; BOM/MPNs synced in Flux (incl. D1/D2 stuffed); Gerber + BOM + CPL exported. Next: JLCPCB order + carrier bring-up. Project: [https://www.flux.ai/cosmoskiller/cosmos-iotdoorsensor~7o](https://www.flux.ai/cosmoskiller/cosmos-iotdoorsensor~7o)

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

Digital inputs (external pulls mandatory — do not rely on MCU internals alone):
- Reed CT10-1530-G1 from 3.3 V to XIAO D9 (GPIO20). **10 kΩ pull-down to GND at D9**; closed = HIGH.
- Factory-reset tact TS-1088-AR02016 from XIAO BOOT (GPIO9) to GND. **10 kΩ pull-up to 3V3 at BOOT**.

Digital outputs (3.3 V, active high):
- D3 GPIO21 → green LED (e.g. Würth 150060VS75000) + 330 Ω. No buzzer.
- D4 GPIO22 → yellow LED (e.g. LTST-C190KSKT) + 330 Ω, and BZ2 AI-1223-TWT-3V-2-R via S8050 + 1 kΩ base; BZ+ = 3V3; **D2 1N4148W flyback populated** (same as D1).
- D5 GPIO23 → red LED (e.g. LTST-C190KRKT) + 330 Ω, and BZ1 AI-1223-TWT-3V-2-R (same MPN) via S8050 + 1 kΩ; **D1 1N4148W flyback populated**.
- First fab: **BZ1, BZ2, D1, D2 all mandatory** — do not DNP or exclude from BOM.

Layout:
- Schematic + layout + DRC clean in Flux. Solid GND pour; antenna keep-out; test pads BAT+/3V3/GND/ADC.
- Do not reassign GPIOs. Passives and diodes use locked MPNs from HARDWARE.md BOM.
```



### Bill of materials (prototype — locked)

Passives and diodes below are **locked** for JLCPCB. Confirm LCSC stock codes at order time; **MPNs** are the stable identity. Flux BOM/MPNs synced Sep 2026 (R1–R9, C1–C2, D1–D2 included).


| Ref      | Qty | Description / MPN                                      | Notes / LCSC (typical)                         |
| -------- | --- | ------------------------------------------------------ | ---------------------------------------------- |
| U1       | 1   | Seeed XIAO ESP32-C6 (**113991254**)                    | Matter MCU module                              |
| U2       | 1   | JST **S2B-PH-K-S(LF)(SN)**                             | Right-angle PH 2.0, 1S pouch — C173752         |
| BAT1     | 1   | 1S Li-ion pouch                                        | Off-board; 3.7 V nominal                       |
| SW1      | 1   | Coto **CT10-1530-G1**                                  | Reed NO, SMD                                   |
| SW2      | 1   | XUNPU **TS-1088-AR02016**                              | Factory reset — GPIO9 — C720477                |
| R1, R2   | 2   | Yageo **RC0603FR-07100KL** (100 kΩ 1% 0603)            | Battery divider — C25803                       |
| R3–R5    | 3   | Yageo **RC0603FR-07330RL** (330 Ω 1% 0603)             | LED series — C23148                            |
| R6, R7   | 2   | Yageo **RC0603FR-071KL** (1 kΩ 1% 0603)                | Q1 / Q2 base — C21190                          |
| R8       | 1   | Yageo **RC0603FR-0710KL** (10 kΩ 1% 0603)              | Pull-down at D9 / GPIO20 (reed) — C25804       |
| R9       | 1   | Yageo **RC0603FR-0710KL** (10 kΩ 1% 0603)              | Pull-up at BOOT / GPIO9 — C25804               |
| C1, C2   | 2   | Samsung **CL10B104KB8NNNC** (100 nF X7R 0603)          | VBAT bypass + ADC filter — C14663              |
| LED1     | 1   | Würth **150060VS75000**                                | Green status — GPIO21                          |
| LED2     | 1   | Lite-On **LTST-C190KSKT**                              | Yellow confirm — GPIO22                        |
| LED3     | 1   | Lite-On **LTST-C190KRKT**                              | Red alarm — GPIO23                             |
| Q1, Q2   | 2   | **S8050** SOT-23                                       | Alarm / arm buzzer low-side                    |
| BZ1, BZ2 | 2   | PUI **AI-1223-TWT-3V-2-R**                             | **Mandatory** — same 2.3 kHz; alarm / arm      |
| D1, D2   | 2   | **1N4148W**                                            | **Mandatory** flybacks on BZ1 / BZ2 — C129216  |
| —        | —   | Enclosure, magnet                                      | Mechanical                                     |


**Bring-up checklist** — prototype validated 2026-07 (XIAO ESP32-C6 bench carrier; contact input exercised with a **latching toggle** in place of reed for Boolean State testing).

- [x] Divider ratio verified (100 kΩ / 100 kΩ → plausible cell % in HA; fine-tune divider/thresholds after MVP soak if needed)
- [x] Contact input toggles Matter Boolean State (endpoint 1) — latching switch stand-in for reed; replace with reed + magnet on production carrier
- [x] Long-press factory reset clears fabric (GPIO9)
- [x] Battery percent updates in Matter Power Source cluster (endpoint 3) and visible in Home Assistant
- [x] LEDs match `evt_service` / panic tasks on GPIO21–23
- [x] HA low-battery package — `[home-assistant/packages/cosmos_door_sensor.yaml](../home-assistant/packages/cosmos_door_sensor.yaml)` installed and notifying; fleet/OTA in [cosmos-ha-field](https://github.com/CosmosKiller/cosmos-ha-field)
- [x] Flux carrier: layout + DRC clean; BOM locked in HARDWARE.md
- [x] Flux: R1–R9 / C1–C2 MPNs applied; **D1 and D2** both in BOM (first batch)
- [x] Gerber + BOM + CPL exported from Flux (fab package ready)
- [ ] JLCPCB order + carrier bring-up (reed + both buzzers + both flybacks)



### Firmware modules

Matter endpoints: contact Boolean State, arm/disarm OnOff, **read-only panic indicator** (second contact_sensor — clears when reed closes), **siren OnOff** (remote buzzer + silence), Power Source battery. **Disarm (arm Off)** stops arming/siren, clears panic + siren Matter state, confirm blink; **siren Off** stops buzzer only. Endpoints ship **Fixed Label** names (`Door contact`, `Arm / disarm`, `Panic alarm`, `Siren`) and **semantic tags** so commissioners can tell them apart before HA rename.

| Matter role | HA typical entity | Notes |
|-------------|-------------------|-------|
| Contact | `binary_sensor.*` | Reed open = on (PIR analog on intercom) |
| Arm/disarm | `switch.*` | ON = arm; **OFF = disarm + clear panic/siren + silence** |
| Panic indicator | `binary_sensor.*` | Read-only; ON = intrusion; **OFF when reed closes** (tamper analog) |
| Siren | `switch.*` | ON = buzzer (any automation); **OFF = silence** |
| Battery | `sensor.*` | Power Source % |

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


| XIAO pin | ESP GPIO | Firmware / Kconfig                     | Function                                         |
| -------- | -------- | -------------------------------------- | ------------------------------------------------ |
| D8       | GPIO19   | `CONFIG_BEDSIDE_LAMP_LED_GPIO`         | WS2812 data (RMT) — **10 LEDs**                  |
| D9       | GPIO20   | `CONFIG_BEDSIDE_LAMP_USER_BUTTON_GPIO` | User tact to **GND** + **10 kΩ to 3V3**          |
| BOOT     | GPIO9    | `FACTORY_RESET_BUTTON_PIN`             | Factory reset tact to **GND** + **10 kΩ to 3V3** |
| D0 / A0  | GPIO0    | `CONFIG_COSMOS_BATTERY_ADC_GPIO`       | Battery sense (divider mid-tap)                  |


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
- User tact: XIAO D9 (GPIO20) to GND + **10 kΩ pull-up to 3V3 at D9**. Accessible on top or side of enclosure.
- Factory-reset tact: XIAO BOOT (GPIO9) to GND + **10 kΩ pull-up to 3V3 at BOOT**, long-press ≥ 5 s. Separate from user button; recess or side placement to avoid accidents.

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
| R_PU1      | 1   | **10 kΩ**, 0603                                                             | Pull-up at GPIO20                                          |
| R_PU2      | 1   | **10 kΩ**, 0603                                                             | Pull-up at BOOT / GPIO9                                    |
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


| XIAO pin | ESP GPIO | Firmware                         | Function / wiring                                                                |
| -------- | -------- | -------------------------------- | -------------------------------------------------------------------------------- |
| D9       | GPIO20   | `BUTTON_GPIO_PIN`                | Action tact to **GND** + **10 kΩ to 3V3** (press = LOW)                          |
| BOOT     | GPIO9    | `FACTORY_RESET_BUTTON_PIN`       | Reset tact to **GND** + **10 kΩ to 3V3**; long ≥ 5 s — **not** the action button |
| D3       | GPIO21   | `SINGLE_PRESS_LED_PIN`           | Active-high LED + 330 Ω (single-press feedback)                                  |
| D8       | GPIO19   | `MULTI_PRESS_LED_PIN`            | Active-high LED + 330 Ω (multi-press feedback)                                   |
| D0 / A0  | GPIO0    | `CONFIG_COSMOS_BATTERY_ADC_GPIO` | Battery divider mid-tap (2:1)                                                    |


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
- Primary action tact: between XIAO D9 (GPIO20) and GND. **10 kΩ pull-up to 3V3 at D9.** Large, easy to press.
- Factory-reset tact: between XIAO BOOT (GPIO9) and GND. **10 kΩ pull-up to 3V3 at BOOT.** Recessed or side-mounted; long press ≥ 5 s. Separate from the action button.

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
| R_PU1  | 1   | **10 kΩ**, 0603                                                             | Pull-up at GPIO20               |
| R_PU2  | 1   | **10 kΩ**, 0603                                                             | Pull-up at BOOT / GPIO9         |
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

**Main board:** [Waveshare ESP32-C5-Touch-LCD-2.8](https://docs.waveshare.com/ESP32-C5-Touch-LCD-2.8) (ESP32-C5 + 2.8″ ST7789 + CST3530 touch)  
**Matter role:** Temperature / humidity (SHTC3); gas via carrier **SGP41** (VOC + NOx); local touch UI.  
**PID (test):** `0x8003` — see [MANUFACTURING.md](MANUFACTURING.md).  
**Platform:** agreed 2026-09-20 — [ARCHITECTURE.md](ARCHITECTURE.md). Flux builds an **expansion carrier**, not a second MCU board.

> **Supersedes** prior XIAO ESP32-C5 + bare BME680 + 1.3″ ST7789 + EC11 carrier plan. Interim `iotEnvironmentalSensor` firmware may still target old pins until the C5-Touch port lands.

### Product decisions (locked for v1)


| Item | Choice |
| ---- | ------ |
| Main electronics | Waveshare **ESP32-C5-Touch-LCD-2.8** (docs + kit) |
| Display / touch | Onboard **240×320 ST7789** + **CST3530** (SPI/I2C per Waveshare) |
| Onboard env | **SHTC3** T/H @ I2C `0x70` (SDA GPIO0 / SCL GPIO1 shared bus) |
| Carrier gas | Sensirion **SGP41** (VOC + NOx) — locked |
| Pressure | **Deferred** — not on first carrier |
| Audio | Onboard mic + speaker (Waveshare) |
| Power | Onboard **ETA6098** + **MX1.25** 1S Li-ion option; Waveshare USB-C = product port |
| Flux role | Carrier mating to I2C / SH1.0 / free pins: **SGP41**, mounts, optional extras |
| Encoder | **Dropped** for this platform (touch UI replaces EC11) |
| Factory reset | Use Waveshare BOOT / long-press policy — map in firmware when porting |

### Onboard interfaces (Waveshare — do not reassign)

Shared I2C (`GPIO0` SDA / `GPIO1` SCL): CH32V003 EXIO `0x24`, QMI8658 `0x6B`, PCF85063 `0x51`, SHTC3 `0x70`, CST3530 `0x58`.

| Function | Notes |
| -------- | ----- |
| LCD | ST7789 SPI — SCLK GPIO6, MOSI GPIO7, DC GPIO9, CS GPIO10; RST/BL via CH32 EXIO |
| Touch | CST3530; INT GPIO5; RST via CH32 EXIO0 |
| Battery ADC | CH32 **EXIO_ADC** (BAT_ADC) — prefer Waveshare path over a second divider |
| Expansion | I2C header + **SH1.0 12PIN** — primary Flux carrier attachment |

**Carrier GPIO map:** TBD after header pinout lock with firmware (do not invent production pins yet).

### Flux.ai project prompt

```text
Design a 2-layer expansion carrier for "Cosmos iotEnvironmentalSensor" that mates to the Waveshare ESP32-C5-Touch-LCD-2.8 (main board). Do NOT place a second MCU or display.

Main board (already chosen — not on this PCB):
- Waveshare ESP32-C5-Touch-LCD-2.8: ESP32-C5, 2.8" ST7789 + CST3530, SHTC3, mic/speaker, ETA6098 + MX1.25 1S, USB-C.
- Docs: https://docs.waveshare.com/ESP32-C5-Touch-LCD-2.8

Carrier goals:
- Host Sensirion **SGP41** (VOC + NOx) on the shared I2C bus brought out from the Waveshare I2C or SH1.0 header.
- Pressure sensor: **deferred** — do not place on first carrier.
- Mechanical: stand / wall mount / cable strain for desk use; keep airflow to SHTC3 and SGP41 away from heat sources.
- Power: take 3V3 + GND from main board only. Do not add USB-C or a second charger. Do not dual-feed VBUS/BAT.
- I2C: 4.7 kΩ pull-ups only if the main board does not already provide them on the expansion connector — verify Waveshare schematic before duplicating.
- Silkscreen: 3V3, GND, SDA, SCL, SGP ADDR notes, REV, product name.
- 2 layers, 1.6 mm FR4, 1 oz, JLCPCB-friendly, 0603 passives.

Do not reintroduce BME680, EC11, or a separate ST7789 module on this carrier.
```

### Bill of materials (prototype)

| Ref | Qty | Description | Notes |
| --- | --- | ----------- | ----- |
| U_MAIN | 1 | Waveshare **ESP32-C5-Touch-LCD-2.8** (+ batt option) | Main board — not Flux fab |
| BAT1 | 0–1 | 1S Li-ion MX1.25 (Waveshare option) | ETA6098 on main board |
| U_SGP | 1 | Sensirion **SGP41** | Carrier — VOC + NOx |
| R_I2C | 0–2 | 4.7 kΩ, 0603 | Only if header lacks pulls |
| C_SGP | 1–2 | 100 nF, 0603 | Local decoupling |
| J_HDR | 1 | Mating connector for Waveshare I2C / SH1.0 | Per Waveshare pinout |
| U_PRESS | 0 | Sensirion pressure | **Deferred** — omit first fab |
| — | — | Enclosure / stand | Mechanical |

### Bring-up checklist

- [ ] Devkit: SHTC3 reads T/H; ETA6098 battery path works
- [ ] Matter temp/humidity (retire BME680 pressure until pressure MPN locked)
- [ ] Carrier: **SGP41** on I2C; HA VOC + NOx / air-quality entities
- [ ] Touch UI / LVGL path (later)
- [ ] OTA image builds for `esp32c5`

### Firmware modules

Target: `esp32c5` on Waveshare BSP. Replace `bme680_task` with **SHTC3** (+ **SGP41** VOC/NOx). Display/touch via Waveshare ST7789 + CST3530 + CH32 EXIO. OTA via `cosmos_matter_ota`. Battery sense via Waveshare BAT_ADC path when enabled.


---


## iotDoorIntercom (SKU 5)

**Main board:** [Waveshare ESP32-P4-WIFI6](https://docs.waveshare.com/ESP32-P4-WIFI6) (ESP32-P4NRW32 + **ESP32-C6-MINI-1** Wi-Fi 6/BLE)  
**Matter role:** Matter **1.5 camera** (**video + 2-way audio bring-up**) + doorbell / PIR / tamper / siren (carrier).  
**PID (test):** `0x8005` — see [MANUFACTURING.md](MANUFACTURING.md).  
**HA:** **No package for now** — use Matter Live View + native doorbell / PIR / tamper / siren entities.  
**Platform:** agreed 2026-09-20 — [ARCHITECTURE.md](ARCHITECTURE.md) ADR-015.

> **HTTPS MJPEG** is **SKU 6** only. SKU 5 product video is Matter 1.5 on P4+C6.

### Product decisions (locked for v1)


| Item | Choice |
| ---- | ------ |
| Main electronics | Waveshare **ESP32-P4-WIFI6** (P4 + C6) |
| Camera | **MIPI-CSI** on P4 — kit: **Waveshare RPi Camera (B) Rev 2.0 (OV5647)** |
| Matter / Wi-Fi | **ESP32-C6** companion — Matter signaling + Wi-Fi 6 |
| Video | Matter Live View (WebRTC) |
| Audio / 2-way | Onboard **ES8311** + **NS4150B**; Matter volume defaults **254**; attrs drive codec via `BRIDGE_CMD_SET_AUDIO` |
| Matter entities | Doorbell, PIR occupancy, tamper contact, siren OnOff (+ camera A/V) |
| HA package | **None for now** |
| Flux carrier | Doorbell, AM312 PIR, tamper (leaf + pogo), siren LED/piezo, 1S battery as needed |
| Form | Outdoor doorbell / wall-mount |
| Door lock | None for v1 |

### SoC split (Espressif Matter camera)

| Chip | Role |
| ---- | ---- |
| **ESP32-P4** | MIPI-CSI capture, ISP, H.264, WebRTC media, audio (ES8311) |
| **ESP32-C6** | Wi-Fi 6 + BLE, Matter stack / signaling (Audio + Speaker features) |

Expect **two firmware images** (media_adapter on P4, matter_camera on C6) per Espressif docs. Security I/O lives on the **P4 40-pin header** — GPIO map below is firmware source of truth (do not reuse XIAO S3 Sense pins).

### Onboard audio (Waveshare — do not reassign)

Same pinout as Espressif Function EV BSP (`espressif/esp32_p4_function_ev_board`); SKU5 keeps that BSP selected.

| Signal | P4 GPIO | Notes |
| ------ | ------- | ----- |
| I2C SDA | **GPIO7** | Shared I2C0 (codec + camera SCCB) |
| I2C SCL | **GPIO8** | ES8311 7-bit addr **0x18** |
| I2S MCLK | **GPIO13** | ES8311 master clock |
| I2S SCLK | **GPIO12** | Bit clock |
| I2S LRCK / WS | **GPIO10** | Frame sync |
| I2S DOUT → codec | **GPIO9** | Playback (DSDIN) |
| I2S DIN ← codec | **GPIO11** | Mic capture (ASDOUT) |
| PA enable | **GPIO53** | **NS4150B**, active high |

Speaker connector: MX1.25 2P, **8 Ω / 2 W**. Mic is onboard (analog into ES8311). Matter Camera AVSM **MicrophoneVolumeLevel** / **SpeakerVolumeLevel** (default **254**, Min=1 Max=254) are applied on P4 via `esp_codec_dev_set_in_gain` / `set_out_vol` (+ mute); mic PGA uses ceil-to-6 dB steps up to **42 dB**; PA enable follows codec mute.

### GPIO map (must match firmware)

P4 expansion header (Pico-compatible). Pulls at the MCU pin on the carrier.

| Function | P4 GPIO | Pull / drive | Firmware |
| -------- | ------- | ------------ | -------- |
| Doorbell | **GPIO27** | Tact to **GND**; internal/external pull-up (pressed = LOW). Also accepts tact→3V3 + PD. | `DOORBELL_PIN` |
| PIR | **GPIO32** | External 10 kΩ to GND; AM312 OUT | `PIR_PIN` |
| Tamper | **GPIO33** | External 10 kΩ to 3V3; NC to GND when seated (open = HIGH) | `TAMPER_PIN` |
| Siren | **GPIO21** | Active-high via NPN + piezo / LED (header pin 15) | `ALARM_LED_PIN` |
| Status LED | **GPIO22** | Active-high LED (header — confirm silk) | `LED_PIN` |
| Factory reset | **GPIO28** | External 10 kΩ to 3V3; tact to GND (carrier recessed — not board BOOT) | `CONFIG_FACTORY_RESET_BUTTON_GPIO` (P4 sense → C6) |
| Battery ADC (later) | **GPIO20** | 2:1 divider → ADC1_CH4 | `CONFIG_COSMOS_BATTERY_ADC_GPIO` |

### Reserved pins (do not use for carrier I/O)

| Pins | Why |
| ---- | --- |
| GPIO14–19 | ESP-Hosted SDIO to C6 |
| GPIO54 | C6 reset (Hosted) |
| GPIO37 / 38 | UART0 console (USB-UART) |
| GPIO7 / 8 | Board I2C0 (ES8311 + camera SCCB) |
| GPIO9–13 | Board I2S (ES8311) |
| GPIO53 | NS4150B PA enable |
| GPIO35 | On-board BOOT |
| GPIO39–48 | MicroSD / SDMMC bank (incl. default SD1 data on 39–48) |
| GPIO24 / 25 | USB |

**Conflict note:** Function EV BSP maps **GPIO27** to LCD_RST when `CONFIG_BSP_LCD_TYPE_1024_600=y`. SKU5 keeps `CONFIG_MEDIA_STREAM_ENABLE_VIDEO_PLAYER` **off** so display init never claims GPIO27 — CosmOS doorbell stays free. Do not enable the video player on this kit without relocating doorbell.

### Tamper electromechanical detail (carrier — unchanged intent)

Seated = path to GND; open = pull-up HIGH → latched panic + Matter contact open. Leaf spring + back pogo pads in series recommended. Firmware does **not** clear siren on remount — HA turns Off siren OnOff.

### Flux.ai project prompt

```text
Design a 2-layer outdoor expansion carrier for "Cosmos iotDoorIntercom" that mates to the Waveshare ESP32-P4-WIFI6 main board. Do NOT place a second Wi-Fi MCU or replace the P4 camera path.

Main board (already chosen — not fabricated here):
- Waveshare ESP32-P4-WIFI6: ESP32-P4 + ESP32-C6-MINI-1, MIPI-CSI camera, MIPI-DSI, mic/speaker, USB-C, TF, 40-pin GPIO expansion.
- Docs: https://docs.waveshare.com/ESP32-P4-WIFI6
- Product camera = MIPI-CSI on P4 (Matter 1.5 camera / WebRTC). Do not design for XIAO S3 Sense DVP.

Locked P4 GPIO map (must match firmware — HARDWARE.md):
- GPIO27 DOORBELL — tact to 3V3, 10 kΩ pull-down at pin
- GPIO32 PIR — AM312 OUT, 10 kΩ pull-down at pin
- GPIO33 TAMPER — NC to GND when seated, 10 kΩ pull-up to 3V3 (open = HIGH)
- GPIO21 SIREN — NPN (S8050) + 1 kΩ base → red LED 330 Ω + 3–5 V active piezo from 3V3/BAT+; flyback diode
- GPIO22 STATUS LED — active-high
- GPIO28 FACTORY RESET — recessed tact to GND, 10 kΩ pull-up to 3V3 (not board BOOT GPIO35)
- GPIO20 BATTERY ADC (optional) — 2:1 divider mid-tap

Do NOT use: GPIO14–19 (Hosted SDIO), GPIO54 (C6 reset), GPIO37/38 (UART0), GPIO7/8 (board I2C / ES8311), GPIO9–13 (board I2S), GPIO53 (PA enable), GPIO35 (BOOT), GPIO39–48 (TF/SDMMC), GPIO24/25 (USB).

Carrier goals:
- Outdoor doorbell / wall-mount (~60×100 mm class). Gasketed enclosure, camera window aligned to the P4 CSI module / flex.
- Fresnel window for PIR; keep siren/LED optical isolation from PIR.
- Tamper: leaf spring to chassis GND when seated + two gold/pogo pads on PCB back.
- Power: prefer single USB-C charge path (main board and/or carrier J1) into 1S charger + protect → pouch on JST-PH 2.0. No dual-feed. Size for >=1 A camera + Wi-Fi peaks.
- Mate to P4-WIFI6 40-pin header; silkscreen GPIO numbers above.
- 2 layers, 1.6 mm FR4, 1 oz, conformal-coat friendly, JLCPCB 0603 passives.
- Silkscreen: BAT+, GND, 3V3, DOORBELL, PIR, TAMPER, SIREN, STATUS, RESET, REV.

Do not keep the XIAO S3 Sense (OV3660 DVP) as the camera solution on this carrier.
```

### Bill of materials (prototype)

| Ref | Qty | Description | Notes |
| --- | --- | ----------- | ----- |
| U_MAIN | 1 | Waveshare **ESP32-P4-WIFI6** (+ camera module) | Main board — kit |
| CAM1 | 1 | MIPI-CSI camera (OV5647-class or Waveshare kit cam) | On P4 CSI |
| SW1 | 1 | Doorbell tact (weatherized) | Carrier |
| SW2 | 1 | Recessed tact | Factory reset |
| U2 | 1 | AM312 (or 3.3 V mini PIR) | Carrier |
| SW3 | 1 | Leaf spring / chassis contact | Tamper |
| PAD1, PAD2 | 2 | Gold / pogo pads (back) | Case closed |
| Q1 | 1 | S8050 SOT-23 | Siren / LED |
| R_B | 1 | 1 kΩ, 0603 | Base |
| D_ALM | 1 | Red LED, 0603 | |
| R_LED | 1 | 330 Ω, 0603 | |
| BZ1 | 1 | 3–5 V active piezo | + flyback |
| D_FB | 1 | 1N4148W | Flyback |
| R_PDx / R_PUx | 4 | 10 kΩ, 0603 | External pulls |
| J1 | 0–1 | USB-C | Only if charge path is on carrier |
| J2 | 1 | JST-PH 2.0 | 1S pouch |
| U3 | 0–1 | 1S charger + protect | If not using main-board-only power |
| R1, R2 | 2 | 100 kΩ 1% | Battery divider if ADC on carrier |
| C1 | 1 | 100 nF | ADC filter |
| BAT1 | 1 | 1S Li-ion pouch | Outdoor runtime |
| — | — | Outdoor enclosure, gaskets, Fresnel, pogos | Mechanical |

### Bring-up checklist

- [ ] P4-WIFI6 kit: Matter 1.5 camera example (P4 media + C6 Matter) streams
- [ ] Carrier: doorbell / PIR / tamper / siren on locked GPIOs
- [ ] Tamper leaf + pogo path; HA clears siren via OnOff
- [ ] 1S charge path; battery % if enabled
- [ ] Outdoor enclosure RF / camera window soak
- [ ] HA package updated for Matter camera (not MJPEG)

### Firmware modules

**Target:** dual image — P4 `media_adapter` + C6 `matter_camera` (esp-matter camera). Security tasks use the locked P4 GPIO map above.

**App today:** `iotDoorIntercom/` — P4-side I/O source of truth (**Doorbell / PIR / Tamper / Siren** Fixed Labels + legacy OnOff call/media gate). No MJPEG (`iotSecurityCamera` / SKU 6). Dual-image Matter camera merge is separate.

---

## iotSecurityCamera (SKU 6)

**Module:** [Seeed XIAO ESP32-S3 Sense](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/) (OV3660 DVP, PID `0x3660`)  
**Matter role:** OnOff **stream gate** + OnOff **siren** + CSI occupancy (no Matter camera cluster).  
**Video:** HTTPS MJPEG `GET /stream` and still JPEG `GET /capture` (both follow the stream gate).  
**PID (test):** `0x8006` — see [MANUFACTURING.md](MANUFACTURING.md).  
**HA:** Package wraps the MJPEG stream (UI MJPEG IP Camera + OnOff gate helpers).  
**Platform:** locked 2026-09-20 — [ARCHITECTURE.md](ARCHITECTURE.md) ADR-014.

### Product decisions (locked for v1)

| Item | Choice |
| ---- | ------ |
| Main electronics | Seeed **XIAO ESP32-S3 Sense** |
| Camera (now) | Onboard **OV3660** → HTTPS MJPEG `/stream` |
| Snapshot | **`GET /capture`** — one JPEG, same stream gate as `/stream` |
| Matter | Stream-gate OnOff + siren OnOff + CSI occupancy (+ battery if kept); **no** doorbell / PIR / tamper |
| Siren | **GPIO1 (D0)** — same blink as SKU 5; latched until the Matter siren OnOff is Off |
| Presence | **Lab:** [espectre](https://github.com/francescopace/espectre) CSI → Matter occupancy (`kRFSensing`). Not field-validated |
| Form | Compact wall / shelf security cam |
| Out of scope | Doorbell, PIR, tamper (SKU 5 only) |

### Controls (v1)

| Signal | Function |
|--------|----------|
| Stream gate | Matter OnOff — enables / disables MJPEG clients |
| Siren | Matter OnOff — GPIO1 (XIAO D0), active-high. NPN + piezo; do not source the buzzer from the pin |
| Presence | Matter occupancy — Wi-Fi CSI (ESPectre), no extra GPIO |
| Factory reset | GPIO0 (BOOT) long-press |
| HTTPS cert | Embedded Beta self-signed (`main/certs/`) |

### Bring-up checklist

- [x] HTTPS MJPEG streams; HA MJPEG IP Camera entity
- [x] Matter stream gate
- [ ] Slim firmware: remove inherited doorbell / PIR / tamper / siren tasks
- [x] Snapshot endpoint `GET /capture`
- [ ] Presence on hardware: ESPectre CSI occupancy while `/stream` is on
- [x] Siren blink on GPIO1 (D0); Matter OnOff starts and clears it

### GPIO map (must match firmware)

Camera DVP pins stay in `cam_task.h`. Do not reuse them for the siren.

| XIAO pin | ESP GPIO | Firmware | Function |
| -------- | -------- | -------- | -------- |
| D0 | **GPIO1** | `ALARM_LED_PIN` | Siren, active-high via NPN |
| D4 | **GPIO5** | `CONFIG_COSMOS_BATTERY_ADC_GPIO` | Battery ADC |
| BOOT | **GPIO0** | `CONFIG_FACTORY_RESET_BUTTON_GPIO` | Factory reset long-press |

### Firmware modules

**App:** `iotSecurityCamera/` — `cam_task` + `http_stream_task` + Matter stream-gate OnOff + siren OnOff (`panic_alarm_task`) + `csi_presence_task` (ESPectre). Doorbell / PIR / tamper are not in this app.

