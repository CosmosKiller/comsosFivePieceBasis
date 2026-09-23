# Architecture — cosmosFivePieceBasis

Human ↔ Architect agreement. Platform choices, HW/SW partition, and phased plan. Update when goals or MCU decisions change.

**Language:** English only.

---

## Status

| Field | Value |
|-------|--------|
| Status | `agreed` (SKU 5 / 6 product wrap-up Sep 2026); SKU 1 / 2 / 4 unchanged |
| Last updated | 2026-09-20 |
| Architect | Cosmos Architect |
| Human owner | cosmoskiller |

---

## 1. Problem and product intent

### One-sentence product

A **six-SKU** Matter / Home Assistant device family: door contact + alarm, dual-mode button, environmental desk display, bedside lamp, outdoor **Matter camera intercom** (video now, 2-way later), and **HTTPS MJPEG security camera** (HA `camera.*` wrapper; still JPEG `/capture`; siren OnOff).

### Success criteria

- [x] SKU 1 carrier fab-ready (Gerber/BOM/CPL)
- [ ] SKU 3 on Waveshare C5 touch LCD + Flux expansion carrier (SHTC3 + SGP41)
- [ ] SKU 5 on Waveshare ESP32-P4-WIFI6 + Matter 1.5 camera + Flux outdoor carrier
- [x] SKU 6 = `iotSecurityCamera` on XIAO ESP32-S3 Sense (HTTPS MJPEG migrated from former SKU 5 interim)
- [ ] HA packages where needed (SKU 6 MJPEG wrapper; SKU 5 uses Matter Live View — **no HA package for now**)

### Explicit non-goals

- Do not change SKU **1 / 2 / 4** platform or carriers in this upgrade.
- Do not keep BME680 or XIAO ESP32-C5 as the long-term SKU 3 target.
- Do not treat S3 Sense MJPEG as the SKU 5 intercom product camera — that path is SKU 6.
- Do not ship an HA package for SKU 5 until Matter Live View / intercom UX needs helpers.
- Do not invent production GPIO for free header pins until carrier pin map is locked with firmware.

---

## 2. Constraints

