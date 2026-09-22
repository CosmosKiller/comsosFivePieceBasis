# Home Assistant — SKU packages (firmware monorepo)

Device/SKU HA YAML that ships with firmware documentation. **Canonical** location is here; [cosmos-ha-field](https://github.com/CosmosKiller/cosmos-ha-field) keeps deploy copies for the Pi.

| Path | Purpose |
|------|---------|
| [packages/cosmos_door_sensor.yaml](packages/cosmos_door_sensor.yaml) | Low-battery notify + latch — `iotDoorSensor` |
| [packages/cosmos_security_camera.yaml](packages/cosmos_security_camera.yaml) | MJPEG wrapper + presence/siren/auto-off — `iotSecurityCamera` (SKU 6) |
| [packages/cosmos_security.yaml](packages/cosmos_security.yaml) | Intrusion alarm for door/window trips (strobe + sirens) |
| [lovelace/cosmos_security_camera.yaml](lovelace/cosmos_security_camera.yaml) | Security-cam dashboard view (stock cards) |
| [secrets.yaml.example](secrets.yaml.example) | Optional stream URL note (camera is UI-configured) |

**SKU 5 (`iotDoorIntercom`):** **no HA package** — Matter Live View + native doorbell / PIR / tamper / siren entities.

## Install packages

1. Copy desired files → HA OS `/config/packages/`
2. In `configuration.yaml`:
   ```yaml
   homeassistant:
     packages: !include_dir_named packages
   ```
3. **Developer tools → Check configuration** → restart HA.
4. Replace every `# TODO` entity id after Matter commissioning.

## Add SKU 6 MJPEG camera (UI — required)

SKU 6 has **no Matter camera entity**. YAML `camera: platform: mjpeg` is **not** supported in current HA. Add the stream in the UI:

1. Enable the Matter **stream gate** switch (so `/stream` answers).
2. **Settings → Devices & services → Add integration → MJPEG IP Camera**
3. MJPEG URL: `https://<device-ip>/stream`  
   Verify SSL: **off** (self-signed Beta cert)  
   Name: **Cosmos Security Camera** (entity becomes `camera.cosmos_security_camera`)
4. If the entity id differs, rename it or update Lovelace to match.

## Install security-cam Lovelace view

1. Open the dashboard → ⋮ → **Edit** → **Raw configuration editor** (or YAML mode).
2. Under `views:`, add the contents of [lovelace/cosmos_security_camera.yaml](lovelace/cosmos_security_camera.yaml) as **one list item**.
3. Fix TODO entity ids to match the package.
4. Save. Open path **`/lovelace/cosmos-security-cam`**.

## Security groups (`cosmos_security` package)

| Group | Entity | Purpose |
|-------|--------|---------|
| Security lights | `light.security_system_lights` | Red strobe during intrusion (create in UI or YAML — not in package) |
| Security sirens | `switch.security_system_sirens` | **Switch group** in [packages/cosmos_security.yaml](packages/cosmos_security.yaml) — Matter siren switches |

After commissioning each unit, add its `switch.*_siren` to the **Security System Sirens** group. SKU 5 siren belongs here when present; SKU 6 siren is **later**.

## Entity checklist (SKU 6 security camera)

| Role | Typical domain | Used as |
|------|----------------|---------|
| Stream gate (OnOff) | `switch.*` | Enable / disable HTTPS `/stream` |
| MJPEG camera | `camera.cosmos_security_camera` | Created via UI (MJPEG IP Camera) |
| Presence | `binary_sensor.*` | Auto stream (CSI / espectre later) |
| Siren clear | `switch.*` | Off = silence (later) |
| Auto-off helper | `input_number.*` | Minutes until stream gate Off |
| Snapshot | — | **Later** |

## Entity checklist (SKU 5 door intercom)

No package. After Matter commission + Live View:

| Role | Typical domain | Notes |
|------|----------------|-------|
| Matter camera / Live View | (controller Live View) | Video now; 2-way later |
| Doorbell | `event.*` / switch | Carrier |
| PIR occupancy | `binary_sensor.*` | Carrier |
| Tamper | `binary_sensor.*` | Carrier |
| Siren clear | `switch.*` | Off = silence |

## Entity checklist (door sensor)

After commissioning, rename the Matter device to **Cosmos Door Sensor** (optional). Firmware **Fixed Labels** default endpoint names to **Door contact**, **Arm / disarm**, **Panic alarm**, and **Siren**:

| Role | Typical domain | Default Matter name | Notes |
|------|----------------|---------------------|-------|
| Contact (Boolean State) | `binary_sensor.*` | Door contact | Reed open = on |
| Arm/disarm (OnOff) | `switch.*` | Arm / disarm | ON = arm; **OFF = disarm + clear panic/siren + silence** |
| Panic indicator (Boolean State) | `binary_sensor.*` | Panic alarm | Read-only; ON = intrusion; off when reed closes |
| Siren (mounted OnOff) | `switch.*` | Siren | ON = buzzer; **Off = silence** |
| Battery (Power Source) | `sensor.*` | (varies) | Low-battery notify in package |

## Sync deploy copy → cosmos-ha-field

```bash
rsync -av --delete \
  /home/cosmos/myProjects/pioIdfTest/cosmosFivePieceBasis/home-assistant/packages/ \
  /home/cosmos/uHome/myProjects/cosmos-ha-field/packages/ && \
rsync -av --delete \
  /home/cosmos/myProjects/pioIdfTest/cosmosFivePieceBasis/home-assistant/lovelace/ \
  /home/cosmos/uHome/myProjects/cosmos-ha-field/lovelace/
```

## Field / Pi

Commissioning, Pi OTA, and fleet docs: **[cosmos-ha-field](https://github.com/CosmosKiller/cosmos-ha-field)**.
