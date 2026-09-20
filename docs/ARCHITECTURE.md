# Architecture — cosmosFivePieceBasis

Human ↔ Architect agreement. Platform choices, HW/SW partition, and phased plan. Update when goals or MCU decisions change.

**Language:** English only.

---

## Status

| Field | Value |
|-------|--------|
| Status | `agreed` (SKU 3 / 5 platform upgrade Sep 2026); SKU 1 / 2 / 4 unchanged |
| Last updated | 2026-09-20 |
| Architect | Cosmos Architect |
| Human owner | cosmoskiller |

---

## 1. Problem and product intent

### One-sentence product

A five-SKU Matter / Home Assistant device family: door contact + alarm, dual-mode button, environmental desk display, bedside lamp, and outdoor door intercom with camera.

### Success criteria

- [x] SKU 1 carrier fab-ready (Gerber/BOM/CPL)
- [ ] SKU 3 on Waveshare C5 touch LCD + Flux expansion carrier (SHTC3 + SGP41)
- [ ] SKU 5 on Waveshare ESP32-P4-WIFI6 + Matter 1.5 camera + Flux outdoor carrier
- [ ] HA packages commission and operate each SKU

### Explicit non-goals

- Do not change SKU **1 / 2 / 4** platform or carriers in this upgrade.
- Do not keep BME680 or XIAO ESP32-C5 / XIAO ESP32-S3 Sense as the long-term SKU 3 / 5 targets.
- Do not invent production GPIO for free header pins until carrier pin map is locked with firmware.

---

## 2. Constraints

| Area | Constraint | Notes |
|------|------------|-------|
| Cost | Prefer Waveshare main boards + small Flux carriers | Avoid full custom MCU PCBs for 3 / 5 v1 |
| Power / battery | SKU 3: Waveshare **ETA6098** + MX1.25 1S; SKU 5: portable 1S + USB charge | Anti-leakage rules in HARDWARE.md |
| Size / enclosure | SKU 3 desk; SKU 5 outdoor doorbell | Carrier form TBD in Flux |
| Connectivity | Matter over Wi-Fi; SKU 5 Matter **1.5 camera** | P4 + C6 split is Espressif reference |
| Certifications / protocol | Test VID/PID family; production cert later | |
| Schedule | Devkits on hand for bring-up | Firmware port after docs lock |
| Supply chain | Waveshare + Sensirion + JLCPCB | |

---

## 3. Platform agreement (required)