| Area | Constraint | Notes |
|------|------------|-------|
| Cost | Prefer Waveshare main boards + small Flux carriers | Avoid full custom MCU PCBs for 3 / 5 v1 |
| Power / battery | SKU 3: Waveshare **ETA6098** + MX1.25 1S; SKU 5: portable 1S + USB charge | Anti-leakage rules in HARDWARE.md |
| Size / enclosure | SKU 3 desk; SKU 5 outdoor doorbell; SKU 6 compact cam | Carrier form TBD in Flux |
| Connectivity | Matter over Wi-Fi; SKU 5 Matter **1.5 camera**; SKU 6 MJPEG + HA `camera.*` | P4 + C6 split is Espressif reference |
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
| SKU 5 camera goal | **Matter 1.5 camera** — **video + 2-way audio bring-up** | Intercom Live View via Matter; not MJPEG |
| SKU 5 Matter entities | **Doorbell + PIR + tamper + siren** (carrier) + Matter camera | Native Matter; **no HA package for now** |
| SKU 5 Flux role | **Outdoor expansion carrier** | Doorbell, PIR, tamper, siren, battery/power as needed |
| SKU 6 main board | **Seeed XIAO ESP32-S3 Sense** (OV3660 DVP) | HTTPS MJPEG; HA `camera.*` via MJPEG IP Camera |
| SKU 6 product role | **MJPEG security camera** | Video + still JPEG + siren OnOff; **no doorbell / PIR / tamper** |
| SKU 6 deferred | **Siren** + **presence** ([espectre](https://github.com/francescopace/espectre) CSI) | Not in v1 feature set |
| SKU 6 HA | **Package = stream wrapper** (OnOff gate + MJPEG UI camera) | No Matter native camera entity |
| SKU 6 firmware app | **`iotSecurityCamera/`** | Migrated from former S3 intercom MJPEG stack |
| Framework / SDK | **ESP-IDF** + **esp-matter** | P4: prefer IDF (Arduino limited); C5: IDF |
| Language | C / C++ | |

**Agreement checklist**

- [x] Human and Architect agree on SKU 3 / 5 main boards
- [x] Flux = carrier / expand capabilities (not replace main board)
- [x] SKU 5 → Matter camera (video + 2-way audio bring-up) + doorbell/PIR/tamper/siren; **no HA package**
- [x] SKU 6 → MJPEG cam + HA stream wrapper; no doorbell/PIR/tamper; siren/presence later
- [x] SKU 3 → ETA6098 battery option + **SGP41** on carrier
- [x] Do not touch SKU 1 / 2 / 4
- [ ] Pin maps for carrier headers locked with firmware (SKU 5 done; SKU 3 next)
- [ ] BUILD.md / MANUFACTURING.md updated for new targets after first bring-up

Once carrier GPIOs are locked, keep [HARDWARE.md](HARDWARE.md) as GPIO source of truth and align firmware.

---

## 4. System context

### Actors and interfaces

| Actor / system | Interface | Direction | Notes |
|----------------|-----------|-----------|-------|
| Home Assistant | Matter / Wi-Fi | ↔ | Commission + entities |
| User (SKU 3) | Touch LCD + optional carrier sensors | → | Local UI + env data |
| User (SKU 5) | Doorbell / PIR / tamper / siren + Matter Live View (A/V) | → | 2-way audio bring-up; no HA package |
| User (SKU 6) | HTTPS MJPEG + still JPEG + siren | → | HA package wraps stream; still URL `/capture`; siren OnOff |
| Waveshare main PCB | USB-C, battery, radios, onboard sensors | — | Product compute (3 / 5) |
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

**SKU 6**

```text
[ XIAO ESP32-S3 Sense + OV3660 ]
              │
   Matter OnOff stream gate + HTTPS MJPEG /stream
   HA: MJPEG IP Camera entity (no Matter camera cluster)
   Still JPEG GET /capture (same stream gate); CSI presence (espectre)
   Siren OnOff on GPIO1 (D0), same blink as SKU 5
   Out of scope: doorbell / PIR / tamper
```

### HW / SW partition

| Responsibility | Hardware | Firmware | Notes |
|----------------|----------|----------|-------|
| Display / touch (SKU 3) | Waveshare onboard | LVGL / Matter UI later | CH32 EXIO for RST/BL |
| T/H | SHTC3 onboard | Replace `bme680_task` | I2C 0x70 |
| Gas | **SGP41** on carrier | New task (VOC + NOx indices) | Shared I2C bus careful |
| Camera / encode (SKU 5) | P4 + MIPI-CSI | media_adapter path | Matter 1.5 |
| Matter / Wi-Fi (SKU 5) | C6 on WIFI6 board | matter_camera path | Dual image flash |
| Security I/O (SKU 5) | Carrier → P4 GPIO27/32/33/46/21/22 | `iotDoorIntercom/` tasks | Locked map in HARDWARE.md |
| MJPEG camera (SKU 6) | S3 Sense OV3660 | `cam_task` + `http_stream_task` | HA MJPEG IP Camera + OnOff gate |
| Snapshot (SKU 6) | S3 Sense OV3660 | `GET /capture` on the HTTPS server | Same stream gate as `/stream`; HA still-image URL |
| Siren (SKU 6) | GPIO1 (XIAO D0) | `panic_alarm_task` | Matter OnOff; same blink as SKU 5; no tamper auto-start |
| Doorbell/PIR/tamper (SKU 6) | — | **Out of scope** | Belong to SKU 5 only |

---

## 5. Architecture decisions (ADRs lite)

| ID | Decision | Status | Consequences |
|----|----------|--------|--------------|
| ADR-010 | SKU 3/5 use Waveshare **main boards**; Flux builds **carriers** | accepted | Smaller ECAD scope; depend on Waveshare pinouts/docs |
| ADR-011 | SKU 5 targets **Matter 1.5 camera** (P4+C6), not MJPEG-only | accepted | Dual firmware; intercom Live View via Matter |
| ADR-012 | SKU 3 env sensing = **SHTC3 + SGP41**; BME680 retired; **pressure deferred** | accepted | VOC + NOx on carrier; no Matter pressure until later |
| ADR-013 | SKU 3 power = Waveshare **ETA6098** + MX1.25 1S | accepted | Carrier need not duplicate charger unless extra loads require it |
| ADR-014 | SKU 6 = MJPEG security cam + HA stream wrapper; no doorbell/PIR/tamper; siren/presence later | accepted | Six-SKU; HA `camera.*` only path without Matter camera entity |
| ADR-015 | SKU 5 = Matter camera (video + 2-way audio bring-up) + doorbell/PIR/tamper/siren; **no HA package** | accepted | Controllers use Matter Live View + native entities; ES8311 path via Function EV BSP |
| ADR-001… | Prior XIAO-centric SKU 3/5 decisions | superseded | See HARDWARE history / git |

---

## 6. Risks and open questions

| Risk / question | Impact | Mitigation / owner | Status |
|-----------------|--------|--------------------|--------|
| Pressure on SKU 3 | Missing P entity | **Deferred** — no pressure on first carrier | closed |
| SGP40 vs SGP41 | BOM / NOx | **SGP41 locked** (VOC + NOx) | closed |
| P4 dual-image flash / OTA complexity | Manufacturing | Follow esp-matter camera docs; kit bring-up first | open |
| Free GPIO / SH1.0 pin assign for carriers | Firmware lock | **SKU 5 locked** (HARDWARE.md); SKU 3 still open | open (SKU 3) |
| Google Home / HA Matter camera entity gaps | SKU 5 Live View UX | HA Live View works; Google may lack Matter cam; SKU 6 covers HA `camera.*` | open |
| espectre CSI presence maturity | SKU 6 roadmap | Track upstream; keep PIR until CSI presence ships | open |

---

## 7. Action plan

### Phase 0 — Align

- [x] Agree SKU 3 / 5 platforms with human
- [x] Lock SKU 5 / 6 product wrap-up (Matter cam + entities vs MJPEG + HA wrapper)
- [x] Migrate MJPEG firmware to `iotSecurityCamera/`; strip cam from `iotDoorIntercom`

### Phase 1 — Devkit bring-up

- [ ] SKU 3: ESP-IDF on C5-Touch-LCD-2.8 — SHTC3, display, ETA6098 battery sense
- [x] SKU 5: ESP32-P4-WIFI6 kit — Matter camera **split mode** (C6 `matter_camera` + P4 `streaming_only` / KVS) — CosmOS glue in `iotDoorIntercom/{c6,p4}`; see [BUILD.md](BUILD.md#sku-5--waveshare-esp32-p4-wifi6-matter-15-camera)
  - [x] Clone `esp-port-for-amazon-kvs-sdk` → `KVS_SDK_PATH`
  - [x] Build/flash C6 signaling; build/flash P4 media (video Live View verified)
  - [ ] WebRTC **2-way audio** verified in Live View (ES8311; plug speaker; talk back)
  - [ ] Flux carrier / security I/O field wiring
- [x] SKU 6: S3 Sense HTTPS MJPEG field path (app = `iotSecurityCamera`)

### Phase 2 — Firmware port

- [ ] Port `iotEnvironmentalSensor` off BME680 → SHTC3 (+ SGP later)
- [x] Port SKU 5 security I/O onto P4 GPIOs; wire Matter 1.5 camera to intercom UX (bridge EVT/CMD; C6 policy + P4 sense/actuate)
- [ ] SKU 6: field-check ESPectre CSI occupancy while MJPEG `/stream` is on (firmware wired; not validated on hardware)

### Phase 3 — Flux carriers

- [ ] SKU 3 carrier: **SGP41**, header mating, mounts
- [ ] SKU 5 carrier: doorbell, PIR, tamper, siren, power as needed
- [ ] SKU 6 carrier (optional): mount / tamper / power for Sense module

### Phase 4 — HA / manufacturing

- [x] HA: SKU 6 stream-wrapper package; SKU 5 **no package** (Matter Live View)
- [ ] Slim SKU 6 firmware to video/stream-gate only (drop inherited doorbell/PIR/tamper code)
- [ ] Update BUILD.md / MANUFACTURING.md targets when P4 builds land in-tree
