# Home Assistant integration

Matter commissioning, battery alerts, and (later) field-deploy automations for Cosmos devices. Copy the YAML under [`home-assistant/`](../home-assistant/) into your HA `config/` before or when beta kits go out.

Related: [HARDWARE.md](HARDWARE.md) (power / divider), [HAOTA.md](HAOTA.md) (OTA via DCL), [TESTING.md](TESTING.md) (chip-tool OTA).

---

## Commissioning (MVP sensor)

1. Flash and commission the binary sensor to HA (Companion app → **Settings → Matter → Add device**).
2. Confirm entities appear: **contact / boolean state**, **Power Source** or **battery** attributes.
3. Note the **battery % entity id** (Developer tools → States, filter by device name). Update placeholders in [`home-assistant/packages/cosmos_binary_sensor.yaml`](../home-assistant/packages/cosmos_binary_sensor.yaml).

Firmware publishes Power Source on endpoint 3 (`cosmos_battery`):

| Matter attribute | Meaning |
|------------------|---------|
| BatPercentRemaining | 0–200 in spec (= 0–100% in 0.5% steps); HA usually shows 0–100 |
| BatVoltage | Cell voltage (mV) after 2:1 divider math in firmware |

Sample interval defaults to **60 s** per app `sdkconfig.defaults` — fine for low-battery notifications, not for live fuel-gauge accuracy.

---

## Low-battery notifications

Voltage-based % is enough for MVP: when it drops below a threshold, notify the household once (with debounce and optional “already notified” latch).

**Install**

1. Copy `home-assistant/packages/cosmos_binary_sensor.yaml` → `config/packages/cosmos_binary_sensor.yaml`
2. Ensure `configuration.yaml` loads packages:

   ```yaml
   homeassistant:
     packages: !include_dir_named packages
   ```

3. Replace placeholder entity ids (`sensor.cosmos_contact_battery` → your real id).
4. **Check configuration** → restart HA.

**Tune**

| Setting | Default | Notes |
|---------|---------|-------|
| Low threshold | 20% | Start here; adjust after field data |
| `for` debounce | 10 min | Reduces false alerts when Wi‑Fi load dips voltage |
| Recovery threshold | 25% | Clears latch after charge / recovery |

Optional: add a dashboard card (Mushroom / tile) bound to the same battery entity.

---

## Beta kit (sensor + Raspberry Pi) — later

When field OTA via Pi is deployed, extend this doc / packages with:

- [ ] Pi reachability (`ping` or `binary_sensor` via API)
- [ ] “Firmware update ready” helper tied to Pi sync (see field OTA architecture in chat / future `FIELD_OTA.md`)
- [ ] Per-home device naming (`cosmos_contact_alice`, etc.)

---

## Entity naming convention (recommended)

| Device | Suggested entity prefix |
|--------|-------------------------|
| Binary sensor @ home Alice | `sensor.cosmos_contact_alice_battery` |
| Binary sensor @ home Bob | `sensor.cosmos_contact_bob_battery` |

Use **Settings → Devices → Rename** in HA so automations stay readable across homes.

---

## Troubleshooting

| Symptom | Check |
|---------|--------|
| No battery entity | Power Source endpoint commissioned? Re-interview device. |
| % stuck at 0 or 1 | Open circuit on divider / no battery on BAT pads |
| % jumps wildly | Normal under TX burst; increase automation `for:` debounce |
| Alert while on USB charge | Voltage rises when charging; raise threshold or disable alert when a “charging” proxy exists |

---

## Tracking checklist

Copy into your HA project board or issue:

- [ ] Commission test sensor to dev HA
- [ ] Record battery entity id in `cosmos_binary_sensor.yaml`
- [ ] Verify low-battery automation (temporarily set threshold to 90% for test)
- [ ] Verify recovery / latch reset
- [ ] Mobile / persistent notification channel configured
- [ ] Per beta home: duplicate package or use multi-entity template (before Pi kit rollout)