| Decision | Choice | Rationale |
|----------|--------|-----------|
| SKU 1 / 2 / 4 | **Unchanged** (XIAO C6 / existing plans) | Explicit human lock |
| SKU 3 main board | [Waveshare ESP32-C5-Touch-LCD-2.8](https://docs.waveshare.com/ESP32-C5-Touch-LCD-2.8) | Built-in display, touch, SHTC3, mic/speaker, dual-band Wi-Fi, free headers |
| SKU 3 MCU | **ESP32-C5** | Matter + dual-band; IDF target `esp32c5` |
| SKU 3 charger | Onboard **ETA6098** + MX1.25 1S Li-ion option | Human choice — use Waveshare battery path |
| SKU 3 sensors | Onboard **SHTC3** (T/H); carrier **Sensirion SGP41** (VOC + NOx) | Pressure **deferred** |
| SKU 3 Flux role | **Expansion carrier only** | Gas (**SGP41**), mounts, extra I/O — not a second MCU; pressure deferred |
| SKU 5 main board | [Waveshare ESP32-P4-WIFI6](https://docs.waveshare.com/ESP32-P4-WIFI6) (P4 + ESP32-C6-MINI-1) | Espressif Matter camera class; kit on hand |
| SKU 5 MCU split | **P4** = media / H.264 / camera; **C6** = Wi-Fi 6 + Matter signaling | Matches esp-matter camera example |
| SKU 5 camera goal | **Matter 1.5 camera soon** (WebRTC) | Supersedes S3 Sense MJPEG as product target |
| SKU 5 Flux role | **Outdoor expansion carrier** | Doorbell, PIR, tamper, siren, battery/power as needed |
| Framework / SDK | **ESP-IDF** + **esp-matter** | P4: prefer IDF (Arduino limited); C5: IDF |
| Language | C / C++ | |

**Agreement checklist**

- [x] Human and Architect agree on SKU 3 / 5 main boards
- [x] Flux = carrier / expand capabilities (not replace main board)
- [x] SKU 5 → Matter 1.5 camera path
- [x] SKU 3 → ETA6098 battery option + **SGP41** on carrier
- [x] Do not touch SKU 1 / 2 / 4
- [ ] Pin maps for carrier headers locked with firmware (next)
- [ ] BUILD.md / MANUFACTURING.md updated for new targets after first bring-up

Once carrier GPIOs are locked, keep [HARDWARE.md](HARDWARE.md) as GPIO source of truth and align firmware.

---

## 4. System context

### Actors and interfaces

| Actor / system | Interface | Direction | Notes |
|----------------|-----------|-----------|-------|
| Home Assistant | Matter / Wi-Fi | ↔ | Commission + entities |
| User (SKU 3) | Touch LCD + optional carrier sensors | → | Local UI + env data |
| User (SKU 5) | Doorbell / PIR / camera / siren | → | Security + video |
| Waveshare main PCB | USB-C, battery, radios, onboard sensors | — | Product compute |
| Flux carrier | Headers / sensors / outdoor I/O | — | Expansion only |

### High-level block diagrams

**SKU 3**

```text
[ SGP41 (VOC+NOx) ] --I2C/headers--> [ Waveshare C5-Touch-LCD-2.8 ]
        Flux carrier                              │
                                    ST7789+touch, SHTC3, mic/spk
                                    ETA6098 + 1S, USB-C
                                    ESP32-C5 Matter / Wi-Fi
```

**SKU 5**

```text
[ doorbell/PIR/tamper/siren/BAT ] --> [ Waveshare ESP32-P4-WIFI6 ]
           Flux carrier                      │
                              P4: MIPI-CSI cam, H.264, audio
                              C6: Wi-Fi 6 + Matter 1.5 camera signaling
```

### HW / SW partition

| Responsibility | Hardware | Firmware | Notes |
|----------------|----------|----------|-------|
| Display / touch (SKU 3) | Waveshare onboard | LVGL / Matter UI later | CH32 EXIO for RST/BL |
| T/H | SHTC3 onboard | Replace `bme680_task` | I2C 0x70 |
| Gas | **SGP41** on carrier | New task (VOC + NOx indices) | Shared I2C bus careful |
| Camera / encode (SKU 5) | P4 + MIPI-CSI | media_adapter path | Matter 1.5 |
| Matter / Wi-Fi (SKU 5) | C6 on WIFI6 board | matter_camera path | Dual image flash |
| Security I/O (SKU 5) | Carrier | Port from S3 Sense app | New pin map TBD |

---

## 5. Architecture decisions (ADRs lite)

| ID | Decision | Status | Consequences |
|----|----------|--------|--------------|
| ADR-010 | SKU 3/5 use Waveshare **main boards**; Flux builds **carriers** | accepted | Smaller ECAD scope; depend on Waveshare pinouts/docs |
| ADR-011 | SKU 5 targets **Matter 1.5 camera** (P4+C6), not MJPEG-only | accepted | Dual firmware; retire S3 Sense as product MCU |
| ADR-012 | SKU 3 env sensing = **SHTC3 + SGP41**; BME680 retired; **pressure deferred** | accepted | VOC + NOx on carrier; no Matter pressure until later |
| ADR-013 | SKU 3 power = Waveshare **ETA6098** + MX1.25 1S | accepted | Carrier need not duplicate charger unless extra loads require it |
| ADR-001… | Prior XIAO-centric SKU 3/5 decisions | superseded | See HARDWARE history / git |

---

## 6. Risks and open questions

| Risk / question | Impact | Mitigation / owner | Status |
|-----------------|--------|--------------------|--------|
| Pressure on SKU 3 | Matter P entity | **Deferred** — no pressure on first carrier | closed |
| SGP40 vs SGP41 | BOM / NOx | **SGP41 locked** (VOC + NOx) | closed |
| P4 dual-image flash / OTA complexity | Manufacturing | Follow esp-matter camera docs; kit bring-up first | open |
| Free GPIO / SH1.0 pin assign for carriers | Firmware lock | Map after header pinout read; update HARDWARE | open |
| Interim S3 Sense firmware vs P4 target | Dual maintenance | Keep S3 for field until P4 Matter camera works | open |

---

## 7. Action plan

### Phase 0 — Align

- [x] Agree SKU 3 / 5 platforms with human
- [x] Record agreement in this file + HARDWARE.md
- [x] Gas = **SGP41**; pressure deferred

### Phase 1 — Devkit bring-up

- [ ] SKU 3: ESP-IDF on C5-Touch-LCD-2.8 — SHTC3, display, ETA6098 battery sense
- [ ] SKU 5: ESP32-P4-WIFI6 kit — Matter camera example (P4 + C6)

### Phase 2 — Firmware port

- [ ] Port `iotEnvironmentalSensor` off BME680 → SHTC3 (+ SGP later)
- [ ] Port / replace `iotDoorIntercom` camera path toward Matter 1.5

### Phase 3 — Flux carriers

- [ ] SKU 3 carrier: **SGP41**, header mating, mounts
- [ ] SKU 5 carrier: doorbell, PIR, tamper, siren, power as needed

### Phase 4 — HA / manufacturing

- [ ] Update HA packages for new entities
- [ ] Update BUILD.md / MANUFACTURING.md targets when builds exist
